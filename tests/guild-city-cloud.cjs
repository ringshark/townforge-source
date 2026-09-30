const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const script=fs.readFileSync('web/cloud.js','utf8').replace('  init();','  window.__test={setup:function(c,u){sb=c;user=u;status="synced";},stamp:setStamp,upload:upload};');
function boot(storage,disk,client) {
 let syncCallback;
 const ctx={Date,Promise,crypto:{randomUUID:()=> '10000000-0000-0000-0000-000000000001'},localStorage:{getItem:k=>storage[k]||null,setItem:(k,v)=>storage[k]=v,removeItem:k=>delete storage[k]},document:{addEventListener:()=>{}},addEventListener:()=>{},Module:{_TF_persistReady:1},FS:{readFile:()=>disk.text,syncfs:(pull,cb)=>syncCallback=cb}};
 ctx.window=ctx;vm.createContext(ctx);vm.runInContext(script,ctx);ctx.__test.setup(client,{id:'u'});ctx.__test.stamp('2026-09-30T00:00:00Z');return {ctx,sync:()=>syncCallback(null)};
}
(async()=>{
 const storage={},disk={text:'version=1\nwood=100\nore=20\ngold=10\n'};let attempts=0,uploads=0,firstArgs;
 const result={request:'10000000-0000-0000-0000-000000000001',wood:-20,ore:0,leather:0,gold:0,summary:'Deposit',stamp:'2026-09-30T00:01:00Z'};
 const client={from:()=>({select:()=>({maybeSingle:async()=>({data:{data:disk.text,updated_at:'2026-09-30T00:00:00Z'}})})}),rpc:async(fn,args)=>{if(fn==='tf_save_commit'){uploads++;return {data:{data:args.p_data,updated_at:result.stamp}};}attempts++;if(!firstArgs)firstArgs=args;else assert.equal(args.p_request,firstArgs.p_request);if(attempts===1)throw Error('Response lost after server committed');return {data:result};}};
 let b=boot(storage,disk,client);await assert.rejects(()=>b.ctx.TFCloud.guildAction('contribute','wood'),/Response lost/);assert.ok(b.ctx.TFCloud.guildPending());await b.ctx.__test.upload(true);assert.equal(uploads,0);
 // Reload preserves exactly the same UUID and original inventory snapshot.
 b=boot(storage,disk,client);const recovered=await b.ctx.TFCloud.guildAction('retry','');assert.equal(recovered.request,result.request);assert.equal(firstArgs.p_save,disk.text);
 disk.text='version=1\nwood=80\nguildCityReceipt='+result.request+'\n';b.ctx.TFCloud.guildAck(result.request,result.stamp);assert.ok(b.ctx.TFCloud.guildPending());b.sync();assert.equal(b.ctx.TFCloud.guildPending(),false);assert.equal(storage['tf-cloud-stamp'],result.stamp);assert.equal(storage['tf-guild-city-pending'],undefined);
 // SQL failures have rolled back and release the reservation.
 const rejected={...client,rpc:async()=>({error:{message:'Not enough materials'}})};
 b=boot(storage,disk,rejected);await assert.rejects(()=>b.ctx.TFCloud.guildAction('contribute','wood'),e=>e.message.includes('Not enough'));assert.equal(b.ctx.TFCloud.guildPending(),false);
 console.log('PASS: lost-response recovery keeps UUID and snapshot; cloud upload exclusion; local save acknowledgement; database rejection releases reservation');
})().catch(e=>{console.error(e);process.exit(1)});
