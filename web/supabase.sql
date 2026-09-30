-- Town Forge cloud saves: run once in Supabase (SQL Editor -> New query -> Run).
-- One row per player; row-level security means each signed-in player can only
-- ever read or write their own save.
create table if not exists public.saves (
  user_id    uuid primary key references auth.users (id) on delete cascade,
  data       text not null,          -- the whole save file
  prev_data  text,                   -- an older copy (kept at most hourly) as a safety net
  updated_at timestamptz not null default now()
);
alter table public.saves enable row level security;
drop policy if exists "players manage their own save" on public.saves;
create policy "players manage their own save" on public.saves
  for all using (auth.uid() = user_id) with check (auth.uid() = user_id);

-- ===================================================================
-- Online guilds + War Week (2026-09-27). Safe to re-run.
-- Players never write these tables directly: every change goes through the
-- tf_* functions below, which check who is asking (auth.uid()), keep points
-- honest (current week only, capped per day) and tidy up empty guilds.
-- Week key: 'W' + whole weeks since Monday 1970-01-05 (UTC); day 0 = Monday.
-- ===================================================================
create table if not exists public.guilds (
  id         uuid primary key default gen_random_uuid(),
  name       text not null check (char_length(name) between 3 and 24),
  tag        text not null check (char_length(tag) between 2 and 4),
  created_by uuid references auth.users (id) on delete set null,
  created_at timestamptz not null default now()
);
create unique index if not exists guilds_name_key on public.guilds (lower(name));

create table if not exists public.guild_members (
  user_id   uuid primary key references auth.users (id) on delete cascade,
  guild_id  uuid not null references public.guilds (id) on delete cascade,
  char_name text not null default 'Adventurer' check (char_length(char_name) <= 24),
  power     int  not null default 0,
  snapshot  text check (char_length(snapshot) <= 4000),  -- the character, for Battle Day
  joined_at timestamptz not null default now(),
  seen_at   timestamptz not null default now()
);
create index if not exists guild_members_guild on public.guild_members (guild_id);

create table if not exists public.war_scores (
  week       text not null,
  day        int  not null check (day between 0 and 6),
  user_id    uuid not null references auth.users (id) on delete cascade,
  guild_id   uuid not null references public.guilds (id) on delete cascade,
  points     int  not null check (points between 0 and 5000),
  updated_at timestamptz not null default now(),
  primary key (week, day, user_id)
);
create index if not exists war_scores_week on public.war_scores (week, guild_id);

alter table public.guilds        enable row level security;
alter table public.guild_members enable row level security;
alter table public.war_scores    enable row level security;
drop policy if exists "signed-in players can see guilds" on public.guilds;
create policy "signed-in players can see guilds" on public.guilds for select to authenticated using (true);
drop policy if exists "signed-in players can see members" on public.guild_members;
create policy "signed-in players can see members" on public.guild_members for select to authenticated using (true);
drop policy if exists "signed-in players can see war scores" on public.war_scores;
create policy "signed-in players can see war scores" on public.war_scores for select to authenticated using (true);
-- (no insert/update/delete policies: writes only happen inside the functions)

create or replace function public.tf_week_now() returns text language sql stable as $$
  select 'W' || floor((extract(epoch from now()) - 345600) / 604800)::bigint;
$$;
create or replace function public.tf_day_now() returns int language sql stable as $$
  select (floor((extract(epoch from now()) - 345600) / 86400)::bigint % 7)::int;
$$;

create or replace function public.tf_clean(t text, n int) returns text language sql immutable as $$
  select left(btrim(regexp_replace(regexp_replace(t, '[|\r\n\t]', ' ', 'g'), '\s+', ' ', 'g')), n);
$$;

create or replace function public.tf_create_guild(p_name text, p_tag text, p_char text default 'Adventurer', p_power int default 0)
returns uuid language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); gid uuid;
begin
  if me is null then raise exception 'Sign in first.'; end if;
  if exists (select 1 from guild_members where user_id = me) then raise exception 'Leave your guild first.'; end if;
  p_name := coalesce(tf_clean(p_name, 40), '');
  p_tag  := upper(btrim(regexp_replace(coalesce(p_tag, ''), '[^A-Za-z0-9]', '', 'g')));
  if char_length(p_name) not between 3 and 24 then raise exception 'Guild names are 3-24 letters.'; end if;
  if char_length(p_tag) not between 2 and 4 then raise exception 'Tags are 2-4 letters or digits.'; end if;
  if exists (select 1 from guilds where lower(name) = lower(p_name)) then raise exception 'That guild name is taken.'; end if;
  insert into guilds (name, tag, created_by) values (p_name, p_tag, me) returning id into gid;
  insert into guild_members (user_id, guild_id, char_name, power)
    values (me, gid, coalesce(nullif(tf_clean(p_char, 24), ''), 'Adventurer'), greatest(0, least(coalesce(p_power, 0), 100000)));
  return gid;
