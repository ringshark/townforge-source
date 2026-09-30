// Integration tests use a real PostgreSQL engine (PGlite), not mocked SQL.
const {PGlite}=require('@electric-sql/pglite'); const fs=require('node:fs');const assert=require('node:assert/strict');
(async()=>{
const db=new PGlite(); await db.exec(`create schema auth;create table auth.users(id uuid primary key);create role anon;create role authenticated;create function auth.uid() returns uuid language sql stable as $$select nullif(current_setting('tf.uid',true),'')::uuid$$;`);
await db.exec(fs.readFileSync('web/supabase.sql','utf8'));await db.exec(fs.readFileSync('web/guild-city.sql','utf8'));await db.exec(fs.readFileSync('web/guild-city.sql','utf8'));
const ids=['a','b','c','d'].map(c=>'00000000-0000-0000-0000-00000000000'+c);let user=0,seq=10;
async function as(i){user=i;await db.query(`select set_config('tf.uid',$1,false)`,[ids[i]]);}
async function q(sql,args=[]){return (await db.query(sql,args)).rows;}
async function bad(sql,args,fragment){try{await db.query(sql,args);assert.fail('Expected '+fragment);}catch(e){assert.ok(e.message.includes(fragment),e.message);}}
const snapshot='version=1\ncharacterName=Test\nwood=5000\nore=5000\nleather=5000\ngold=5000\n';
async function action(a,t,request){const rows=await q('select data,updated_at::text as updated_at from saves where user_id=$1',[ids[user]]);const sv=rows[0];return (await q('select tf_city_action($1,$2,$3,$4,$5) result',[a,t,request||'10000000-0000-0000-0000-'+String(++seq).padStart(12,'0'),sv?sv.data:snapshot,sv?sv.updated_at:null]))[0].result;}
for(const id of ids) await db.query('insert into auth.users values($1)',[id]);
await as(0);const gid=(await q("select tf_create_guild('Forgekeepers','TF','Leader',500) gid"))[0].gid;
await as(1);await db.query('select tf_join_guild($1,$2,$3)',[gid,'Member',500]);await as(2);await db.query('select tf_join_guild($1,$2,$3)',[gid,'Scout',500]);
await as(0);let r=await action('contribute','wood','10000000-0000-0000-0000-000000000001');assert.equal(r.wood,-20);
let replay=await q('select tf_city_action($1,$2,$3,$4,$5) r',['contribute','wood','10000000-0000-0000-0000-000000000001',snapshot,null]);assert.equal(replay[0].r.request,r.request);assert.equal((await q('select wood from guild_city_stock'))[0].wood,20);
await bad('select tf_city_action($1,$2,$3,$4,$5)',['contribute','ore','10000000-0000-0000-0000-000000009001',snapshot,null],'cloud character changed');
await as(1);await bad('select tf_city_action($1,$2,$3,$4,$5)',['contribute','wood',r.request,snapshot,null],'another action');
await action('foundation','timber');assert.equal((await q('select project_timber from guilds where id=$1',[gid]))[0].project_timber,1);
await assert.rejects(()=>action('upgrade','aid'),/Only officers/);
await as(0);await bad('select tf_project_work($1,$2)',['ore','10000000-0000-0000-0000-000000009003'],'Update the game');
await db.query('update guilds set hall_level=3,funds=2000 where id=$1',[gid]);await db.query('update guild_city_stock set wood=500,ore=500 where guild_id=$1',[gid]);
r=await action('research','timber');assert.equal(r.wood,-10);
await action('upgrade','workshop');let b=(await q("select * from guild_city_buildings where kind='workshop'"))[0];assert.equal(b.level,1);assert.ok(b.until_at);
await action('help_build','workshop');await bad('select tf_city_action($1,$2,$3,$4,$5)',['help_build','workshop','10000000-0000-0000-0000-000000009004',(await q('select data from saves where user_id=$1',[ids[0]]))[0].data,(await q('select updated_at::text as updated_at from saves where user_id=$1',[ids[0]]))[0].updated_at],'already helped');
await db.exec("update guild_city_buildings set until_at=now()-interval '1 second'");await q('select tf_city_hub()');assert.equal((await q("select level from guild_city_buildings where kind='workshop'"))[0].level,2);
r=await action('contract','timber');assert.equal(r.wood,-20);await action('contract','ore');await action('contract','leather');
await assert.rejects(()=>action('contract','timber'),/three daily contracts/);
const before=(await q('select merit from guild_members where user_id=$1',[ids[0]]))[0].merit;assert.ok(before>=105);
await action('launch','expedition');let exp=(await q("select id from guild_city_runs where kind='expedition'"))[0].id;
for(let i=0;i<3;i++){await as(i);await action('join_run',String(exp));}
await assert.rejects(()=>action('claim_run',String(exp)),/completed successful/);
await db.exec("update guild_city_runs set ends_at=now()-interval '1 second' where kind='expedition'");await q('select tf_city_hub()');r=await action('claim_run',String(exp));assert.equal(r.gold,75);await assert.rejects(()=>action('claim_run',String(exp)),/unclaimed/);
await as(0);await action('launch','boss');let boss=(await q("select id from guild_city_runs where kind='boss'"))[0].id;
await assert.rejects(()=>action('boss_hit',String(boss)),/Join the raid/);await action('join_run',String(boss));await action('boss_hit',String(boss));await assert.rejects(()=>action('boss_hit',String(boss)),/cooling down/);
await as(1);await action('join_run',String(boss));await db.query('update guild_city_runs set progress=goal where id=$1',[boss]);await q('select tf_city_hub()');await assert.rejects(()=>action('claim_run',String(boss)),/unclaimed participation/);
await as(0);r=await action('claim_run',String(boss));assert.equal(r.gold,150);
await as(3);const other=(await q("select tf_create_guild('Rivals','RV','Rival',500) gid"))[0].gid;await as(0);await db.query('select tf_declare_war($1)',[other]);await action('schedule_war',other);
let battle=(await q('select * from guild_city_battles'))[0];assert.equal(new Date(battle.starts_at).getUTCDay(),6);assert.equal(new Date(battle.starts_at).getUTCHours(),18);
await assert.rejects(()=>action('war_gate',String(battle.id)),/scheduled window/);
await db.exec("update guild_city_battles set starts_at=now()-interval '1 minute',ends_at=now()+interval '1 hour'");
await action('war_gate',String(battle.id));await action('war_tower',String(battle.id));await action('war_keep',String(battle.id));await assert.rejects(()=>action('war_gate',String(battle.id)),/three battle orders/);
await assert.rejects(()=>action('claim_war',String(battle.id)),/not ended/);
await db.exec("update guild_city_battles set ends_at=now()-interval '1 second'");r=await action('claim_war',String(battle.id));assert.equal(r.gold,200);await assert.rejects(()=>action('claim_war',String(battle.id)),/unclaimed battle/);
await as(3);await assert.rejects(()=>action('claim_war',String(battle.id)),/unclaimed battle/);
await as(0);const saveBefore=(await q('select data,updated_at::text as updated_at from saves where user_id=$1',[ids[0]]))[0];await q('select tf_save_commit($1,$2)',[saveBefore.data,saveBefore.updated_at]);await bad('select tf_save_commit($1,$2)',[saveBefore.data,saveBefore.updated_at],'cloud character changed');
await db.exec('set role anon');await bad('select tf_city_hub()',[],'permission denied');await db.exec('reset role');
assert.ok((await q('select count(*) n from guild_city_audit'))[0].n>0);
console.log('PASS: repeatable schema; inventory debit, CAS and replay; officer gates; construction and help; contract cap; expedition and raid participation; scheduled objectives and once-only rewards; anonymous denial; audit trail');await db.close();
})().catch(e=>{console.error(e.stack);process.exit(1)});
