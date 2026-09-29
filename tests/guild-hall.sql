-- Guild Hall scenario test (run against a scratch database that has web/supabase.sql
-- and an auth stub: auth.users + auth.uid() reading the tf.uid setting).
\set ON_ERROR_STOP 1
insert into auth.users values ('00000000-0000-0000-0000-00000000000a'), ('00000000-0000-0000-0000-00000000000b'),
                              ('00000000-0000-0000-0000-00000000000c'), ('00000000-0000-0000-0000-00000000000d')
  on conflict do nothing;
create or replace function pg_temp.as_user(c char) returns void language sql as $$
  select set_config('tf.uid', '00000000-0000-0000-0000-00000000000' || c, false); $$;
create or replace function pg_temp.expect_error(q text, frag text) returns void language plpgsql as $$
begin
  execute q;
  raise exception 'FAIL: expected an error containing "%" from: %', frag, q;
exception when others then
  if sqlerrm like 'FAIL:%' then raise; end if;
  if position(frag in sqlerrm) = 0 then raise exception 'FAIL: got "%" instead of "%" from: %', sqlerrm, frag, q; end if;
end $$;
create or replace function pg_temp.check(ok boolean, what text) returns void language plpgsql as $$
begin if not ok then raise exception 'FAIL: %', what; end if; raise notice 'ok  %', what; end $$;

-- A founds, B and C join
select pg_temp.as_user('a'); select tf_create_guild('Iron Wolves', 'IW', 'Aldric', 100) is not null;
select pg_temp.as_user('b'); select tf_join_guild((select id from guilds where name = 'Iron Wolves'), 'Brenna', 80);
select pg_temp.as_user('c'); select tf_join_guild((select id from guilds where name = 'Iron Wolves'), 'Corin', 60);
select pg_temp.check((select (tf_guild_hub()->>'hall')::int = 1), 'new guild starts at Hall 1');
select pg_temp.check((select (tf_guild_hub()->>'charges')::int = 20), 'members start with 20 donations');

-- research: tier gate, donations, recommended bonus, level up
select pg_temp.as_user('b');
select pg_temp.expect_error($$select tf_donate('keen')$$, 'Guild Hall must be level 3');
select pg_temp.expect_error($$select tf_donate('nonsense')$$, 'Unknown research');
select tf_donate('timber') from generate_series(1, 9);
select pg_temp.check((select level = 0 and progress = 90 from guild_tech where tech = 'timber'), '9 donations = 90/100 progress');
select pg_temp.as_user('a'); select tf_recommend('timber');
select pg_temp.as_user('b'); select pg_temp.check(tf_donate('timber') = 1, 'recommended donation levels timber to 1');
select pg_temp.check((select merit = 9*10 + 15 and contrib = 105 from guild_members where char_name = 'Brenna'), 'merit and weekly contribution credited');
select pg_temp.check((select funds = 105 from guilds where name = 'Iron Wolves'), 'guild funds credited');
select tf_donate('veins') from generate_series(1, 10);
select pg_temp.expect_error($$select tf_donate('veins')$$, 'No donations left');
select pg_temp.as_user('c');
select pg_temp.expect_error($$select tf_recommend('veins')$$, 'Only officers');

-- hall upgrade: rank + funds checks, lend a hand once, completion
select pg_temp.expect_error($$select tf_hall_upgrade()$$, 'Only officers');
select pg_temp.as_user('a'); select tf_set_rank('00000000-0000-0000-0000-00000000000b', 1);
update guilds set funds = 150;
select pg_temp.expect_error($$select tf_hall_upgrade()$$, 'needs 200 funds');
update guilds set funds = 500;
select pg_temp.as_user('b'); select tf_hall_upgrade();
select pg_temp.check((select funds = 300 and build_secs = 3600 from guilds), 'upgrade spends 200 funds, 1h build');
select pg_temp.expect_error($$select tf_hall_upgrade()$$, 'already being built');
select pg_temp.as_user('c'); select tf_lend_hand();
select pg_temp.expect_error($$select tf_lend_hand()$$, 'already lent a hand');
select pg_temp.check((select build_hands = 1 and build_until < now() + interval '3421 seconds' from guilds), 'a hand cuts 5% (180s)');
select pg_temp.check((select merit = 20 from guild_members where char_name = 'Corin'), 'lending a hand earns 20 merit');
select pg_temp.as_user('b'); select tf_lend_hand();
select pg_temp.check((select (tf_guild_hub()->>'lent')::boolean), 'the hub shows your hand lent');
select pg_temp.as_user('c');
select pg_temp.expect_error($$select tf_lend_hand()$$, 'already lent a hand');
select pg_temp.check((select (tf_guild_hub()->>'lent')::boolean), 'still lent after a guildmate lends too');
update guilds set build_until = now() - interval '1 second';
select pg_temp.check((select (tf_guild_hub()->>'hall')::int = 2), 'finished build raises the Hall to 2');
select pg_temp.check((select build_until is null from guilds), 'build cleared');
select pg_temp.check((select tf_member_cap(id) = 32 from guilds), 'Hall 2 holds 32 members');