end $$;

create or replace function public.tf_join_guild(p_guild uuid, p_char text default 'Adventurer', p_power int default 0)
returns void language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid();
begin
  if me is null then raise exception 'Sign in first.'; end if;
  if exists (select 1 from guild_members where user_id = me) then raise exception 'Leave your guild first.'; end if;
  perform 1 from guilds where id = p_guild for update;   -- one join at a time per guild
  if not found then raise exception 'That guild no longer exists.'; end if;
  if (select count(*) from guild_members where guild_id = p_guild) >= 30 then raise exception 'That guild is full (30 members).'; end if;
  insert into guild_members (user_id, guild_id, char_name, power)
    values (me, p_guild, coalesce(nullif(tf_clean(p_char, 24), ''), 'Adventurer'), greatest(0, least(coalesce(p_power, 0), 100000)));
end $$;

create or replace function public.tf_leave_guild() returns void
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); gid uuid;
begin
  delete from guild_members where user_id = me returning guild_id into gid;
  if gid is not null and not exists (select 1 from guild_members where guild_id = gid) then
    delete from guilds where id = gid;   -- the last one out closes the hall
  end if;
end $$;

-- Report today's points (the game sends its running total for the day).
create or replace function public.tf_submit(p_week text, p_day int, p_points int, p_char text default null, p_power int default null, p_snap text default null)
returns void language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); gid uuid; today int := tf_day_now();
begin
  if me is null then raise exception 'Sign in first.'; end if;
  select guild_id into gid from guild_members where user_id = me;
  if gid is null then return; end if;
  if p_week <> tf_week_now() then return; end if;                 -- only the current war week
  if p_day not in (today, (today + 6) % 7) or (p_day = 6 and today = 0) then return; end if; -- today (or just before midnight)
  insert into war_scores (week, day, user_id, guild_id, points)
    values (p_week, p_day, me, gid, greatest(0, least(coalesce(p_points, 0), 5000)))
  on conflict (week, day, user_id) do update
    set points = greatest(war_scores.points, excluded.points), guild_id = excluded.guild_id, updated_at = now();
  update guild_members set seen_at = now(),
    char_name = coalesce(nullif(tf_clean(p_char, 24), ''), char_name),
    power     = case when p_power is null then power else greatest(0, least(p_power, 100000)) end,
    snapshot  = coalesce(left(p_snap, 4000), snapshot)
  where user_id = me;
end $$;

-- Guild table for a week: members and war points, best first.
create or replace function public.tf_standings(p_week text)
returns table (id uuid, name text, tag text, members int, points bigint)
language sql stable security definer set search_path = public as $$
  select g.id, g.name, g.tag,
         (select count(*)::int from guild_members m where m.guild_id = g.id),
         coalesce((select sum(w.points) from war_scores w where w.guild_id = g.id and w.week = p_week), 0)
  from guilds g
  order by 5 desc, 4 desc, g.created_at
  limit 100;
$$;

revoke all on function public.tf_create_guild(text, text, text, int), public.tf_join_guild(uuid, text, int),
  public.tf_leave_guild(), public.tf_submit(text, int, int, text, int, text), public.tf_standings(text) from public, anon;
grant execute on function public.tf_create_guild(text, text, text, int), public.tf_join_guild(uuid, text, int),
  public.tf_leave_guild(), public.tf_submit(text, int, int, text, int, text), public.tf_standings(text) to authenticated;

-- ===================================================================
-- Guild management (2026-09-28): ranks, message of the day, guild wars.
-- rank: 0 member, 1 officer, 2 leader. Safe to re-run.
-- ===================================================================
alter table public.guild_members add column if not exists rank int not null default 0 check (rank between 0 and 2);
alter table public.guilds add column if not exists motd text not null default '' check (char_length(motd) <= 140);
-- every guild gets a leader: its founder if still in it, else its longest-standing member
update public.guild_members m set rank = 2
  from public.guilds g where g.id = m.guild_id and m.user_id = g.created_by
  and not exists (select 1 from public.guild_members x where x.guild_id = g.id and x.rank = 2);
