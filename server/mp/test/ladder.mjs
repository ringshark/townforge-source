import assert from 'node:assert/strict';
import {DenEngine,ladderWeek,LADDER_STAGES} from '../src/den-engine.js';
import {verifyLadderAccount} from '../src/ladder-auth.js';
let now=Date.UTC(2026,8,30,12);const account='11111111-1111-4111-8111-111111111111';
const player={id:'p',key:'browser-a',account,name:'QA Duelist',x:650,z:750};
let e=new DenEngine({},()=>now);e.peersFrom([player]);e.wallet(player.key);
const anonymous=new DenEngine({},()=>now);anonymous.peersFrom([{...player,account:null}]);assert.throws(()=>anonymous.practice('p',true),/Sign in/);
function win(){e.practice('p',true);const stage=e.state.duel.ladder.stage;assert.equal(e.state.duel.hpB,LADDER_STAGES[stage-1].hp);now+=3001;e.move('p',735,750,.6);for(let i=0;e.state.duel&&i<30;i++){e.fight('p','strike');now+=1000}assert.equal(e.state.duel,null)}
e.practice('p',true);now+=3001;e.fight('p','surrender');assert.equal(e.ladderSnapshot(player).cleared,0);assert.equal(e.ladderSnapshot(player).losses,1);
win();assert.equal(e.ladderSnapshot(player).cleared,1);e.finish('p','Knockout');assert.equal(e.ladderSnapshot(player).cleared,1);
// Another device, same verified account: resumes stage 2 with a separate test purse.
e=new DenEngine(structuredClone(e.state),()=>now);e.peersFrom([{...player,key:'browser-b'}]);assert.equal(e.ladderSnapshot(player).cleared,1);
win();win();assert.equal(e.ladderSnapshot(player).title,'Pit Contender');win();win();assert.equal(e.ladderSnapshot(player).title,'Blackwake Champion');assert.equal(e.ladderSnapshot(player).wins,5);assert.throws(()=>e.practice('p',true),/complete/);assert.equal(e.wallet('browser-a'),500);assert.equal(e.wallet('browser-b'),500);
const summary=e.snapshot('p');assert.equal(summary.ladder.leaders[0].cleared,5);assert.ok(!JSON.stringify(summary).includes(account));
now=ladderWeek(now)+7*86400000;assert.equal(e.ladderSnapshot(player).cleared,0);assert.equal(e.ladderSnapshot(player).title,'Blackwake Champion');assert.equal(e.ladderSnapshot(player).leaders.length,0);
// Free practice never advances the ladder. Countdown interruption is not a ranked loss.
e.practice('p');now+=4000;e.finish('p','Knockout');assert.equal(e.ladderSnapshot(player).cleared,0);
e.practice('p',true);e.leave('p');assert.equal(e.ladderSnapshot(player).losses,0);
// Match that crosses the reset cannot grant progress in the new week.
now=ladderWeek(now)+7*86400000-10000;e.practice('p',true);now+=20000;e.finish('p','Knockout');assert.equal(e.ladderSnapshot(player).cleared,0);
const env={SUPABASE_URL:'https://test-project.supabase.co',SUPABASE_ANON_KEY:'public-anon'};
let requestInfo;assert.equal(await verifyLadderAccount(env,'a.b.c',async(url,opts)=>{requestInfo={url,opts};return{ok:true,json:async()=>({id:account})}}),account);assert.match(requestInfo.url,/\/auth\/v1\/user$/);assert.equal(requestInfo.opts.headers.Authorization,'Bearer a.b.c');
await assert.rejects(verifyLadderAccount(env,'invalid'),/Sign in/);await assert.rejects(verifyLadderAccount(env,'a.b.c',async()=>({ok:false})),/expired/);await assert.rejects(verifyLadderAccount(env,'a.b.c',async()=>({ok:true,json:async()=>({id:'forged'})})),/verification failed/);await assert.rejects(verifyLadderAccount({},'a.b.c'),/not configured/);
// Harder opponents react to guard, use lunges and remain in bounds.
const ai=new DenEngine({},()=>now);ai.peersFrom([player]);ai.weekly().records[account]={name:'QA',cleared:4,wins:4,losses:0,clearMs:40000};ai.practice('p',true);now+=4000;ai.practiceTick();assert.ok(ai.state.duel.npc.x>=535&&ai.state.duel.npc.x<=965);assert.equal(ai.state.duel.npc.pace,185);
console.log('PASS ladder account verification, five ordered stages, knockout-only progress, cross-device persistence, titles, reset, free practice and bot profiles');
