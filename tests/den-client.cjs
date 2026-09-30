const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs'),{webcrypto}=require('node:crypto');
const saved=new Map(),sockets=[];let timer;
class Socket{constructor(){this.readyState=1;this.sent=[];sockets.push(this)}send(s){this.sent.push(JSON.parse(s))}close(){}message(m){this.onmessage({data:JSON.stringify(m)})}}
const ctx={window:{TF_MP_URL:'wss://example.test'},WebSocket:Socket,crypto:webcrypto,localStorage:{getItem:k=>saved.get(k)||null,setItem:(k,v)=>saved.set(k,v),removeItem:k=>saved.delete(k)},setTimeout:f=>(timer=f,1),clearTimeout:()=>{},Date,Math,Number,String,JSON,Uint8Array,Array};
vm.runInNewContext(fs.readFileSync('web/mpnet.js','utf8'),ctx);const client=ctx.window.TFMp;
client.zone('den','Alice','hero');let ws=sockets[0];ws.onopen();assert.match(ws.sent[0].token,/^[a-f0-9]{64}$/);let secret=ws.sent[0].token;
ws.message({t:'welcome',id:'a',players:[]});client.denAction('challenge','b',50);assert.ok(client.state().includes('needs its combat update'));
ws.message({t:'den_state',version:1,gold:500,offers:[],duel:null,rollSeq:0,results:[]});client.denAction('challenge','b',50);assert.deepEqual(ws.sent.at(-1),{t:'den_challenge',target:'b',stake:50});
client.denAction('roll','6',10);const request=ws.sent.at(-1);assert.equal(request.sequence,1);assert.ok(saved.has('tf-den-roll'));
client.denAction('roll','1',50);assert.deepEqual(ws.sent.at(-1),request); // no second unresolved wager
ws.onclose();timer();ws=sockets.at(-1);ws.onopen();assert.equal(ws.sent[0].token,secret);
ws.message({t:'welcome',id:'a2',players:[]});ws.message({t:'den_state',version:1,gold:540,offers:[],duel:null,rollSeq:1,results:[]});assert.deepEqual(ws.sent.at(-1),request);
ws.message({t:'den_roll',request:request.request,sequence:1,roll:6,face:6,stake:10,payout:50,gold:540});assert.ok(!saved.has('tf-den-roll'));assert.match(client.state(),/Returned 50/);
client.denAction('roll','2',25);const second=ws.sent.at(-1);assert.equal(second.sequence,2);
ws.message({t:'den_error',text:'Not enough gold',request:second.request,rejected:false});assert.ok(saved.has('tf-den-roll'));
ws.message({t:'den_error',text:'Not enough gold',request:second.request,rejected:true});assert.ok(!saved.has('tf-den-roll'));
console.log('PASS Den client capability gate, persistent purse, one pending roll, reconnect recovery, sequence and definitive rejection');

const stale=sockets[0];stale.message({t:'welcome',id:'stale',players:[{id:'ghost',name:'Alice'}]});assert.ok(!client.state().includes('ghost'));
ws.message({t:'join',p:{id:'a2',name:'Alice'}});assert.ok(!client.state().includes('p=a2|'));
client.denAction('practice','',0);assert.equal(ws.sent.at(-1).t,'den_practice');
console.log('PASS obsolete sockets ignored, self identity excluded and NPC practice action');
