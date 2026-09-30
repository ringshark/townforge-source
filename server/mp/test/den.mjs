import assert from 'node:assert/strict';
import { DenEngine } from '../src/den-engine.js';
let now=10000;const a={id:'a',key:'key-a',name:'Alice',x:650,z:750},b={id:'b',key:'key-b',name:'Bob',x:850,z:750},c={id:'c',key:'key-c',name:'Watcher',x:700,z:800};
const e=new DenEngine({},()=>now,()=>6);e.peersFrom([a,b,c]);
assert.throws(()=>e.fight('a','strike'),/Accept/);assert.throws(()=>e.challenge('a','a',10),/another/);assert.throws(()=>e.challenge('a','b',NaN));
e.challenge('a','b',50);let id=e.state.offers[0].id;assert.equal(e.wallet(a.key),500);assert.throws(()=>e.answer('a',id,true),/challenged/);
e.answer('b',id,false);assert.equal(e.wallet(a.key),500);assert.equal(e.state.duel,null);
e.challenge('a','b',50);id=e.state.offers[0].id;e.answer('b',id,true);assert.equal(e.wallet(a.key),450);assert.equal(e.wallet(b.key),450);
assert.throws(()=>e.answer('b',id,true));assert.equal(e.wallet(a.key),450);assert.throws(()=>e.fight('a','strike'),/countdown/);assert.throws(()=>e.fight('c','strike'),/Accept/);
assert.deepEqual(e.move('a',1000,1000,.2),{x:650,z:750});
now+=3001;assert.throws(()=>e.fight('a','strike'),/closer/);e.move('a',735,750,.5);e.fight('b','guard');e.fight('a','strike');assert.equal(e.state.duel.hpB,95);assert.throws(()=>e.fight('a','strike'),/Recover/);
const at=e.move('a',50000,50000,.2);assert.ok(at.x<=965&&at.z<=965&&Math.hypot(at.x-735,at.z-750)<65);
e.leave('a');assert.equal(e.wallet(b.key),550);assert.equal(e.wallet(a.key),450);e.leave('a');assert.equal(e.wallet(b.key),550);
// Interrupted countdown refunds both deposits and expiry refunds an unresolved fight.
a.x=650;a.z=750;b.x=850;b.z=750;e.challenge('a','b',100);e.answer('b',e.state.offers[0].id,true);e.leave('b');assert.equal(e.wallet(a.key),450);assert.equal(e.wallet(b.key),550);
e.challenge('a','b',10);e.answer('b',e.state.offers[0].id,true);now+=184000;e.tick();assert.equal(e.wallet(a.key),450);assert.equal(e.wallet(b.key),550);
e.challenge('a','b',0);now+=31000;e.tick();assert.equal(e.state.offers.length,0);
// Casino verifies proximity, uses an atomic wallet, and replays only the outstanding UUID.
assert.throws(()=>e.casino('a','request-000000001',10,6,1),/entrance/);a.x=1150;a.z=650;
let r=e.casino('a','request-000000001',10,6,1);assert.equal(r.payout,50);assert.equal(e.wallet(a.key),490);
assert.deepEqual(e.casino('a','request-000000001',10,6,1),r);assert.equal(e.wallet(a.key),490);
e.casino('a','request-000000002',10,1,2);assert.equal(e.wallet(a.key),480);assert.throws(()=>e.casino('a','request-000000001',10,6,1),/state changed/);assert.equal(e.wallet(a.key),480);
const restored=new DenEngine(JSON.parse(JSON.stringify(e.state)),()=>now,()=>1);restored.peersFrom([a,b]);assert.equal(restored.snapshot('a').gold,480);assert.equal(restored.snapshot('a').rollSeq,2);
assert.throws(()=>restored.casino('a','request-000000003',51,1,3));assert.throws(()=>restored.casino('a','request-000000003',1,7,3));
console.log('PASS Den consent, no unsolicited damage, escrow/refunds, cooldown, range, movement, disconnect, deadlines, casino bounds/recovery and persistence');

{let now=100000;const game=new DenEngine({},()=>now);game.peersFrom([{id:'p',key:'practice-player',name:'Player',x:650,z:750}]);game.wallet('practice-player');game.practice('p');assert.equal(game.state.duel.stake,0);assert.equal(game.wallet('practice-player'),500);const saved=structuredClone(game.state);const restored=new DenEngine(saved,()=>now);restored.peersFrom([{id:'p',key:'practice-player',name:'Player',x:650,z:750}]);now+=4000;restored.practiceTick();assert.ok(restored.state.duel.npc.x<850);now+=1000;restored.practiceTick();assert.ok(restored.state.duel.hpA<100);restored.fight('p','surrender');assert.equal(restored.wallet('practice-player'),500);assert.equal(restored.state.duel,null);console.log('PASS free NPC practice, restored bot movement/combat and unchanged purse');}
