// Packaged 2D build: assets, real input, native menu and daytime/nighttime frames.
const engines=require('playwright');
const engine=process.env.TF_BROWSER||'chromium';
const {PNG}=require('pngjs');
const assert=require('assert'),fs=require('fs');
const origin=process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765';
function changed(a,b){a=PNG.sync.read(a);b=PNG.sync.read(b);let n=0;for(let i=0;i<a.data.length;i+=16)if(Math.abs(a.data[i]-b.data[i])+Math.abs(a.data[i+1]-b.data[i+1])+Math.abs(a.data[i+2]-b.data[i+2])>65)n++;return n;}
(async()=>{
 const browser=await engines[engine].launch({headless:true,...(process.env.TF_CHROME?{executablePath:process.env.TF_CHROME}:{}),args:engine==='chromium'?['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']:[]});
 fs.mkdirSync('browser-checks',{recursive:true});
 try{
  for(const spec of [{name:'desktop-day',viewport:{width:1280,height:720},hour:13},{name:'phone-day',viewport:{width:844,height:390},hasTouch:true,isMobile:true,hour:13},{name:'desktop-night',viewport:{width:1280,height:720},hour:2}]){
   const {name,hour,...opts}=spec,context=await browser.newContext({...opts,timezoneId:'America/New_York',deviceScaleFactor:1,serviceWorkers:engine==='webkit'?'allow':'block'});
   const page=await context.newPage(),errors=[],art=new Set(),models=[];
   page.on('pageerror',e=>{errors.push(e.message);console.error('PAGEERROR',e.stack);});
   page.on('console',m=>{if(/\.gltf\] File loaded successfully/.test(m.text()))models.push(m.text());const hit=m.text().match(/assets\/town_polish\/([^\]]+)\] File loaded successfully/);if(hit)art.add(hit[1]);});
   await page.clock.setFixedTime(new Date(`2026-10-23T${String(hour).padStart(2,'0')}:00:00-04:00`));
   await page.goto(origin,{waitUntil:'domcontentloaded',timeout:120000});
   await page.waitForFunction(()=>{let e=document.querySelector('#loading');return e&&getComputedStyle(e).display==='none'},null,{timeout:120000});
   await page.waitForTimeout(6000);
   if(models.length)console.error('UNEXPECTED MODELS',JSON.stringify(models));
   await page.screenshot({path:`browser-checks/${name}-startup.png`});
   assert.deepEqual(models,[],'2D idle startup must not preload 3D models');
   assert.equal(art.size,18,'all eighteen building and scenery assets load');
   const canvas=page.locator('#canvas');await canvas.focus();
   const first=await page.screenshot({path:`browser-checks/${name}-town.png`});
   await page.keyboard.down('w');await page.waitForTimeout(1000);await page.keyboard.up('w');await page.waitForTimeout(300);
   const moved=await page.screenshot({path:`browser-checks/${name}-walk.png`});
   assert(changed(first,moved)>2000,'walking moves the hero/camera');
   // The render target stays 960x540 inside the physical canvas; map its letterbox.
   const rect=await canvas.boundingBox();
   const scale=Math.min(rect.width/960,rect.height/540);
   const map=PNG.sync.read(await page.screenshot({clip:{
       x:rect.x+(rect.width-960*scale)/2+830*scale,
       y:rect.y+(rect.height-540*scale)/2+174*scale,
       width:104*scale,height:104*scale}}));
   let mapHigh=0,mapLow=255;
   for(let i=0;i<map.data.length;i+=4){const v=Math.max(map.data[i],map.data[i+1],map.data[i+2]);mapHigh=Math.max(mapHigh,v);mapLow=Math.min(mapLow,v);}
   assert(mapHigh>90 && mapHigh-mapLow>35,'native landscape minimap contains terrain instead of a blank panel');
   const x=rect.x+(rect.width-960*scale)/2+75*scale;
   const y=rect.y+(rect.height-540*scale)/2+43*scale;
   if(spec.hasTouch && engine==='chromium'){const client=await context.newCDPSession(page);await client.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x,y,id:1}]});await page.waitForTimeout(150);await client.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});}
   else if(spec.hasTouch) {
       // WebKit has no CDP touch API. Hold a DOM touch across rendered frames,
       // matching a finger press rather than a zero-duration automated tap.
       const touch=async(type)=>page.evaluate(({type,x,y})=>{
           const canvas=document.querySelector('#canvas');
           const point={identifier:1,target:canvas,clientX:x,clientY:y,pageX:x,pageY:y,screenX:x,screenY:y};
           const event=new Event(type,{bubbles:true,cancelable:true});
           for(const key of ['touches','targetTouches','changedTouches'])Object.defineProperty(event,key,{value:type==='touchend' && key!=='changedTouches'?[]:[point]});
           canvas.dispatchEvent(event);
       },{type,x,y});
       await touch('touchstart');await page.waitForTimeout(150);await touch('touchend');
   }
   else await page.mouse.click(x,y,{delay:150});
   await page.waitForTimeout(500);
   const menu=await page.screenshot({path:`browser-checks/${name}-menu.png`});
   assert(changed(moved,menu)>6000,'MENU opens native navigation');
   assert.deepEqual(errors,[],'no browser runtime exceptions');
   const heap=await page.evaluate(()=>typeof HEAPU8!=='undefined'?HEAPU8.length:0);
   assert(heap>0 && heap<=256*1024*1024,'2D town startup heap stays at or below 256 MB');
   console.log('HEAP',heap);
   if(spec.hasTouch && engine==='webkit') {
       await page.reload({waitUntil:'domcontentloaded'});
       await page.waitForFunction(()=>document.querySelector('#loading').style.display==='none',null,{timeout:120000});
       await page.waitForTimeout(1000);
       assert.deepEqual(errors,[],'WebKit reload with service workers enabled');
   }
   const status=await page.evaluate(()=>{
       Module.setStatus('Loading again');
       const visible=getComputedStyle(document.querySelector('#loading')).display!=='none';
       Module.onAbort('test failure');Module.setStatus('');
       return {visible,errorVisible:getComputedStyle(document.querySelector('#loading')).display!=='none',text:document.querySelector('#loading').textContent};
   });
   assert(status.visible && status.errorVisible && /could not start/.test(status.text),'startup failures remain visible after the loader was hidden');
   console.log(`PASS ${name}: eighteen assets, movement, ${spec.hasTouch?'touch':'mouse'} menu and rendered scenery`);
   await context.close();
  }
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
