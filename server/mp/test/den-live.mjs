import assert from 'node:assert/strict';import {randomBytes} from 'node:crypto';
const base=process.env.MP_URL||'ws://127.0.0.1:8787';const sleep=ms=>new Promise(r=>setTimeout(r,ms));
async function until(test,label){for(let i=0;i<100;++i){const value=test();if(value)return value;await sleep(50)}throw Error('Timeout: '+label)}
async function open(name,zone='den',token=randomBytes(32).toString('hex')){const ws=new WebSocket(base+'/zone/'+zone),inbox=[];ws.onmessage=e=>inbox.push(JSON.parse(e.data));await until(()=>ws.readyState===1,'open');ws.send(JSON.stringify({t:'hello',name,look:'hero',token}));const welcome=await until(()=>inbox.find(x=>x.t==='welcome'),'welcome');if(zone==='den')await until(()=>inbox.find(x=>x.t==='den_state'),'den state');return {ws,inbox,id:welcome.id,send:m=>ws.send(JSON.stringify(m)),state:()=>inbox.filter(x=>x.t==='den_state').at(-1)}}
const a=await open('QA Alice'),b=await open('QA Bob');
try{
assert.equal(a.state().gold,500);assert.equal(b.state().gold,500);
assert.equal(a.state().ladder.version,1);assert.equal(a.state().ladder.stages.length,5);
a.send({t:'den_ladder',accessToken:'invalid',account:'forged'});await until(()=>a.inbox.some(x=>x.t==='den_error'),'invalid ladder auth');assert.equal(a.state().ladder.verified,false);
a.send({t:'den_fight',action:'strike'});await until(()=>a.inbox.some(x=>x.t==='den_error' && /Accept a duel/.test(x.text)),'unsolicited rejection');
for(let z=1200;z>=750;z-=50){a.send({t:'pos',x:650,z,yaw:0,mv:1});b.send({t:'pos',x:850,z,yaw:0,mv:1});await sleep(240)}
a.send({t:'den_challenge',target:b.id,stake:50});const offer=await until(()=>b.state().offers?.[0],'offer');assert.equal(a.state().gold,500);
b.send({t:'den_answer',offer:offer.id,accept:true});await until(()=>a.state().duel,'accepted');assert.equal(a.state().gold,450);assert.equal(b.state().gold,450);
await sleep(3200);assert.equal(a.state().spellsVersion,1);
a.send({t:'den_cast',spell:0});await until(()=>a.state().duel?.castA?.spell===0,'casting');assert.ok(a.state().duel.manaA<100);
await until(()=>b.state().duel?.hpB===96,'server spell damage');assert.equal(b.state().duel.castA,null);
b.send({t:'den_cast',spell:1});await until(()=>b.state().duel?.hpB===100,'server healing');
a.send({t:'den_fight',action:'surrender'});await until(()=>!b.state().duel&&b.state().gold===550,'payout');assert.equal(a.state().gold,450);
for(let i=1;i<=8;++i){b.send({t:'pos',x:850+Math.min(300,i*40),z:750-Math.min(100,i*15),yaw:0,mv:1});await sleep(240)}
const request={t:'den_roll',request:crypto.randomUUID(),stake:10,face:1,sequence:1};b.send(request);const roll=await until(()=>b.inbox.find(x=>x.t==='den_roll'),'roll');assert.equal(roll.gold,550-10+roll.payout);b.send(request);await until(()=>b.inbox.filter(x=>x.t==='den_roll').length===2,'replay');assert.equal(b.inbox.filter(x=>x.t==='den_roll').at(-1).gold,roll.gold);
a.send({t:'den_practice'});await until(()=>a.state().duel?.npc,'NPC practice');await until(()=>a.state().duel?.hpA<100,'NPC attacks');a.send({t:'den_fight',action:'surrender'});await until(()=>!a.state().duel,'practice end');assert.equal(a.state().gold,450);
console.log('PASS actual Worker sockets: consent, initial purse, escrow, surrender payout, casino and replay');
}finally{a.ws.close();b.ws.close()}

const token=randomBytes(32).toString('hex'),old=await open('Same browser','town0',token),current=await open('Same browser','town0',token);
try {await until(()=>old.ws.readyState===3,'replaced connection closes');assert.ok(!current.inbox.find(x=>x.t==='welcome').players.some(p=>p.id===old.id));console.log('PASS town reconnect replaces the previous browser identity');}finally{old.ws.close();current.ws.close()}