update public.guild_members m set rank = 2
  where m.user_id = (select x.user_id from public.guild_members x where x.guild_id = m.guild_id order by x.joined_at limit 1)
  and not exists (select 1 from public.guild_members x where x.guild_id = m.guild_id and x.rank = 2);

create table if not exists public.guild_wars (
  guild_id    uuid not null references public.guilds (id) on delete cascade,
  target_id   uuid not null references public.guilds (id) on delete cascade,
  declared_by uuid references auth.users (id) on delete set null,
  declared_at timestamptz not null default now(),
  primary key (guild_id, target_id),
  check (guild_id <> target_id)
);
alter table public.guild_wars enable row level security;
drop policy if exists "signed-in players can see guild wars" on public.guild_wars;
create policy "signed-in players can see guild wars" on public.guild_wars for select to authenticated using (true);

-- the founder leads
create or replace function public.tf_create_guild(p_name text, p_tag text, p_char text default 'Adventurer', p_power int default 0)
returns uuid language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); gid uuid;
begin
  if me is null then raise exception 'Sign in first.'; end if;
  if exists (select 1 from guild_members where user_id = me) then raise exception 'Leave your guild first.'; end if;
  p_name := coalesce(tf_clean(p_name, 40), '');
  p_tag  := upper(btrim(regexp_replace(coalesce(p_tag, ''), '[^A-Za-z0-9]', '', 'g')));
  if char_length(p_name) not between 3 and 24 then raise exception 'Guild names are 3-24 letters.'; end if;
  if char_length(p_tag) not between 2 and 4 then raise exception 'Tags are 2-4 letters or digits.'; end if;
  if exists (select 1 from guilds where lower(name) = lower(p_name)) then raise exception 'That guild name is taken.'; end if;
  insert into guilds (name, tag, created_by) values (p_name, p_tag, me) returning id into gid;
  insert into guild_members (user_id, guild_id, char_name, power, rank)
    values (me, gid, coalesce(nullif(tf_clean(p_char, 24), ''), 'Adventurer'), greatest(0, least(coalesce(p_power, 0), 100000)), 2);
  return gid;
end $$;

-- a leader who leaves hands the guild to the highest-ranked, longest-standing member
create or replace function public.tf_leave_guild() returns void
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); gid uuid; r int;
begin
  delete from guild_members where user_id = me returning guild_id, rank into gid, r;
  if gid is null then return; end if;
  if not exists (select 1 from guild_members where guild_id = gid) then
    delete from guilds where id = gid;   -- the last one out closes the hall
  elsif r = 2 then
    update guild_members set rank = 2 where user_id =
      (select user_id from guild_members where guild_id = gid order by rank desc, joined_at limit 1);
  end if;
end $$;

create or replace function public.tf_my_rank(out gid uuid, out r int)
language sql stable security definer set search_path = public as $$
  select guild_id, rank from guild_members where user_id = auth.uid();
$$;

create or replace function public.tf_set_motd(p_text text) returns void
language plpgsql security definer set search_path = public as $$
declare gid uuid; r int;
begin
  select * into gid, r from tf_my_rank();
  if gid is null then raise exception 'You are not in a guild.'; end if;
  if r < 1 then raise exception 'Only officers can change the message.'; end if;
  update guilds set motd = coalesce(tf_clean(p_text, 140), '') where id = gid;
end $$;

create or replace function public.tf_kick(p_user uuid) returns void
language plpgsql security definer set search_path = public as $$
declare gid uuid; r int; tr int;
begin
  select * into gid, r from tf_my_rank();
  if gid is null or r < 1 then raise exception 'Only officers can remove members.'; end if;
  select rank into tr from guild_members where user_id = p_user and guild_id = gid;
  if tr is null then raise exception 'They are not in your guild.'; end if;
  if tr >= r then raise exception 'You can only remove members ranked below you.'; end if;
  delete from guild_members where user_id = p_user;
end $$;

