BEGIN;
-- Stage One: shared Hall foundation. Daily work orders are server-issued
-- prototype supplies, not a claim of authoritative ownership of local inventory.
alter table public.guilds add column if not exists project_timber int not null default 0;
alter table public.guilds add column if not exists project_ore int not null default 0;
alter table public.guilds add column if not exists project_tools int not null default 0;
alter table public.guilds add column if not exists project_contracts int not null default 0;
create table if not exists public.guild_project_receipts (
  request_id uuid primary key,
  guild_id uuid not null references public.guilds(id) on delete cascade,
  user_id uuid not null references auth.users(id) on delete cascade,
  day int not null, kind text not null, created_at timestamptz not null default now(),
  unique(guild_id, user_id, day)
);
alter table public.guild_project_receipts enable row level security;
create or replace function public.tf_project_work(p_kind text, p_request uuid) returns text
language plpgsql security definer set search_path = public as $$
declare m guild_members; g guilds; receipt guild_project_receipts;
begin
  select * into m from guild_members where user_id = auth.uid() for update;
  if m.user_id is null then raise exception 'You are not in a guild.'; end if;
  select * into receipt from guild_project_receipts where request_id = p_request;
  if found then
    if receipt.user_id <> m.user_id or receipt.guild_id <> m.guild_id or receipt.kind <> p_kind then
      raise exception 'This receipt belongs to another contribution.';
    end if;
    return receipt.kind;
  end if;
  select * into g from guilds where id = m.guild_id for update;
  if g.hall_level > 1 or g.build_until is not null then raise exception 'The foundation is already finished.'; end if;
  if p_kind not in ('timber','ore','tools','contracts') then raise exception 'Unknown work order.'; end if;
  if exists(select 1 from guild_project_receipts where guild_id = g.id and user_id = m.user_id and day = tf_day_key()) then
    raise exception 'You have completed your daily construction order.';
  end if;
  if (p_kind = 'timber' and g.project_timber >= 20) or (p_kind = 'ore' and g.project_ore >= 12)
    or (p_kind = 'tools' and g.project_tools >= 8) or (p_kind = 'contracts' and g.project_contracts >= 3) then
    raise exception 'That requirement is already supplied.';
  end if;
  insert into guild_project_receipts(request_id,guild_id,user_id,day,kind) values(p_request,g.id,m.user_id,tf_day_key(),p_kind);
  update guilds set project_timber = project_timber + case when p_kind = 'timber' then 1 else 0 end,
    project_ore = project_ore + case when p_kind = 'ore' then 1 else 0 end,
    project_tools = project_tools + case when p_kind = 'tools' then 1 else 0 end,
    project_contracts = project_contracts + case when p_kind = 'contracts' then 1 else 0 end where id = g.id;
  return p_kind;
end $$;
revoke all on function public.tf_project_work(text,uuid) from public, anon;
grant execute on function public.tf_project_work(text,uuid) to authenticated;

