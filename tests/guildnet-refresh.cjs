const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
async function run(failed) {
  const client = {
    rpc: async () => ({data: [{id:'g', name:'Guild', tag:'G', members:1, points:10}]}),
    from(table) {
      let membership = false;
      const q = {
        select(columns) { membership = columns === 'guild_id'; return q; },
        eq() { return q; }, or() { return q; }, maybeSingle() { return q; },
        then(resolve, reject) {
          let data = membership ? {guild_id:'g'} : table === 'guild_members' ?
            [{user_id:'u',char_name:'Mark',power:1,rank:2}] : table === 'guilds' ? {motd:'Hello'} : [];
          return Promise.resolve(!membership && table === failed ? {error:{message:'Unavailable: '+table}} : {data}).then(resolve,reject);
        }
      }; return q;
    }
  };
  const ctx = {Date, TFCloud:{client:()=>client,user:()=>({id:'u'}),configured:true}};
  ctx.window=ctx; vm.createContext(ctx);
  vm.runInContext(fs.readFileSync('web/guildnet.js','utf8'),ctx);
  ctx.TFGuildNet.refresh('week');
  await new Promise(setImmediate);
  const state=ctx.TFGuildNet.state();
  assert.match(state,/busy=0/);
  if(failed) assert.ok(state.includes('msg=Unavailable: '+failed),state);
  else { assert.match(state,/motd=Hello/); assert.match(state,/me=u\|2/); }
}
(async()=>{for(const table of [null,'guild_members','war_scores','guilds','guild_wars']) await run(table); console.log('PASS: guild refresh success and all four detail-query failures');})().catch(e=>{console.error(e);process.exit(1)});