-- leader only: 0 member / 1 officer; 2 hands over leadership (you become an officer)
create or replace function public.tf_set_rank(p_user uuid, p_rank int) returns void
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); gid uuid; r int;
begin
  select * into gid, r from tf_my_rank();
  if gid is null or r < 2 then raise exception 'Only the guild leader can change ranks.'; end if;
  if p_user = me then raise exception 'Pick another member.'; end if;
  if not exists (select 1 from guild_members where user_id = p_user and guild_id = gid) then raise exception 'They are not in your guild.'; end if;
  if p_rank = 2 then
    update guild_members set rank = 1 where user_id = me;
    update guild_members set rank = 2 where user_id = p_user;
  else
    update guild_members set rank = greatest(0, least(p_rank, 1)) where user_id = p_user;
  end if;
end $$;

create or replace function public.tf_declare_war(p_target uuid) returns void
language plpgsql security definer set search_path = public as $$
declare gid uuid; r int;
begin
  select * into gid, r from tf_my_rank();
  if gid is null or r < 1 then raise exception 'Only officers can declare war.'; end if;
  if p_target = gid then raise exception 'You cannot declare war on yourselves.'; end if;
  if not exists (select 1 from guilds where id = p_target) then raise exception 'That guild no longer exists.'; end if;
  if (select count(*) from guild_wars where guild_id = gid) >= 5 then raise exception 'A guild can wage at most 5 wars at once.'; end if;
  insert into guild_wars (guild_id, target_id, declared_by) values (gid, p_target, auth.uid()) on conflict do nothing;
end $$;

create or replace function public.tf_end_war(p_target uuid) returns void
language plpgsql security definer set search_path = public as $$
declare gid uuid; r int;
begin
  select * into gid, r from tf_my_rank();
  if gid is null or r < 1 then raise exception 'Only officers can end a war.'; end if;
  delete from guild_wars where guild_id = gid and target_id = p_target;
end $$;

revoke all on function public.tf_my_rank(), public.tf_set_motd(text), public.tf_kick(uuid), public.tf_set_rank(uuid, int),
  public.tf_declare_war(uuid), public.tf_end_war(uuid) from public, anon;
grant execute on function public.tf_set_motd(text), public.tf_kick(uuid), public.tf_set_rank(uuid, int),
  public.tf_declare_war(uuid), public.tf_end_war(uuid) to authenticated;

-- ===================================================================
-- Guild Hall (2026-09-29): the alliance loop, after Whiteout Survival.
-- Safe to re-run. As before, players never touch these tables directly;
-- everything goes through the tf_* functions, which check membership and rank,
-- enforce the daily limits server-side and do each change in one transaction.
--   Hall      shared level; officers spend guild funds to upgrade it (a timed
--             build that every member can lend a hand to, once each)
--   Research  members donate to techs; donations come from a charge pool that
--             refills over time (20 max, one per 12 minutes) - the pool is the
--             anti-abuse limit, whatever a client claims to have in its pack
--   Help      ask the guild to speed your settlement build; each help cuts it
--   Merit     your guild currency (donations, helping); spent in the shop
--   Gifts     boss kills and treasure chests send every member a gift
-- ===================================================================
alter table public.guilds add column if not exists hall_level  int    not null default 1 check (hall_level between 1 and 10);
alter table public.guilds add column if not exists funds       bigint not null default 0 check (funds >= 0);
alter table public.guilds add column if not exists build_until timestamptz;           -- upgrading to hall_level + 1 until then
alter table public.guilds add column if not exists build_secs  int    not null default 0;
alter table public.guilds add column if not exists build_hands int    not null default 0;
alter table public.guilds add column if not exists recommended text   not null default '';

alter table public.guild_members add column if not exists merit        int  not null default 0 check (merit >= 0);
alter table public.guild_members add column if not exists charges      real not null default 20;
alter table public.guild_members add column if not exists charges_at   timestamptz not null default now();
alter table public.guild_members add column if not exists contrib      int  not null default 0;    -- this week
alter table public.guild_members add column if not exists contrib_week text not null default '';
alter table public.guild_members add column if not exists help_day     int  not null default -1;   -- merit from helping, per day
alter table public.guild_members add column if not exists help_merit   int  not null default 0;
alter table public.guild_members add column if not exists lent_build   timestamptz;               -- the build they lent a hand to
alter table public.guild_members add column if not exists gift_seen    bigint not null default 0;

