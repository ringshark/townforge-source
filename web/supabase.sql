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
