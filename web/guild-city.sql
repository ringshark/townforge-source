-- Guild city progression. Apply after supabase.sql; safe to repeat.
create table if not exists public.guild_city_stock (
 guild_id uuid primary key references guilds(id) on delete cascade,
 wood int not null default 0 check(wood>=0), ore int not null default 0 check(ore>=0), leather int not null default 0 check(leather>=0)
);
create table if not exists public.guild_city_buildings (
 guild_id uuid references guilds(id) on delete cascade, kind text not null,
 level int not null default 1 check(level between 1 and 5), until_at timestamptz,
 primary key(guild_id,kind), check(kind in ('workshop','storehouse','aid','command'))
);
create table if not exists public.guild_city_receipts (
 request_id uuid primary key, guild_id uuid not null references guilds(id) on delete cascade,
 user_id uuid not null references auth.users(id) on delete cascade,
 action text not null, target text not null, result jsonb not null,
 char_name text not null, created_at timestamptz not null default now()
);
create table if not exists public.guild_city_runs (
 id bigint generated always as identity primary key, guild_id uuid not null references guilds(id) on delete cascade,
 kind text not null check(kind in ('expedition','boss')), starts_at timestamptz not null default now(), ends_at timestamptz not null,
 goal int not null, progress int not null default 0, finished boolean not null default false,
 success boolean not null default false
);
create unique index if not exists guild_city_one_run on guild_city_runs(guild_id,kind) where not finished;
create table if not exists public.guild_city_participants (
 run_id bigint references guild_city_runs(id) on delete cascade, user_id uuid references auth.users(id) on delete cascade,
 points int not null default 0, claimed boolean not null default false, last_hit timestamptz,
 primary key(run_id,user_id)
);
create table if not exists public.guild_city_daily (
 guild_id uuid references guilds(id) on delete cascade, user_id uuid references auth.users(id) on delete cascade,
 day int not null, count int not null default 0, primary key(guild_id,user_id,day)
);
create table if not exists public.guild_city_battles (
 id bigint generated always as identity primary key, attacker uuid not null references guilds(id) on delete cascade,
 defender uuid not null references guilds(id) on delete cascade, starts_at timestamptz not null, ends_at timestamptz not null,
 check(attacker<>defender), unique(attacker,defender,starts_at)
);
create table if not exists public.guild_city_objectives (
 battle_id bigint references guild_city_battles(id) on delete cascade, guild_id uuid references guilds(id) on delete cascade,
 objective text not null check(objective in ('gate','tower','keep')), points int not null default 0,
 primary key(battle_id,guild_id,objective)
);
create table if not exists public.guild_city_combatants (
 battle_id bigint references guild_city_battles(id) on delete cascade, user_id uuid references auth.users(id) on delete cascade,
 guild_id uuid references guilds(id) on delete cascade, actions int not null default 0, claimed boolean not null default false,
 primary key(battle_id,user_id)
);
alter table guild_city_buildings add column if not exists build_key uuid;
create table if not exists guild_city_build_helpers (
 guild_id uuid references guilds(id) on delete cascade, kind text not null, build_key uuid not null,
 user_id uuid references auth.users(id) on delete cascade, primary key(guild_id,kind,build_key,user_id)
);
create table if not exists guild_city_audit (
 id bigint generated always as identity primary key, guild_id uuid references guilds(id) on delete cascade,
 char_name text not null, summary text not null, created_at timestamptz not null default now()
);
alter table guild_city_build_helpers enable row level security;
alter table guild_city_audit enable row level security;
create or replace function public.tf_city_audit() returns trigger language plpgsql security definer set search_path=public as $$
declare label text; who text; gid uuid;
begin
 select char_name into who from guild_members where user_id=auth.uid();
 if tg_table_name='guilds' then
  gid:=new.id;
  if new.hall_level<>old.hall_level then label:='Hall completed: level '||new.hall_level;
  elsif new.build_until is distinct from old.build_until and new.build_until is not null then label:='Hall construction timer updated';
  elsif new.recommended is distinct from old.recommended then label:='Research recommendation: '||coalesce(new.recommended,'cleared');
  elsif new.motd is distinct from old.motd then label:='Guild notice updated'; end if;
 else
  gid:=new.guild_id;
  if new.rank<>old.rank then label:='Rank changed: '||new.char_name||' to '||new.rank; end if;
 end if;
 if label is not null then insert into guild_city_audit(guild_id,char_name,summary) values(gid,coalesce(who,'Builders'),label); end if;
 return new;
