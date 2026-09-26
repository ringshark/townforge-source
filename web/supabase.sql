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