-- help: one open request each, helpers once per request, no self-help, max helps, merit cap
select pg_temp.as_user('a'); select tf_request_help('Great Hall to level 4', 7200) > 0;
select pg_temp.as_user('a'); select pg_temp.check(tf_help_all() = 0, 'you cannot help yourself');
select pg_temp.as_user('b'); select pg_temp.check(tf_help_all() = 1, 'B helps A');
select pg_temp.check(tf_help_all() = 0, 'B cannot help the same request twice');
select pg_temp.as_user('c'); select pg_temp.check(tf_help_all() = 1, 'C helps A');
select pg_temp.as_user('a');
select pg_temp.check((select (h->>'n')::int = 2 and (h->>'max')::int = 7 from json_array_elements(tf_guild_hub()->'helps') h where (h->>'mine')::boolean), 'A sees 2 of 7 helps');
select tf_request_help('Mine to level 2', 600);
select pg_temp.check((select count(*) = 1 from guild_helps where open and user_id = '00000000-0000-0000-0000-00000000000a'), 'a new request replaces the old');
update guild_members set help_merit = 48, help_day = tf_day_key() where char_name = 'Brenna';
select pg_temp.as_user('b'); select merit from guild_members where char_name = 'Brenna' \gset before_
select tf_help_all();
select pg_temp.check((select merit = :before_merit + 2 from guild_members where char_name = 'Brenna'), 'help merit capped at 50 a day');

-- shop: server prices, can't overspend, unknown items refused
select pg_temp.as_user('c');
select pg_temp.expect_error($$select tf_shop_buy('crown')$$, 'not for sale');
select pg_temp.expect_error($$select tf_shop_buy('map')$$, 'Not enough merit');
update guild_members set merit = 100 where char_name = 'Corin';
select pg_temp.check(tf_shop_buy('bandages') = 40, 'bandages cost 60 merit (100 -> 40)');
select pg_temp.expect_error($$select tf_shop_buy('bandages')$$, 'Not enough merit');
select pg_temp.check((select merit >= 0 from guild_members where char_name = 'Corin'), 'merit never negative');

-- gifts: everyone gets them, opened once, daily send limit, newcomers don't inherit
select pg_temp.as_user('b'); select tf_send_gift(1);
select pg_temp.as_user('a');
select pg_temp.check((select json_array_length(tf_guild_hub()->'gifts') = 1), 'A has a gift waiting');
select pg_temp.check(tf_open_gifts() = '{1}', 'A opens a world-boss gift');
select pg_temp.check(tf_open_gifts() = '{}', 'opened gifts are gone');
select pg_temp.as_user('b'); select tf_send_gift(0) from generate_series(1, 15);
select pg_temp.check((select count(*) = 12 from guild_gifts where from_name = 'Brenna'), 'gift sending capped at 12 a day');
select pg_temp.expect_error($$select tf_send_gift(9)$$, 'Unknown gift');
select pg_temp.as_user('d'); select tf_join_guild((select id from guilds where name = 'Iron Wolves'), 'Dara', 10);
select pg_temp.check((select json_array_length(tf_guild_hub()->'gifts') = 0), 'a newcomer does not inherit old gifts');

-- leaving closes your help request; outsiders see nothing
select pg_temp.as_user('a'); select tf_request_help('Walls', 900);
select pg_temp.as_user('a'); select tf_leave_guild();
select pg_temp.check((select count(*) = 0 from guild_helps where open and user_id = '00000000-0000-0000-0000-00000000000a'), 'leaving closes your help request');
select pg_temp.check(tf_guild_hub() is null, 'no hub for someone outside a guild');
select pg_temp.expect_error($$select tf_donate('timber')$$, 'not in a guild');
\echo ALL GUILD HALL TESTS PASSED
