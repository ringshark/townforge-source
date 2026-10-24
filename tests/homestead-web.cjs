// A disposable restored save exercises the shipped Home/Farm UI through real
// pointer input. Read-only diagnostics assert economy and persistent state.
const {chromium,webkit}=require('playwright');
const assert=require('assert'),fs=require('fs');
const origin=process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765';
const fixture=fs.readFileSync(process.env.TF_HOMESTEAD_FIXTURE||'/tmp/tf-homestead-fixture.txt','utf8');
(async()=>{
 for(const spec of [{name:'desktop',engine:chromium,viewport:{width:1280,height:720}},{name:'phone',engine:webkit,viewport:{width:844,height:390},isMobile:true,hasTouch:true}]){
  const browser=await spec.engine.launch({headless:true,args:spec.engine===chromium?['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']:[]});
  try {
   const context=await browser.newContext({viewport:spec.viewport,isMobile:!!spec.isMobile,hasTouch:!!spec.hasTouch,serviceWorkers:'block'});
   const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   const ready=()=>page.waitForFunction(()=>typeof _TF_HomesteadState==='function' && document.querySelector('#loading').style.display==='none',null,{timeout:120000});
   const state=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_HomesteadState())));
   await page.goto(origin,{waitUntil:'domcontentloaded'});await ready();
   await page.evaluate(async(save)=>{Module._TF_blockSave=1;FS.writeFile('/persist/townforge_save.txt',save);await new Promise((resolve,reject)=>FS.syncfs(false,e=>e?reject(e):resolve()));},fixture);
   await page.reload({waitUntil:'domcontentloaded'});await ready();await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_HomesteadState())).farm,null,{timeout:120000});await page.waitForTimeout(1000);
   let initial=await state();assert(initial.farm && initial.seeds===6 && initial.produce===8,'restored farm data reaches the real game');
   async function tap(x,y){
    const r=await page.locator('#canvas').boundingBox(),scale=Math.min(r.width/960,r.height/540);
    x=r.x+(r.width-960*scale)/2+x*scale;y=r.y+(r.height-540*scale)/2+y*scale;
    if(spec.hasTouch){
     const touch=type=>page.evaluate(({type,x,y})=>{const canvas=document.querySelector('#canvas'),p={identifier:1,target:canvas,clientX:x,clientY:y,pageX:x,pageY:y,screenX:x,screenY:y};const e=new Event(type,{bubbles:true,cancelable:true});for(const k of ['touches','targetTouches','changedTouches'])Object.defineProperty(e,k,{value:type==='touchend'&&k!=='changedTouches'?[]:[p]});canvas.dispatchEvent(e);},{type,x,y});
     await touch('touchstart');await page.waitForTimeout(160);await touch('touchend');
    }else await page.mouse.click(x,y,{delay:160});
    await page.waitForTimeout(400);
   }
   await tap(75,43);await tap(800,178);await tap(860,134);
   assert((await state()).open,'Home opens the native farm page');
   fs.mkdirSync('browser-checks',{recursive:true});await page.screenshot({path:`browser-checks/homestead-${spec.name}-farm.png`});
   const before=await state();await tap(565,186);const bought=await state();assert.equal(bought.seeds,before.seeds+5);assert.equal(bought.gold,before.gold-15);
   await tap(710,186);const shipped=await state();assert.equal(shipped.produce,0);assert.equal(shipped.gold,bought.gold+64);
   // Observe the game's own autosave, flush it, and restore it in a fresh runtime.
   await page.waitForFunction(()=>{try{const save=FS.readFile('/persist/townforge_save.txt',{encoding:'utf8'});return /homesteadLife=/.test(save)&&JSON.parse(UTF8ToString(_TF_HomesteadState())).seeds===11&&save.includes(' 11 3 3 0 4 3 ');}catch{return false;}},null,{timeout:30000});
   await page.evaluate(()=>new Promise((resolve,reject)=>FS.syncfs(false,e=>e?reject(e):resolve())));
   await page.reload({waitUntil:'domcontentloaded'});await ready();await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_HomesteadState())).farm,null,{timeout:120000});const restored=await state();assert.equal(restored.seeds,11);assert.equal(restored.produce,0);assert.equal(restored.gold,shipped.gold);
   assert.deepEqual(errors,[]);console.log(`PASS homestead ${spec.name}: restored save, farm navigation, real ${spec.hasTouch?'touch':'mouse'} actions, seed/produce economy and reload persistence`);
   await context.close();
  }finally{await browser.close();}
 }
})().catch(e=>{console.error(e);process.exit(1)});
