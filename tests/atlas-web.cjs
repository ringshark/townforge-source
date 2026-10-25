const engines=require('playwright'),assert=require('assert'),fs=require('fs');
const {PNG}=require('pngjs');
const engine=process.env.TF_BROWSER||'chromium';
(async()=>{
 const browser=await engines[engine].launch({headless:true,args:engine==='chromium'?['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']:[]});
 try {
  for(const mobile of [false,true]){
   const context=await browser.newContext({viewport:mobile?{width:844,height:390}:{width:1280,height:720},hasTouch:mobile,isMobile:mobile,deviceScaleFactor:1,timezoneId:'America/New_York'});
   const page=await context.newPage(),errors=[];page.on('pageerror',e=>{errors.push(e.message);console.error(e.stack)});
   await page.goto(process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765',{waitUntil:'domcontentloaded'});
   await page.waitForFunction(()=>typeof _TF_AtlasState==='function' && typeof _TF_OutdoorState==='function' && JSON.parse(UTF8ToString(_TF_OutdoorState())).outdoor,null,{timeout:120000});
   await page.waitForTimeout(1500);await page.locator('#canvas').focus();
   const box=await page.locator('#canvas').boundingBox(),scale=Math.min(box.width/960,box.height/540);
   const pos=(x,y)=>({x:box.x+(box.width-960*scale)/2+x*scale,y:box.y+(box.height-540*scale)/2+y*scale});
   const click=async(x,y)=>{const p=pos(x,y);await page.mouse.click(p.x,p.y,{delay:140});await page.waitForTimeout(150)};
   const state=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_AtlasState())));
   const hero=()=>page.evaluate(()=>JSON.parse(UTF8ToString(_TF_OutdoorState())));
   await click(882,226);await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_AtlasState())).open);
   fs.mkdirSync('browser-checks',{recursive:true});const prefix=`browser-checks/atlas-${engine}-${mobile?'phone':'desktop'}`;
   await page.screenshot({path:prefix+'-overview.png'});
   async function mapColor(wx,wy){const p=pos(153+wx*412/5760-3,76+wy*412/5760-3);const im=PNG.sync.read(await page.screenshot({clip:{x:p.x,y:p.y,width:6*scale,height:6*scale}}));const rgb=[0,0,0];for(let i=0;i<im.data.length;i+=4)for(let c=0;c<3;c++)rgb[c]+=im.data[i+c];return rgb.map(v=>v/(im.data.length/4));}
   const snow=await mapColor(800,500),lake=await mapColor(2610,4572);
   console.log("ATLAS RASTER",await page.evaluate(()=>UTF8ToString(_TF_AtlasRasterState())),"screen",snow,lake);
   assert(snow[0]>165 && snow[1]>175,'northern snow renders as snow without corrupted horizontal bands');
   assert(lake[2]>lake[0]+20 && lake[1]>lake[0]+20,'Mirrormere renders as lake water');
   const before=await hero();await page.keyboard.down('w');await page.waitForTimeout(250);await page.keyboard.up('w');const after=await hero();
   assert(Math.hypot(after.x-before.x,after.y-before.y)<1,'map input never moves the hero');
   await click(718,38);await click(718,38);assert((await state()).zoom>1.7,'zoom buttons work');
   const view=await state(),a=pos(365,280),b=pos(445,300);await page.mouse.move(a.x,a.y);await page.mouse.down();await page.mouse.move(b.x,b.y,{steps:12});await page.mouse.up();await page.waitForTimeout(100);
   const panned=await state();assert(Math.hypot(panned.cx-view.cx,panned.cy-view.cy)>50,'drag pans the map');assert(panned.open,'drag never dismisses map');
   await click(827,96);assert.equal((await state()).filter,1,'supplies filter');
   await click(905,96);assert.equal((await state()).filter,2,'homes filter');
   await click(749,96);assert.equal((await state()).filter,0,'places filter');
   await click(824,334);assert((await state()).selected>=0,'nearby destination selected');
   await click(824,248);assert((await state()).waypoint>=0,'waypoint set');
   await page.screenshot({path:prefix+'-destination.png'});
   await click(884,38);assert(!(await state()).open,'close button returns to world');
   assert((await state()).waypoint>=0,'waypoint remains active in world');
   await page.screenshot({path:prefix+'-waypoint.png'});
   await click(882,226);await page.waitForFunction(()=>JSON.parse(UTF8ToString(_TF_AtlasState())).open);
   await click(824,498);assert.equal((await state()).waypoint,-1,'clear waypoint');
   await page.keyboard.press('Escape',{delay:150});await page.waitForTimeout(150);assert(!(await state()).open,'Escape closes atlas');
   assert.deepEqual(errors,[],'no map runtime errors');
   console.log(`PASS ${engine} ${mobile?'phone':'desktop'} atlas: overview, zoom, drag, filters, destination, waypoint, clear and close`);
   await context.close();
  }
 }finally{await browser.close()}
})().catch(e=>{console.error(e);process.exit(1)});
