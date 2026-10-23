// Packaged 2D build: assets, real input, native menu and daytime/nighttime frames.
const {chromium}=require('playwright');
const {PNG}=require('pngjs');
const assert=require('assert'),fs=require('fs');
const origin=process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765';
function changed(a,b){a=PNG.sync.read(a);b=PNG.sync.read(b);let n=0;for(let i=0;i<a.data.length;i+=16)if(Math.abs(a.data[i]-b.data[i])+Math.abs(a.data[i+1]-b.data[i+1])+Math.abs(a.data[i+2]-b.data[i+2])>65)n++;return n;}
(async()=>{
 const browser=await chromium.launch({headless:true,...(process.env.TF_CHROME?{executablePath:process.env.TF_CHROME}:{}),args:['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']});
 fs.mkdirSync('browser-checks',{recursive:true});
 try{
  for(const spec of [{name:'desktop-day',viewport:{width:1280,height:720},hour:13},{name:'phone-day',viewport:{width:844,height:390},hasTouch:true,isMobile:true,hour:13},{name:'desktop-night',viewport:{width:1280,height:720},hour:2}]){
   const {name,hour,...opts}=spec,context=await browser.newContext({...opts,timezoneId:'America/New_York',deviceScaleFactor:1,serviceWorkers:'block'});
   const page=await context.newPage(),errors=[],art=new Set();
   page.on('pageerror',e=>errors.push(e.message));
   page.on('console',m=>{const hit=m.text().match(/assets\/town_polish\/([^\]]+)\] File loaded successfully/);if(hit)art.add(hit[1]);});
   await page.clock.setFixedTime(new Date(`2026-10-23T${String(hour).padStart(2,'0')}:00:00-04:00`));
   await page.goto(origin,{waitUntil:'domcontentloaded',timeout:120000});
   await page.waitForFunction(()=>{let e=document.querySelector('#loading');return e&&getComputedStyle(e).display==='none'},null,{timeout:120000});
   await page.waitForTimeout(2500);
   assert.equal(art.size,18,'all eighteen building and scenery assets load');
   const canvas=page.locator('#canvas');await canvas.focus();
   const first=await page.screenshot({path:`browser-checks/${name}-town.png`});
   await page.keyboard.down('w');await page.waitForTimeout(1000);await page.keyboard.up('w');await page.waitForTimeout(300);
   const moved=await page.screenshot({path:`browser-checks/${name}-walk.png`});
   assert(changed(first,moved)>2000,'walking moves the hero/camera');
   // The render target stays 960x540 inside the physical canvas; map its letterbox.
   const rect=await canvas.boundingBox();
   const scale=Math.min(rect.width/960,rect.height/540);
   const x=rect.x+(rect.width-960*scale)/2+75*scale;
   const y=rect.y+(rect.height-540*scale)/2+43*scale;
   if(spec.hasTouch){const client=await context.newCDPSession(page);await client.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{x,y,id:1}]});await page.waitForTimeout(150);await client.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});}
   else await page.mouse.click(x,y,{delay:150});
   await page.waitForTimeout(500);
   const menu=await page.screenshot({path:`browser-checks/${name}-menu.png`});
   assert(changed(moved,menu)>6000,'MENU opens native navigation');
   assert.deepEqual(errors,[],'no browser runtime exceptions');
   console.log(`PASS ${name}: eighteen assets, movement, ${spec.hasTouch?'touch':'mouse'} menu and rendered scenery`);
   await context.close();
  }
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