create table if not exists public.guild_tech (
  guild_id uuid not null references public.guilds (id) on delete cascade,
  tech     text not null,
  level    int  not null default 0 check (level between 0 and 5),
  progress int  not null default 0,
  primary key (guild_id, tech)
);
create table if not exists public.guild_helps (
  id         bigserial primary key,
  guild_id   uuid not null references public.guilds (id) on delete cascade,
  user_id    uuid not null references auth.users (id) on delete cascade,
  char_name  text not null default 'Adventurer',
  label      text not null default '',
  total_secs int  not null default 0,
  helps      int  not null default 0,
  max_helps  int  not null default 6,
  open       boolean not null default true,
  created_at timestamptz not null default now()
);
create index if not exists guild_helps_open on public.guild_helps (guild_id) where open;
create table if not exists public.guild_help_by (
  help_id bigint not null references public.guild_helps (id) on delete cascade,
  helper  uuid   not null references auth.users (id) on delete cascade,
  primary key (help_id, helper)
);
create table if not exists public.guild_gifts (
  id         bigserial primary key,
  guild_id   uuid not null references public.guilds (id) on delete cascade,
  kind       int  not null check (kind between 0 and 3),   -- 0 dungeon boss, 1 world boss, 2 treasure chest, 3 raid repelled
  from_user  uuid references auth.users (id) on delete set null,
  from_name  text not null default '',
  created_at timestamptz not null default now()
);
create index if not exists guild_gifts_guild on public.guild_gifts (guild_id, id);
alter table public.guild_tech    enable row level security;
alter table public.guild_helps   enable row level security;
alter table public.guild_help_by enable row level security;
alter table public.guild_gifts   enable row level security;
-- (no policies: only the functions below read or write these)

-- The research tree. tier = the Hall level it needs; kind = what donations cost
-- in the game (1 wood, 2 ore, 3 leather, 4 gold) - the game debits it locally.
create or replace function public.tf_tech_tier(t text) returns int language sql immutable as $$
  select case t when 'timber' then 1 when 'veins' then 1 when 'tanner' then 1 when 'builders' then 1
                when 'coffers' then 2 when 'ironhide' then 2 when 'keen' then 3 when 'arcane' then 3
                when 'hands' then 4 when 'expansion' then 4 else 0 end;
$$;
create or replace function public.tf_day_key() returns int language sql stable as $$
  select floor((extract(epoch from now()) - 345600) / 86400)::int;
$$;
create or replace function public.tf_member_cap(gid uuid) returns int language sql stable as $$
  select 30 + 2 * (g.hall_level - 1) + 2 * coalesce((select level from guild_tech where guild_id = gid and tech = 'expansion'), 0)
  from guilds g where g.id = gid;
$$;
-- Finish a hall build whose time is up (called before anything reads or changes the hall).
create or replace function public.tf_hall_settle(gid uuid) returns void language plpgsql security definer set search_path = public as $$
begin
  update guilds set hall_level = least(10, hall_level + 1), build_until = null, build_secs = 0, build_hands = 0
  where id = gid and build_until is not null and build_until <= now();
end $$;
-- A hall build's identity: its original finish time (every hand lent cuts exactly build_secs / 20).
create or replace function public.tf_build_key(g guilds) returns timestamptz language sql immutable as $$
  select g.build_until + make_interval(secs => g.build_hands * (g.build_secs / 20));
$$;
-- A member's donation charges, refilled up to now (20 max, one per 720 s).
create or replace function public.tf_charges(m guild_members) returns real language sql stable as $$
  select least(20, m.charges + extract(epoch from (now() - m.charges_at))::real / 720);
$$;

-- joining respects the Hall's member cap now
create or replace function public.tf_join_guild(p_guild uuid, p_char text default 'Adventurer', p_power int default 0)
returns void language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid();
begin
  if me is null then raise exception 'Sign in first.'; end if;
  if exists (select 1 from guild_members where user_id = me) then raise exception 'Leave your guild first.'; end if;
  perform 1 from guilds where id = p_guild for update;
  if not found then raise exception 'That guild no longer exists.'; end if;
  if (select count(*) from guild_members where guild_id = p_guild) >= tf_member_cap(p_guild) then
    raise exception 'That guild is full.'; end if;
  insert into guild_members (user_id, guild_id, char_name, power)
    values (me, p_guild, coalesce(nullif(tf_clean(p_char, 24), ''), 'Adventurer'), greatest(0, least(coalesce(p_power, 0), 100000)));
end $$;

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

