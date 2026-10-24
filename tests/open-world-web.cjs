// Use real keyboard input and read-only diagnostics, without teleporting the hero.
const {chromium,webkit}=require('playwright');
const assert=require('assert'),fs=require('fs');
const engine=process.env.TF_BROWSER==='webkit'?webkit:chromium;
(async()=>{
 const browser=await engine.launch({headless:true,args:engine===chromium?['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']:[]});
 try{
 const context=await browser.newContext({viewport:{width:844,height:390},hasTouch:true,isMobile:true,timezoneId:'America/New_York'});
 const page=await context.newPage(),errors=[];
 page.on('pageerror',e=>{errors.push(e.message);console.error(e.stack)});
 await page.goto(process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765',{waitUntil:'domcontentloaded'});
 await page.waitForFunction(()=>typeof _TF_OutdoorState==='function' && JSON.parse(UTF8ToString(_TF_OutdoorState())).outdoor,null,{timeout:120000});
 const state=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_OutdoorState())));
 await page.locator('#canvas').focus();await page.waitForTimeout(1000);
 const initial=await state();assert.equal(initial.town,0,'starts in the connected Emberhold');
 await page.evaluate(()=>{
  window.worldSamples=[];
  window.worldSampler=setInterval(()=>worldSamples.push(JSON.parse(UTF8ToString(_TF_OutdoorState()))),16);
 });
 const world=(x,y)=>({x:initial.gx+x*initial.fy+y*initial.fx,y:initial.gy-x*initial.fx+y*initial.fy});
 async function walk(target){
  let held=new Set();const started=Date.now();
  try{
   while(Date.now()-started<18000){
    const p=await state(),dx=target.x-p.x,dy=target.y-p.y;
    if(Math.hypot(dx,dy)<9)return;
    const next=new Set();if(Math.abs(dx)>5)next.add(dx>0?'d':'a');if(Math.abs(dy)>5)next.add(dy>0?'s':'w');
    for(const key of held)if(!next.has(key))await page.keyboard.up(key);
    for(const key of next)if(!held.has(key))await page.keyboard.down(key);
    held=next;await page.waitForTimeout(60);
   }
   throw new Error('Walking stalled: '+JSON.stringify({target,position:await state()}));
  } finally {for(const key of held)await page.keyboard.up(key);}
 }
 fs.mkdirSync('browser-checks',{recursive:true});
 await page.screenshot({path:'browser-checks/open-world-town.png'});
 await walk(world(-81,-222));await walk(world(-81,-50));await walk(world(0,-50));
 await walk(world(0,100));assert.equal((await state()).town,-1,'walks into wilderness');
 await page.screenshot({path:'browser-checks/open-world-wilderness.png'});
 await walk(world(0,-50));assert.equal((await state()).town,0,'walks back into Emberhold');
 await page.screenshot({path:'browser-checks/open-world-return.png'});
 const samples=await page.evaluate(()=>{clearInterval(worldSampler);return worldSamples});
 assert(samples.length>20);
 for(let i=1;i<samples.length;i++) {
  assert(samples[i].outdoor,'gate crossing keeps the same outdoor screen');
  assert(Math.hypot(samples[i].x-samples[i-1].x,samples[i].y-samples[i-1].y)<35,'no coordinate jump or gate teleport');
 }
 assert.deepEqual(errors,[],'no runtime errors while crossing and returning');
 console.log('PASS phone continuous town/wilderness crossing and return, '+samples.length+' position samples');
 await context.close();
 } finally {await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