-- Everything the Guild Hall screens show, as one JSON document.
create or replace function public.tf_guild_hub() returns json
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); m guild_members; g guilds; wk text := tf_week_now(); res json;
begin
  select * into m from guild_members where user_id = me;
  if m.user_id is null then return null; end if;
  perform tf_hall_settle(m.guild_id);
  if m.contrib_week <> wk then update guild_members set contrib = 0, contrib_week = wk where user_id = me; m.contrib := 0; end if;
  select * into g from guilds where id = m.guild_id;
  select json_build_object(
    'project', json_build_object('timber',g.project_timber,'ore',g.project_ore,'tools',g.project_tools,'contracts',g.project_contracts,
      'worked',exists(select 1 from guild_project_receipts where guild_id=g.id and user_id=me and day=tf_day_key())),
    'hall', g.hall_level, 'funds', g.funds, 'cap', tf_member_cap(g.id), 'rec', g.recommended,
    'build_left', case when g.build_until is null then -1 else greatest(0, extract(epoch from (g.build_until - now()))::int) end,
    'build_secs', g.build_secs, 'hands', g.build_hands,
    'lent', (g.build_until is not null and m.lent_build = tf_build_key(g)),
    'merit', m.merit, 'charges', floor(tf_charges(m))::int,
    'next_charge', case when tf_charges(m) >= 20 then 0 else ceil(720 - (tf_charges(m) - floor(tf_charges(m))) * 720)::int end,
    'contrib', m.contrib, 'help_merit', case when m.help_day = tf_day_key() then m.help_merit else 0 end,
    'tech', coalesce((select json_agg(json_build_object('t', t.tech, 'l', t.level, 'p', t.progress)) from guild_tech t where t.guild_id = g.id), '[]'::json),
    'helps', coalesce((select json_agg(json_build_object('id', h.id, 'who', h.char_name, 'what', h.label, 'n', h.helps, 'max', h.max_helps,
                                                         'mine', h.user_id = me,
                                                         'done', exists (select 1 from guild_help_by b where b.help_id = h.id and b.helper = me))
                                       order by h.id)
                       from guild_helps h where h.guild_id = g.id and h.open and h.created_at > now() - interval '1 day'), '[]'::json),
    'gifts', coalesce((select json_agg(json_build_object('id', x.id, 'k', x.kind, 'from', x.from_name) order by x.id)
                       from guild_gifts x where x.guild_id = g.id and x.id > m.gift_seen and x.created_at > now() - interval '3 days'), '[]'::json),
    'board', coalesce((select json_agg(json_build_object('n', b.char_name, 'c', b.contrib) order by b.contrib desc)
                       from guild_members b where b.guild_id = g.id and b.contrib_week = wk and b.contrib > 0), '[]'::json)
  ) into res;
  return res;
end $$;

create or replace function public.tf_hall_upgrade() returns void
language plpgsql security definer set search_path = public as $$
declare gid uuid; r int; g guilds; cost bigint; secs int;
begin
  select * into gid, r from tf_my_rank();
  if gid is null or r < 1 then raise exception 'Only officers can upgrade the Hall.'; end if;
  perform tf_hall_settle(gid);
  select * into g from guilds where id = gid for update;
  if g.build_until is not null then raise exception 'The Hall is already being built.'; end if;
  if g.hall_level >= 10 then raise exception 'The Hall is at its greatest.'; end if;
  cost := 200 * g.hall_level * g.hall_level;
  if g.hall_level = 1 then
    if g.project_timber < 20 or g.project_ore < 12 or g.project_tools < 8 or g.project_contracts < 3 then
      raise exception 'Finish the foundation work orders first.';
    end if;
    cost := 0;
  end if;
  if g.funds < cost then raise exception 'The guild needs % funds (it has %).', cost, g.funds; end if;
  secs := 3600 * g.hall_level;
  update guilds set funds = funds - cost, build_until = now() + make_interval(secs => secs), build_secs = secs, build_hands = 0 where id = gid;
end $$;


create or replace function public.tf_shop_buy(p_item text) returns int
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); price int; left_ int; m guild_members;
begin
  select * into m from guild_members where user_id = me for update;
  if m.user_id is null then raise exception 'You are not in a guild.'; end if;
  perform tf_hall_settle(m.guild_id);
  if (select hall_level from guilds where id = m.guild_id) < 2 then raise exception 'Complete the Hall foundation to open the Storehouse.'; end if;
  price := case p_item when 'bandages' then 60 when 'potions' then 120 when 'reagents' then 100
                        when 'horn' then 150 when 'dye' then 300 when 'map' then 500 else null end;
  if price is null then raise exception 'That is not for sale.'; end if;
  update guild_members set merit = merit - price where user_id = me and merit >= price returning merit into left_;
  if left_ is null then raise exception 'Not enough merit (% needed).', price; end if;
  return left_;
end $$;


COMMIT;