-- Donate one charge to a tech. Returns the tech's new level.
create or replace function public.tf_donate(p_tech text) returns int
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); m guild_members; g guilds; ch real; lv int; pr int; need int; gain int; wk text := tf_week_now();
begin
  select * into m from guild_members where user_id = me for update;
  if m.user_id is null then raise exception 'You are not in a guild.'; end if;
  if tf_tech_tier(p_tech) = 0 then raise exception 'Unknown research.'; end if;
  perform tf_hall_settle(m.guild_id);
  select * into g from guilds where id = m.guild_id for update;
  if g.hall_level < tf_tech_tier(p_tech) then raise exception 'The Guild Hall must be level % first.', tf_tech_tier(p_tech); end if;
  ch := tf_charges(m);
  if ch < 1 then raise exception 'No donations left - one refills every 12 minutes.'; end if;
  insert into guild_tech (guild_id, tech) values (g.id, p_tech) on conflict do nothing;
  select level, progress into lv, pr from guild_tech where guild_id = g.id and tech = p_tech for update;
  if lv >= 5 then raise exception 'That research is complete.'; end if;
  gain := case when g.recommended = p_tech then 15 else 10 end;
  need := 100 * (lv + 1);
  pr := pr + gain;
  if pr >= need then lv := lv + 1; pr := 0; end if;
  update guild_tech set level = lv, progress = pr where guild_id = g.id and tech = p_tech;
  update guild_members set charges = ch - 1, charges_at = now(), merit = merit + gain,
    contrib = case when contrib_week = wk then contrib else 0 end + gain, contrib_week = wk
  where user_id = me;
  update guilds set funds = funds + gain where id = g.id;
  return lv;
end $$;

create or replace function public.tf_recommend(p_tech text) returns void
language plpgsql security definer set search_path = public as $$
declare gid uuid; r int;
begin
  select * into gid, r from tf_my_rank();
  if gid is null or r < 1 then raise exception 'Only officers can recommend research.'; end if;
  if p_tech <> '' and tf_tech_tier(p_tech) = 0 then raise exception 'Unknown research.'; end if;
  update guilds set recommended = p_tech where id = gid;
end $$;

-- Officers start the next Hall level: costs funds, takes (level) hours.
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

-- Every member can lend a hand once per build: each cuts 5% of its full time.
create or replace function public.tf_lend_hand() returns void
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); m guild_members; g guilds;
begin
  select * into m from guild_members where user_id = me for update;
  if m.user_id is null then raise exception 'You are not in a guild.'; end if;
  perform tf_hall_settle(m.guild_id);
  select * into g from guilds where id = m.guild_id for update;
  if g.build_until is null then raise exception 'Nothing is being built.'; end if;
  if m.lent_build = tf_build_key(g) then raise exception 'You have already lent a hand to this build.'; end if;
  update guild_members set lent_build = tf_build_key(g), merit = merit + 20 where user_id = me;
  update guilds set build_until = build_until - make_interval(secs => g.build_secs / 20), build_hands = build_hands + 1 where id = g.id;
  perform tf_hall_settle(g.id);
end $$;

-- Ask the guild to help with a timer (your settlement build). One open request each.
create or replace function public.tf_request_help(p_label text, p_total int) returns bigint
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); m guild_members; g guilds; hid bigint; mx int;
begin
  select * into m from guild_members where user_id = me;
  if m.user_id is null then raise exception 'You are not in a guild.'; end if;
  select * into g from guilds where id = m.guild_id;
  update guild_helps set open = false where user_id = me and open;
  mx := 5 + g.hall_level + 2 * coalesce((select level from guild_tech where guild_id = g.id and tech = 'hands'), 0);
  insert into guild_helps (guild_id, user_id, char_name, label, total_secs, max_helps)
    values (g.id, me, m.char_name, coalesce(tf_clean(p_label, 40), ''), greatest(0, least(coalesce(p_total, 0), 864000)), mx)
    returning id into hid;
  return hid;
end $$;
create or replace function public.tf_close_help() returns void
language sql security definer set search_path = public as $$
  update guild_helps set open = false where user_id = auth.uid() and open;
$$;