end $$;
drop trigger if exists guild_city_guild_audit on guilds;
create trigger guild_city_guild_audit after update on guilds for each row execute function tf_city_audit();
drop trigger if exists guild_city_rank_audit on guild_members;
create trigger guild_city_rank_audit after update on guild_members for each row execute function tf_city_audit();
-- All writes and reads use authenticated functions scoped to guild membership.
alter table guild_city_stock enable row level security;
alter table guild_city_buildings enable row level security;
alter table guild_city_receipts enable row level security;
alter table guild_city_runs enable row level security;
alter table guild_city_participants enable row level security;
alter table guild_city_daily enable row level security;
alter table guild_city_battles enable row level security;
alter table guild_city_objectives enable row level security;
alter table guild_city_combatants enable row level security;

create or replace function public.tf_city_field(data text, key text) returns int language plpgsql immutable as $$
declare val text;
begin
 val := substring(data from ('(?m)^'||key||'=([0-9]+)$'));
 if val is null then return 0; end if;
 return least(val::bigint,2000000000)::int;
end $$;
create or replace function public.tf_city_set_field(data text, key text, val int) returns text language plpgsql immutable as $$
begin
 if data ~ ('(?m)^'||key||'=') then return regexp_replace(data,'(?m)^'||key||'=[^\n]*',key||'='||val); end if;
 return data||E'\n'||key||'='||val||E'\n';
end $$;
create or replace function public.tf_city_settle(gid uuid) returns void language plpgsql security definer set search_path=public as $$
begin
 update guild_city_buildings set level=level+1,until_at=null where guild_id=gid and until_at<=now();
 update guild_city_runs set finished=true,success=progress>=goal where guild_id=gid and (ends_at<=now() or (kind='boss' and progress>=goal));
end $$;
create or replace function public.tf_city_hub() returns jsonb language plpgsql security definer set search_path=public as $$
declare m guild_members; res jsonb;
begin
 select * into m from guild_members where user_id=auth.uid();
 if m.user_id is null then return null; end if;
 perform tf_city_settle(m.guild_id);
 select jsonb_build_object(
 'stock',coalesce((select to_jsonb(s)-'guild_id' from guild_city_stock s where guild_id=m.guild_id),'{}'),
 'buildings',coalesce((select jsonb_agg(jsonb_build_object('kind',kind,'level',level,'left',case when until_at is null then -1 else greatest(0,extract(epoch from until_at-now())::int) end)) from guild_city_buildings where guild_id=m.guild_id),'[]'),
 'ledger',coalesce((select jsonb_agg(to_jsonb(q)) from (select char_name,summary,created_at from (select char_name,result->>'summary' as summary,created_at from guild_city_receipts where guild_id=m.guild_id union all select char_name,summary,created_at from guild_city_audit where guild_id=m.guild_id) entries order by created_at desc limit 20) q),'[]'),
 'contracts',coalesce((select count from guild_city_daily where guild_id=m.guild_id and user_id=m.user_id and day=tf_day_key()),0),
 'runs',coalesce((select jsonb_agg(jsonb_build_object('id',r.id,'kind',r.kind,'goal',r.goal,'progress',r.progress,'left',greatest(0,extract(epoch from r.ends_at-now())::int),'finished',r.finished,'success',r.success,'joined',p.user_id is not null,'claimed',coalesce(p.claimed,false))) from guild_city_runs r left join guild_city_participants p on p.run_id=r.id and p.user_id=m.user_id where r.guild_id=m.guild_id and (not r.finished or r.ends_at>now()-interval '2 days')),'[]'),
 'battles',coalesce((select jsonb_agg(jsonb_build_object('id',b.id,'enemy',case when b.attacker=m.guild_id then b.defender else b.attacker end,'starts',extract(epoch from b.starts_at)::bigint,'ends',extract(epoch from b.ends_at)::bigint,'actions',coalesce(c.actions,0),'claimed',coalesce(c.claimed,false),'mine',coalesce((select sum(points) from guild_city_objectives where battle_id=b.id and guild_id=m.guild_id),0),'enemy_score',coalesce((select sum(points) from guild_city_objectives where battle_id=b.id and guild_id<>m.guild_id),0),'objectives',coalesce((select jsonb_agg(jsonb_build_object('kind',objective,'points',points)) from guild_city_objectives where battle_id=b.id and guild_id=m.guild_id),'[]'))) from guild_city_battles b left join guild_city_combatants c on c.battle_id=b.id and c.user_id=m.user_id where (b.attacker=m.guild_id or b.defender=m.guild_id) and b.ends_at>now()-interval '2 days'),'[]')
 ) into res;
 return res;
