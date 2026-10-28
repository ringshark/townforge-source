// Verify real GPU rendering, persisted mixed equipment and mouse/phone pose controls.
const {chromium,webkit}=require('playwright'),assert=require('assert'),fs=require('fs');
const origin=process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765',prefix=process.env.TF_EQUIPMENT_FIXTURE||'/tmp/tf-equipment-fixture';
(async()=>{
 for(const spec of [{name:'desktop',engine:chromium,viewport:{width:1280,height:720}},{name:'phone',engine:webkit,viewport:{width:844,height:390},hasTouch:true,isMobile:true}]){
  const browser=await spec.engine.launch({headless:true,args:spec.name==='desktop'?['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']:[]});
  try{
   const context=await browser.newContext({viewport:spec.viewport,hasTouch:!!spec.hasTouch,isMobile:!!spec.isMobile,serviceWorkers:'block',deviceScaleFactor:1}),page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   const state=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_EquipmentState())));
   const ready=()=>page.waitForFunction(()=>typeof _TF_EquipmentState==='function'&&Module._TF_persistReady===1&&JSON.parse(UTF8ToString(_TF_OutdoorState())).outdoor&&JSON.parse(UTF8ToString(_TF_EquipmentState())).draws>0&&document.querySelector('#loading').style.display==='none',null,{timeout:120000});
   await page.goto(origin,{waitUntil:'domcontentloaded'});await ready();fs.mkdirSync('browser-checks',{recursive:true});
   for(const [material,expected] of [['leather',[1,1,1,1,1,0]],['chain',[2,2,2,2,2,0]],['plate',[3,3,3,3,3,0]],['mixed',[3,1,0,0,2,0]]]){
    const save=fs.readFileSync(prefix+'-'+material+'.txt','utf8');
    await page.evaluate(async save=>{Module._TF_blockSave=1;FS.writeFile('/persist/townforge_save.txt',save);await new Promise((ok,no)=>FS.syncfs(false,e=>e?no(e):ok()));},save);
    await page.reload({waitUntil:'domcontentloaded'});await ready();await page.locator('#canvas').focus();
    const loaded=await state();assert.equal(loaded.parts,84);assert.deepEqual(loaded.materials,expected,'actual saved armor selects each separate body layer');assert.equal(loaded.weapon,material==='mixed'?4:1);
    await page.screenshot({path:`browser-checks/equipment-${spec.name}-${material}-world.png`});
   }
   async function tap(x,y){const r=await page.locator('#canvas').boundingBox(),scale=Math.min(r.width/960,r.height/540);x=r.x+(r.width-960*scale)/2+x*scale;y=r.y+(r.height-540*scale)/2+y*scale;
    if(spec.hasTouch){const touch=type=>page.evaluate(({type,x,y})=>{const c=document.querySelector('#canvas'),p={identifier:1,target:c,clientX:x,clientY:y,pageX:x,pageY:y};const e=new Event(type,{bubbles:true,cancelable:true});for(const k of ['touches','targetTouches','changedTouches'])Object.defineProperty(e,k,{value:type==='touchend'&&k!=='changedTouches'?[]:[p]});c.dispatchEvent(e);},{type,x,y});await touch('touchstart');await page.waitForTimeout(180);await touch('touchend');}
    else await page.mouse.click(x,y,{delay:180});await page.waitForTimeout(350);
   }
   await tap(75,43);await tap(160,180);await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_EquipmentState())).character);
   const modes=['Idle','Walk','Thrust','Cast','Chop','Mine','Fish'];
   for(let mode=0;mode<modes.length;mode++){
    if(mode){const s=await state();await tap(s.buttonX,s.buttonY);}
    await page.waitForFunction(mode=>JSON.parse(UTF8ToString(_TF_EquipmentState())).preview===mode,mode);
    const before=await state();assert.equal(before.action,modes[mode]);assert.deepEqual(before.materials,[3,1,0,0,2,0]);
    if(mode>0){await page.waitForTimeout(280);const after=await state();assert.notEqual(after.phase,before.phase,'preview visibly advances through the real animation');}
    const expectedTool=mode===4?5:mode===5?11:mode===6?12:4;assert.equal(before.weapon,expectedTool);
    await page.screenshot({path:`browser-checks/equipment-${spec.name}-pose-${mode}-${modes[mode]}.png`});
   }
   // A preview cannot alter the real equipped spear or armor in the save.
   const saved=await page.evaluate(()=>FS.readFile('/persist/townforge_save.txt',{encoding:'utf8'}));assert(saved.includes('Spear')&&saved.includes('Plate Mail')&&saved.includes('Leather'));
   assert.deepEqual(errors,[]);console.log(`PASS layered equipment ${spec.name}: all armor sets and mixed saved slots, 84 real sprite parts, world renderer, mouse/touch equipment entry, seven live poses and unchanged saved gear`);
   await context.close();
  }finally{await browser.close();}
 }
})().catch(e=>{console.error(e);process.exit(1)});