-- Help everyone who asked. Returns how many you helped. Helping earns 5 merit
-- each, up to 50 a day.
create or replace function public.tf_help_all() returns int
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); m guild_members; n int := 0; h record; today int := tf_day_key(); earned int;
begin
  select * into m from guild_members where user_id = me for update;
  if m.user_id is null then raise exception 'You are not in a guild.'; end if;
  for h in select id from guild_helps
           where guild_id = m.guild_id and open and user_id <> me and helps < max_helps and created_at > now() - interval '1 day'
             and not exists (select 1 from guild_help_by b where b.help_id = guild_helps.id and b.helper = me)
           for update loop
    insert into guild_help_by (help_id, helper) values (h.id, me);
    update guild_helps set helps = helps + 1 where id = h.id;
    n := n + 1;
  end loop;
  if n > 0 then
    earned := case when m.help_day = today then m.help_merit else 0 end;
    update guild_members set help_day = today, help_merit = least(50, earned + 5 * n),
      merit = merit + greatest(0, least(50, earned + 5 * n) - earned)
    where user_id = me;
  end if;
  return n;
end $$;

-- The shop: prices live here, so a client can't name its own.
create or replace function public.tf_shop_buy(p_item text) returns int
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); price int; left_ int;
begin
  price := case p_item when 'bandages' then 60 when 'potions' then 120 when 'reagents' then 100
                        when 'horn' then 150 when 'dye' then 300 when 'map' then 500 else null end;
  if price is null then raise exception 'That is not for sale.'; end if;
  update guild_members set merit = merit - price where user_id = me and merit >= price returning merit into left_;
  if left_ is null then raise exception 'Not enough merit (% needed).', price; end if;
  return left_;
end $$;

-- A member's deed sends every member a gift (up to 12 a day each).
create or replace function public.tf_send_gift(p_kind int) returns void
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); m guild_members;
begin
  select * into m from guild_members where user_id = me;
  if m.user_id is null then return; end if;
  if p_kind not between 0 and 3 then raise exception 'Unknown gift.'; end if;
  if (select count(*) from guild_gifts where from_user = me and created_at > now() - interval '1 day') >= 12 then return; end if;
  insert into guild_gifts (guild_id, kind, from_user, from_name) values (m.guild_id, p_kind, me, m.char_name);
  update guilds set funds = funds + 5 where id = m.guild_id;
end $$;
-- Open every waiting gift: returns their kinds (the game rolls what's inside).
create or replace function public.tf_open_gifts() returns int[]
language plpgsql security definer set search_path = public as $$
declare me uuid := auth.uid(); m guild_members; kinds int[]; top bigint;
begin
  select * into m from guild_members where user_id = me for update;
  if m.user_id is null then return '{}'; end if;
  select array_agg(kind order by id), max(id) into kinds, top from guild_gifts
    where guild_id = m.guild_id and id > m.gift_seen and created_at > now() - interval '3 days';
  if top is not null then update guild_members set gift_seen = top where user_id = me; end if;
  return coalesce(kinds, '{}');
end $$;

-- someone who leaves or is removed takes their open help request with them
create or replace function public.tf_leave_cleanup() returns trigger language plpgsql security definer set search_path = public as $$
begin
  update guild_helps set open = false where user_id = old.user_id and open;
  return old;
end $$;
drop trigger if exists tf_member_left on public.guild_members;
create trigger tf_member_left after delete on public.guild_members for each row execute function public.tf_leave_cleanup();
-- a new member starts fresh (merit, charges and gifts don't follow people between guilds)
create or replace function public.tf_join_reset() returns trigger language plpgsql as $$
begin
  new.gift_seen := coalesce((select max(id) from public.guild_gifts where guild_id = new.guild_id), 0);
  return new;
end $$;
drop trigger if exists tf_member_joined on public.guild_members;
create trigger tf_member_joined before insert on public.guild_members for each row execute function public.tf_join_reset();

revoke all on function public.tf_guild_hub(), public.tf_donate(text), public.tf_recommend(text), public.tf_hall_upgrade(),
  public.tf_lend_hand(), public.tf_request_help(text, int), public.tf_close_help(), public.tf_help_all(),
  public.tf_shop_buy(text), public.tf_send_gift(int), public.tf_open_gifts(), public.tf_hall_settle(uuid),
  public.tf_member_cap(uuid), public.tf_charges(guild_members) from public, anon;
grant execute on function public.tf_guild_hub(), public.tf_donate(text), public.tf_recommend(text), public.tf_hall_upgrade(),
  public.tf_lend_hand(), public.tf_request_help(text, int), public.tf_close_help(), public.tf_help_all(),
  public.tf_shop_buy(text), public.tf_send_gift(int), public.tf_open_gifts() to authenticated;
