// The Guild Hall data (tf_guild_hub) reaches the game as parseable state lines,
// and finished actions report back with a sequence number.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const hub = { hall: 2, funds: 340, cap: 32, build_left: 1200, build_secs: 7200, hands: 3, lent: true, rec: 'timber',
  merit: 85, charges: 14, next_charge: 400, contrib: 120, help_merit: 10,
  tech: [{ t: 'timber', l: 1, p: 30 }], helps: [{ id: 7, who: 'Brenna', what: 'Mine to level 3', n: 2, max: 7, mine: false, done: false }],
  gifts: [{ id: 9, k: 1, from: 'Corin' }], board: [{ n: 'Brenna', c: 120 }] };
const calls = [];
const client = {
  rpc: async (fn, args) => { calls.push(fn);
    if (fn === 'tf_guild_hub') return { data: hub };
    if (fn === 'tf_shop_buy') return { data: 25 };
    if (fn === 'tf_open_gifts') return { data: [1, 0] };
    return { data: [{ id: 'g', name: 'Guild', tag: 'G', members: 1, points: 10 }] }; },
  from(table) {
    let membership = false;
    const q = { select(c) { membership = c === 'guild_id'; return q; }, eq() { return q; }, or() { return q; }, maybeSingle() { return q; },
      then(res, rej) { const data = membership ? { guild_id: 'g' } : table === 'guild_members' ? [{ user_id: 'u', char_name: 'Mark', power: 1, rank: 2 }]
                         : table === 'guilds' ? { motd: 'Hi' } : []; return Promise.resolve({ data }).then(res, rej); } };
    return q; } };
const ctx = { Date, TFCloud: { client: () => client, user: () => ({ id: 'u' }), configured: true } };
ctx.window = ctx; vm.createContext(ctx);
vm.runInContext(fs.readFileSync('web/guildnet.js', 'utf8'), ctx);
(async () => {
  ctx.TFGuildNet.refresh('W1');
  await new Promise(setImmediate);
  let st = ctx.TFGuildNet.state();
  assert.ok(st.includes('hall=2|340|32|1200|7200|3|1|timber'), st);
  assert.ok(st.includes('mine=85|14|400|120|10'), st);
  assert.ok(st.includes('tech=timber|1|30'), st);
  assert.ok(st.includes('help=7|Brenna|Mine to level 3|2|7|0|0'), st);
  assert.ok(st.includes('gift=9|1|Corin'), st);
  assert.ok(st.includes('board=Brenna|120'), st);
  ctx.TFGuildNet.shopBuy('bandages');
  await new Promise(setImmediate); await new Promise(setImmediate);
  st = ctx.TFGuildNet.state();
  assert.ok(st.includes('done=1|bought|bandages'), st);
  ctx.TFGuildNet.openGifts();
  await new Promise(setImmediate); await new Promise(setImmediate);
  assert.ok(ctx.TFGuildNet.state().includes('done=2|opened|1,0'));
  console.log('PASS: guild hall hub state and action results');
})().catch(e => { console.error(e); process.exit(1); });