end $$;

create or replace function public.tf_city_action(p_action text,p_target text,p_request uuid,p_save text,p_stamp timestamptz) returns jsonb
language plpgsql security definer set search_path=public as $$
declare m guild_members; g guilds; sv saves; old guild_city_receipts; b guild_city_buildings; run guild_city_runs; part guild_city_participants;
 battle guild_city_battles; combat guild_city_combatants; st guild_city_stock; result jsonb; dw int:=0; do_ int:=0; dl int:=0; dg int:=0;
 gain_merit int:=0; cw int:=0; co int:=0; cl int:=0; cg int:=0; goal_ int; pts int; n int; tier int; cost int; secs int; mine bigint; enemy bigint; opp uuid; start_ timestamptz; end_ timestamptz; summary_ text; data_ text; stamp_ timestamptz; level_ int;
begin
 if auth.uid() is null or p_request is null then raise exception 'Sign in first.'; end if;
 select * into m from guild_members where user_id=auth.uid() for update;
 if m.user_id is null then raise exception 'You are not in a guild.'; end if;
 select * into old from guild_city_receipts where request_id=p_request;
 if found then
  if old.user_id<>m.user_id or old.guild_id<>m.guild_id or old.action<>p_action or old.target<>p_target then raise exception 'Receipt belongs to another action.'; end if;
  return old.result;
 end if;
 select * into g from guilds where id=m.guild_id for update;
 perform tf_hall_settle(g.id); select * into g from guilds where id=m.guild_id;
 perform tf_city_settle(g.id);
 perform pg_advisory_xact_lock(hashtext(m.user_id::text));
 select * into sv from saves where user_id=m.user_id for update;
 if sv.updated_at is distinct from p_stamp then raise exception 'Your cloud character changed. Sync it before contributing.'; end if;
 if p_save is null or length(p_save)>4000000 or p_save not like 'version=1%' then raise exception 'A current character save is required.'; end if;
 cw:=tf_city_field(p_save,'wood'); co:=tf_city_field(p_save,'ore'); cl:=tf_city_field(p_save,'leather'); cg:=tf_city_field(p_save,'gold');
 insert into guild_city_stock(guild_id) values(g.id) on conflict do nothing;
 select * into st from guild_city_stock where guild_id=g.id for update;
 if p_action='contribute' then
  if p_target='wood' then dw:=-20; elsif p_target='ore' then do_:=-20; elsif p_target='leather' then dl:=-20; elsif p_target='gold' then dg:=-100; else raise exception 'Unknown contribution.'; end if;
  update guild_city_stock set wood=wood-dw,ore=ore-do_,leather=leather-dl where guild_id=g.id;
  update guilds set funds=funds-dg where id=g.id;
  gain_merit:=10; summary_:='Deposited '||case when dg<0 then '100 gold' else '20 '||p_target end;
 elsif p_action='research' then
  level_:=coalesce((select level from guild_tech where guild_id=g.id and tech=p_target),0);
  cost:=case when p_target in ('coffers','arcane','expansion') then 40+20*level_ else 10+5*level_ end;
  if p_target in ('timber','builders','hands') then dw:=-cost;
  elsif p_target in ('veins','keen') then do_:=-cost;
  elsif p_target in ('tanner','ironhide') then dl:=-cost;
  elsif p_target in ('coffers','arcane','expansion') then dg:=-cost;
  else raise exception 'Unknown research.'; end if;
  level_:=tf_donate(p_target); summary_:='Research: '||p_target||' level '||level_;
 elsif p_action='foundation' then
  if g.hall_level>1 or g.build_until is not null then raise exception 'Foundation is already complete or building.'; end if;
  if p_target='timber' and g.project_timber<20 then dw:=-10; update guilds set project_timber=project_timber+1 where id=g.id;
  elsif p_target='ore' and g.project_ore<12 then do_:=-10; update guilds set project_ore=project_ore+1 where id=g.id;
  elsif p_target='tools' and g.project_tools<8 then dw:=-5; do_:=-5; update guilds set project_tools=project_tools+1 where id=g.id;
  elsif p_target='contracts' and g.project_contracts<3 then dg:=-50; update guilds set project_contracts=project_contracts+1 where id=g.id;
  else raise exception 'That foundation requirement is supplied or unknown.'; end if;
  gain_merit:=10; summary_:='Foundation: '||p_target;
 elsif p_action='upgrade' then
  if m.rank<1 then raise exception 'Only officers can start city upgrades.'; end if;
  if p_target not in ('workshop','storehouse','aid','command') then raise exception 'Unknown building.'; end if;
  tier:=case when p_target in ('workshop','storehouse') then 2 else 1 end;
  if g.hall_level<tier then raise exception 'This building needs Hall level %.',tier; end if;
  insert into guild_city_buildings(guild_id,kind) values(g.id,p_target) on conflict do nothing;
  select * into b from guild_city_buildings where guild_id=g.id and kind=p_target for update;
  if b.until_at is not null then raise exception 'This building is already upgrading.'; end if;
  if b.level>=least(5,g.hall_level) then raise exception 'Raise the Hall before upgrading this building again.'; end if;
  cost:=50*b.level; secs:=900*b.level;
  if st.wood<cost or st.ore<cost or g.funds<100*b.level then raise exception 'Upgrade needs % wood, % ore and % guild funds.',cost,cost,100*b.level; end if;
  update guild_city_stock set wood=wood-cost,ore=ore-cost where guild_id=g.id;
  update guilds set funds=funds-100*b.level where id=g.id;
  update guild_city_buildings set build_key=gen_random_uuid(),until_at=now()+make_interval(secs=>secs) where guild_id=g.id and kind=p_target;
  summary_:='Started '||p_target||' level '||(b.level+1);
 elsif p_action='help_build' then
  select * into b from guild_city_buildings where guild_id=g.id and kind=p_target for update;
  if b.until_at is null then raise exception 'No construction is running here.'; end if;
  if b.build_key is null then raise exception 'This construction queue needs to be restarted.'; end if;
  if exists(select 1 from guild_city_build_helpers where guild_id=g.id and kind=p_target and build_key=b.build_key and user_id=m.user_id) then raise exception 'You already helped this upgrade.'; end if;
  insert into guild_city_build_helpers values(g.id,p_target,b.build_key,m.user_id);
  level_:=coalesce((select level from guild_city_buildings where guild_id=g.id and kind='aid'),1);
  update guild_city_buildings set until_at=greatest(now(),until_at-make_interval(secs=>(45+5*(level_-1))*b.level)) where guild_id=g.id and kind=p_target;
  gain_merit:=5; summary_:='Helped build '||p_target;
 elsif p_action='contract' then
  if g.hall_level<2 then raise exception 'Contracts need Hall level 2.'; end if;
  insert into guild_city_daily(guild_id,user_id,day) values(g.id,m.user_id,tf_day_key()) on conflict do nothing;
  select count into n from guild_city_daily where guild_id=g.id and user_id=m.user_id and day=tf_day_key() for update;
  if n>=3 then raise exception 'All three daily contracts are complete.'; end if;
  if p_target='timber' then dw:=-20; elsif p_target='ore' then do_:=-15; elsif p_target='leather' then dl:=-15; else raise exception 'Unknown contract.'; end if;
  update guild_city_daily set count=count+1 where guild_id=g.id and user_id=m.user_id and day=tf_day_key();
  update guild_city_stock set wood=wood-dw,ore=ore-do_,leather=leather-dl where guild_id=g.id;
  level_:=coalesce((select level from guild_city_buildings where guild_id=g.id and kind='workshop'),1);
  gain_merit:=30+5*(level_-1); update guilds set funds=funds+25 where id=g.id;
  summary_:='Completed '||p_target||' contract';
 elsif p_action='launch' then
  if m.rank<1 then raise exception 'Only officers can launch guild activities.'; end if;
  if p_target not in ('expedition','boss') then raise exception 'Unknown activity.'; end if;
  if g.hall_level<2 then raise exception 'Activities need Hall level 2.'; end if;
  if exists(select 1 from guild_city_runs where guild_id=g.id and kind=p_target and not finished) then raise exception 'This activity is already running.'; end if;
  if exists(select 1 from guild_city_runs where guild_id=g.id and kind=p_target and starts_at::date=now()::date) then raise exception 'One of each activity per guild per day.'; end if;
  cost:=case when p_target='boss' then 100 else 50 end;
  if g.funds<cost then raise exception 'Not enough guild funds.'; end if;
  update guilds set funds=funds-cost where id=g.id;
  secs:=case when p_target='boss' then 3600 else 1800 end;
  goal_:=case when p_target='boss' then 200 else 3 end;
  insert into guild_city_runs(guild_id,kind,ends_at,goal) values(g.id,p_target,now()+make_interval(secs=>secs),goal_);
  summary_:='Launched '||p_target;
 elsif p_action in ('join_run','boss_hit','claim_run') then
  select * into run from guild_city_runs where id=p_target::bigint and guild_id=g.id for update;
  if run.id is null then raise exception 'Activity does not belong to your guild.'; end if;
  select * into part from guild_city_participants where run_id=run.id and user_id=m.user_id for update;
  if p_action='join_run' then
   if run.finished or run.ends_at<=now() then raise exception 'Activity has ended.'; end if;
   if part.user_id is not null then raise exception 'You already joined.'; end if;
   dg:=-25;
   insert into guild_city_participants(run_id,user_id,points) values(run.id,m.user_id,1);
   if run.kind='expedition' then update guild_city_runs set progress=progress+1 where id=run.id; end if;
   summary_:='Joined '||run.kind;
  elsif p_action='boss_hit' then
   if run.kind<>'boss' or run.finished or run.ends_at<=now() then raise exception 'No active guild boss.'; end if;
   if part.user_id is null then raise exception 'Join the raid first.'; end if;
   if part.last_hit is not null and part.last_hit>now()-interval '30 seconds' then raise exception 'The raid strike is cooling down.'; end if;
   if part.points>=101 then raise exception 'Your raid contribution is complete.'; end if;
   pts:=least(20,greatest(5,m.power/100));
   update guild_city_participants set points=points+pts,last_hit=now() where run_id=run.id and user_id=m.user_id;
   update guild_city_runs set progress=least(goal,progress+pts) where id=run.id;
   summary_:='Dungeon boss victory: +'||pts||' raid damage';
  else
   if not run.finished or not run.success then raise exception 'Rewards require a completed successful activity.'; end if;
   if part.user_id is null or part.claimed or (run.kind='boss' and part.points<=1) then raise exception 'No unclaimed participation reward.'; end if;
   update guild_city_participants set claimed=true where run_id=run.id and user_id=m.user_id;
   level_:=coalesce((select level from guild_city_buildings where guild_id=g.id and kind='storehouse'),1);
   dg:=case when run.kind='boss' then 150 else 75 end+10*(level_-1); dw:=10; do_:=10; gain_merit:=50;
   summary_:='Claimed '||run.kind||' reward';
  end if;
 elsif p_action='schedule_war' then
  if m.rank<1 then raise exception 'Only officers can schedule battles.'; end if;
  perform pg_advisory_xact_lock(68412715);
  opp:=p_target::uuid;
  if opp=g.id or not exists(select 1 from guild_wars where (guild_id=g.id and target_id=opp) or (guild_id=opp and target_id=g.id)) then raise exception 'Declare a war against this guild first.'; end if;
  -- Fixed Saturday 18:00 UTC, one-hour window; at least one hour of notice.
  start_:=(date_trunc('week',now() at time zone 'UTC') at time zone 'UTC')+interval '5 days 18 hours';
  if start_<now()+interval '1 hour' then start_:=start_+interval '7 days'; end if;
  if exists(select 1 from guild_city_battles where starts_at=start_ and (attacker in (g.id,opp) or defender in (g.id,opp))) then raise exception 'One scheduled battle per guild per window.'; end if;
  insert into guild_city_battles(attacker,defender,starts_at,ends_at) values(g.id,opp,start_,start_+interval '1 hour');
  summary_:='Scheduled war: Saturday 18:00 UTC';
 elsif p_action in ('war_gate','war_tower','war_keep','claim_war') then
  select * into battle from guild_city_battles where id=p_target::bigint and (attacker=g.id or defender=g.id) for update;
  if battle.id is null then raise exception 'This battle does not belong to your guild.'; end if;
  select * into combat from guild_city_combatants where battle_id=battle.id and user_id=m.user_id for update;
  if combat.user_id is not null and combat.guild_id<>g.id then raise exception 'You already fought for the other side.'; end if;
  if p_action='claim_war' then
   if now()<battle.ends_at then raise exception 'The battle has not ended.'; end if;
   if combat.user_id is null or combat.actions=0 or combat.claimed then raise exception 'No unclaimed battle reward.'; end if;
   select coalesce(sum(points),0) into mine from guild_city_objectives where battle_id=battle.id and guild_id=g.id;
   select coalesce(sum(points),0) into enemy from guild_city_objectives where battle_id=battle.id and guild_id<>g.id;
   dg:=case when mine>enemy then 200 when mine=enemy then 100 else 50 end; gain_merit:=case when mine>enemy then 80 else 40 end;
   update guild_city_combatants set claimed=true where battle_id=battle.id and user_id=m.user_id;
   summary_:='Battle reward: '||case when mine>enemy then 'victory' when mine=enemy then 'draw' else 'participation' end;
  else
   if now()<battle.starts_at or now()>=battle.ends_at then raise exception 'Battle is outside its scheduled window.'; end if;
   if coalesce(combat.actions,0)>=3 then raise exception 'Your three battle orders are complete.'; end if;
   opp:=case when battle.attacker=g.id then battle.defender else battle.attacker end;
   level_:=coalesce((select level from guild_city_buildings where guild_id=opp and kind='command'),1);
   if p_action='war_gate' then dw:=-15; pts:=30; elsif p_action='war_tower' then do_:=-15; pts:=40; else dl:=-15; pts:=50; end if;
   pts:=greatest(5,pts-(level_-1)*5);
   insert into guild_city_combatants(battle_id,user_id,guild_id,actions) values(battle.id,m.user_id,g.id,1) on conflict(battle_id,user_id) do update set actions=guild_city_combatants.actions+1;
   insert into guild_city_objectives(battle_id,guild_id,objective,points) values(battle.id,g.id,substring(p_action from 5),pts) on conflict(battle_id,guild_id,objective) do update set points=guild_city_objectives.points+excluded.points;
   summary_:='Battle order: '||substring(p_action from 5)||' +'||pts;
  end if;
 else raise exception 'Unknown city action.';
 end if;
 if cw+dw<0 or co+do_<0 or cl+dl<0 or cg+dg<0 then raise exception 'Not enough personal materials or gold.'; end if;
 if greatest(cw::bigint+dw,co::bigint+do_,cl::bigint+dl,cg::bigint+dg)>2000000000 then raise exception 'Resource capacity exceeded.'; end if;
 data_:=tf_city_set_field(tf_city_set_field(tf_city_set_field(tf_city_set_field(p_save,'wood',cw+dw),'ore',co+do_),'leather',cl+dl),'gold',cg+dg);
 data_:=regexp_replace(data_,'(?m)^guildCityReceipt=[^\n]*','');
 data_:=data_||E'\nguildCityReceipt='||p_request::text||E'\n';
 stamp_:=clock_timestamp();
 insert into saves(user_id,data,prev_data,updated_at) values(m.user_id,data_,sv.data,stamp_) on conflict(user_id) do update set data=excluded.data,prev_data=excluded.prev_data,updated_at=excluded.updated_at;
 update guild_members set merit=guild_members.merit+gain_merit,contrib=guild_members.contrib+greatest(gain_merit,0),contrib_week=tf_week_now() where user_id=m.user_id;
 result:=jsonb_build_object('request',p_request,'wood',dw,'ore',do_,'leather',dl,'gold',dg,'summary',summary_,'stamp',stamp_);
 insert into guild_city_receipts(request_id,guild_id,user_id,action,target,result,char_name) values(p_request,g.id,m.user_id,p_action,p_target,result,m.char_name);
 return result;
