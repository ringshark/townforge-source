// Real mouse/phone input, restored pet identity, complete art loading and dungeon map.
const {chromium,webkit}=require('playwright'),assert=require('assert'),fs=require('fs');
const origin=process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765';
const fixture=fs.readFileSync(process.env.TF_GRAPHICS_FIXTURE||'/tmp/tf-graphics-fixture.txt','utf8');
(async()=>{
 for(const spec of [{name:'desktop',engine:chromium,viewport:{width:1280,height:720}},{name:'phone',engine:webkit,viewport:{width:844,height:390},hasTouch:true,isMobile:true}]){
  const browser=await spec.engine.launch({headless:true,args:spec.name==='desktop'?['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']:[]});
  try{
   const context=await browser.newContext({viewport:spec.viewport,hasTouch:!!spec.hasTouch,isMobile:!!spec.isMobile,serviceWorkers:'block',deviceScaleFactor:1});
   const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   const ready=()=>page.waitForFunction(()=>typeof _TF_GraphicsState==='function'&&Module._TF_persistReady===1&&JSON.parse(UTF8ToString(_TF_OutdoorState())).outdoor&&JSON.parse(UTF8ToString(_TF_GraphicsState())).heroFrames>0&&document.querySelector('#loading').style.display==='none',null,{timeout:120000});
   const state=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_GraphicsState())));
   const outside=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_OutdoorState())));
   await page.goto(origin,{waitUntil:'domcontentloaded'});await ready();
   await page.evaluate(async save=>{Module._TF_blockSave=1;FS.writeFile('/persist/townforge_save.txt',save);await new Promise((resolve,reject)=>FS.syncfs(false,e=>e?reject(e):resolve()));},fixture);
   await page.reload({waitUntil:'domcontentloaded'});await ready();await page.locator('#canvas').focus();
   const restored=await state();assert.equal(restored.heroFrames,56);assert.equal(restored.northFrames,8);assert.equal(restored.furnitureFrames,60);assert.equal(restored.species,1,'renaming a pet preserves its exact wolf sprite');
   fs.mkdirSync('browser-checks',{recursive:true});await page.screenshot({path:`browser-checks/full-art-${spec.name}-town-and-pet.png`});
   // Walk the actual road out of Emberhold and into the Whisper Crypt. No state setter or teleport.
   async function walk(goal,timeout=18000){let held=new Set(),start=Date.now();try{
    while(Date.now()-start<timeout){if((await state()).dungeon)return;const p=await outside(),dx=goal.x-p.x,dy=goal.y-p.y;if(Math.hypot(dx,dy)<5)return;
     const next=new Set();if(Math.abs(dx)>2.5)next.add(dx>0?'d':'a');if(Math.abs(dy)>2.5)next.add(dy>0?'s':'w');
     for(const key of held)if(!next.has(key))await page.keyboard.up(key);for(const key of next)if(!held.has(key))await page.keyboard.down(key);held=next;await page.waitForTimeout(60);
    }
    throw Error('walking failed to reach '+JSON.stringify(goal)+' from '+JSON.stringify(await outside()));
   }finally{for(const key of held)await page.keyboard.up(key);}}
   const gate=await outside();console.log('Dungeon route start',spec.name,gate);await walk({x:gate.gx+90,y:gate.y});await walk({x:gate.gx+90,y:gate.gy-45});await walk({x:gate.gx,y:gate.gy-45});await walk({x:gate.gx,y:gate.gy+100});await walk({x:1620,y:3330});await walk({x:2520,y:3078});await walk({x:2970,y:3060});
   await page.screenshot({path:`browser-checks/full-art-${spec.name}-crypt-approach.png`});await walk({x:2970,y:2995});
   if(!(await state()).dungeon)await page.keyboard.press('e',{delay:200});
   await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_GraphicsState())).dungeon,null,{timeout:15000});await page.waitForTimeout(300);
   const entered=await state();assert.equal(entered.size,3600);assert.equal(entered.spawns,20);assert(Math.abs(entered.heroX-480)<2&&Math.abs(entered.heroY-270)<2,'dungeon camera centers the hero');
   await page.screenshot({path:`browser-checks/full-art-${spec.name}-dungeon-entry.png`});
   async function tap(x,y){const r=await page.locator('#canvas').boundingBox();x=r.x+x*r.width/960;y=r.y+y*r.height/540;
    if(spec.hasTouch){const touch=type=>page.evaluate(({type,x,y})=>{const c=document.querySelector('#canvas'),p={identifier:1,target:c,clientX:x,clientY:y,pageX:x,pageY:y};const e=new Event(type,{bubbles:true,cancelable:true});for(const k of ['touches','targetTouches','changedTouches'])Object.defineProperty(e,k,{value:type==='touchend'&&k!=='changedTouches'?[]:[p]});c.dispatchEvent(e);},{type,x,y});await touch('touchstart');await page.waitForTimeout(200);await touch('touchend');}
    else await page.mouse.click(x,y,{delay:200});await page.waitForTimeout(400);}
   await tap(850,184);await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_GraphicsState())).map);
   await page.screenshot({path:`browser-checks/full-art-${spec.name}-entire-dungeon-map.png`});const before=await state();
   await page.keyboard.down('w');await page.waitForTimeout(300);await page.keyboard.up('w');const after=await state();assert(Math.hypot(after.x-before.x,after.y-before.y)<1,'overview owns movement input');
   await tap(870,44);assert(!(await state()).map,'mouse/touch closes the overview');
   await page.keyboard.down('w');await page.waitForTimeout(500);await page.keyboard.up('w');const walked=await state();assert(walked.y<before.y-10,'movement resumes after closing map');
   assert.deepEqual(errors,[]);console.log(`PASS full graphics ${spec.name}: all hero/interior art loaded, renamed pet identity, actual gate/road/dungeon entry, 3600-unit exploration, 20 encounters, full map and camera/input ownership`);
   await context.close();
  }finally{await browser.close();}
 }
})().catch(e=>{console.error(e);process.exit(1)});
