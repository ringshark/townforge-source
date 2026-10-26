const {chromium,webkit}=require('playwright'),assert=require('assert'),fs=require('fs');
const fixture=fs.readFileSync(process.env.TF_INTERIOR_FIXTURE||'/tmp/tf-interior-fixture.txt','utf8');
(async()=>{
 for(const spec of [{name:'desktop',engine:chromium,viewport:{width:1280,height:720}},{name:'phone',engine:webkit,viewport:{width:844,height:390},isMobile:true,hasTouch:true}]){
  const browser=await spec.engine.launch({headless:true,args:spec.engine===chromium?['--no-sandbox','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']:[]});
  try{
   const context=await browser.newContext({...spec,engine:undefined,serviceWorkers:'block'}),page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   const ready=()=>page.waitForFunction(()=>typeof _TF_InteriorState==='function' && document.querySelector('#loading').style.display==='none',null,{timeout:120000});
   const state=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_InteriorState())));
   await page.goto(process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765',{waitUntil:'domcontentloaded'});await ready();
   await page.evaluate(async save=>{Module._TF_blockSave=1;FS.writeFile('/persist/townforge_save.txt',save);await new Promise((ok,no)=>FS.syncfs(false,e=>e?no(e):ok()));},fixture);
   await page.reload({waitUntil:'domcontentloaded'});await ready();await page.waitForTimeout(700);await page.locator('#canvas').focus();
   await page.keyboard.press('e',{delay:160});await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_InteriorState())).inside,null,{timeout:10000});await page.waitForTimeout(300);
   const first=await state();assert.equal(first.room,'smith');assert(first.wide && first.vw===960 && first.vh===540,'interior uses the full native landscape canvas');
   fs.mkdirSync('browser-checks',{recursive:true});await page.screenshot({path:`browser-checks/interior-${spec.name}-entrance.png`});
   const r=await page.locator('#canvas').boundingBox();await page.mouse.move(r.x+r.width*.5,r.y+r.height*.5);await page.mouse.wheel(0,700);await page.waitForTimeout(300);
   const scrolled=await state();assert.equal(scrolled.scroll,0,'room never turns into a scroll page');assert.equal(scrolled.x,first.x);assert.equal(scrolled.y,first.y);
   await page.keyboard.down('w');await page.waitForTimeout(700);await page.keyboard.up('w');const moved=await state();assert(moved.y<first.y-15,'room movement works');assert(moved.heroX>0&&moved.heroX<960&&moved.heroY>0&&moved.heroY<540,'camera keeps hero visible');
   await page.screenshot({path:`browser-checks/interior-${spec.name}-walking.png`});
   async function tap(x,y){const b=await page.locator('#canvas').boundingBox();x=b.x+x*b.width/960;y=b.y+y*b.height/540;
    if(spec.hasTouch){const touch=type=>page.evaluate(({type,x,y})=>{const c=document.querySelector('#canvas'),p={identifier:1,target:c,clientX:x,clientY:y,pageX:x,pageY:y};const e=new Event(type,{bubbles:true,cancelable:true});for(const k of ['touches','changedTouches','targetTouches'])Object.defineProperty(e,k,{value:type==='touchend'&&k!=='changedTouches'?[]:[p]});c.dispatchEvent(e);},{type,x,y});await touch('touchstart');await page.waitForTimeout(160);await touch('touchend');}
    else await page.mouse.click(x,y,{delay:160});await page.waitForTimeout(300);}
   await tap(75,43);assert((await state()).menu,'visible menu accepts pointer input');await tap(800,52);assert(!(await state()).menu,'return to room accepts pointer input');
   assert.deepEqual(errors,[]);console.log(`PASS interior ${spec.name}: real building entry, full landscape, no scrolling, camera follow, movement and menu touch routing`);await context.close();
  }finally{await browser.close();}
 }
})().catch(e=>{console.error(e);process.exit(1)});