end $$;
-- The old free prototype orders are deliberately retired.
create or replace function public.tf_project_work(p_kind text,p_request uuid) returns text language plpgsql as $$
begin raise exception 'Update the game to deliver real construction materials.'; end $$;
revoke all on function tf_city_action(text,text,uuid,text,timestamptz),tf_city_hub(),tf_city_settle(uuid),tf_city_field(text,text),tf_city_set_field(text,text,int),tf_city_audit() from public,anon;
grant execute on function tf_city_action(text,text,uuid,text,timestamptz),tf_city_hub() to authenticated;
-- All cloud writes now compare the server revision under the same row lock.
create or replace function public.tf_save_commit(p_data text,p_stamp timestamptz) returns jsonb
language plpgsql security definer set search_path=public as $$
declare sv saves; stamp_ timestamptz; previous text;
begin
 if auth.uid() is null then raise exception 'Sign in first.'; end if;
 if p_data is null or length(p_data)>4000000 or p_data not like 'version=1%' then raise exception 'Invalid character save.'; end if;
 -- Serializes creation of the first save as well as updates.
 perform pg_advisory_xact_lock(hashtext(auth.uid()::text));
 select * into sv from saves where user_id=auth.uid() for update;
 if sv.updated_at is distinct from p_stamp then raise exception 'Your cloud character changed. Sync before uploading.'; end if;
 previous:=case when sv.updated_at<now()-interval '1 hour' then sv.data else sv.prev_data end;
 stamp_:=clock_timestamp();
 insert into saves(user_id,data,prev_data,updated_at) values(auth.uid(),p_data,previous,stamp_)
 on conflict(user_id) do update set data=excluded.data,prev_data=excluded.prev_data,updated_at=excluded.updated_at;
 return jsonb_build_object('data',p_data,'prev_data',previous,'updated_at',stamp_);
end $$;
revoke all on function tf_save_commit(text,timestamptz) from public,anon;
grant execute on function tf_save_commit(text,timestamptz) to authenticated;
drop policy if exists "players manage their own save" on saves;
create policy "players manage their own save" on saves for select using(auth.uid()=user_id);

revoke execute on function tf_donate(text) from authenticated;
