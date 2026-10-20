// Run against the packaged web build before deploying it.
const {chromium}=require('playwright');
const {PNG}=require('pngjs');
const fs=require('fs');
const assert=require('assert');
const origin=process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8765';
function nonblank(buffer){const p=PNG.sync.read(buffer),colors=new Set();for(let i=0;i<p.data.length;i+=64)colors.add((p.data[i]<<16)|(p.data[i+1]<<8)|p.data[i+2]);assert(colors.size>100,'Game frame must contain rendered scenery, not a blank canvas');}
function change(a,b){const x=PNG.sync.read(a),y=PNG.sync.read(b);let n=0,total=0;assert(x.width===y.width&&x.height===y.height);for(let i=0;i<x.data.length;i+=16){total++;if(Math.abs(x.data[i]-y.data[i])+Math.abs(x.data[i+1]-y.data[i+1])+Math.abs(x.data[i+2]-y.data[i+2])>60)n++;}return n/total;}
(async()=>{
 fs.mkdirSync('browser-checks',{recursive:true});
 const browser=await chromium.launch({headless:true,args:['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']});
 try{
  for(const spec of [
   {name:'desktop',viewport:{width:1280,height:720},hasTouch:false,isMobile:false},
   {name:'phone-landscape',viewport:{width:844,height:390},hasTouch:true,isMobile:true},
   {name:'phone-portrait',viewport:{width:390,height:844},hasTouch:true,isMobile:true}
  ]){
   const {name,...options}=spec;
   const context=await browser.newContext({...options,deviceScaleFactor:1,serviceWorkers:'block'});
   const page=await context.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   const assets=[];page.on('response',r=>{if(/townforge\.(wasm|data)(\?|$)/.test(r.url()))assets.push({url:r.url(),status:r.status()});});
   await page.goto(origin,{waitUntil:'domcontentloaded',timeout:120000});
   await page.waitForFunction(()=>{const el=document.getElementById('loading');return el&&getComputedStyle(el).display==='none';},{},{timeout:120000});
   await page.waitForTimeout(2500);
   assert(assets.length>=2&&assets.every(a=>a.status===200),'WASM and data load successfully');
   const canvas=page.locator('#canvas');await canvas.focus();
   async function position(x,y){const r=await canvas.boundingBox();assert(r);return spec.hasTouch&&spec.viewport.height>spec.viewport.width ? {x:r.x+(1-y/540)*r.width,y:r.y+x/960*r.height}:{x:r.x+x/960*r.width,y:r.y+y/540*r.height};}
   async function tap(x,y){const p=await position(x,y);if(spec.hasTouch)await page.touchscreen.tap(p.x,p.y);else await page.mouse.click(p.x,p.y);await page.waitForTimeout(350);}
   await tap(580,337); // Explore freely: use a fresh save for each context.
   const first=await page.screenshot({path:`browser-checks/${name}-first-view.png`});nonblank(first);
   await canvas.focus();await page.keyboard.press('v');await page.waitForTimeout(1500);
   const second=await page.screenshot({path:`browser-checks/${name}-other-view.png`});nonblank(second);
   assert(change(first,second)>.15,'Both 2D and 3D views render and the view switch changes the world');
   if(spec.hasTouch){
    const client=await context.newCDPSession(page),a=await position(75,440),b=await position(120,420);
    await client.send('Input.dispatchTouchEvent',{type:'touchStart',touchPoints:[{...a,id:1}]});
    await client.send('Input.dispatchTouchEvent',{type:'touchMove',touchPoints:[{...b,id:1}]});
    await page.waitForTimeout(700);await client.send('Input.dispatchTouchEvent',{type:'touchEnd',touchPoints:[]});
   }else{await page.keyboard.down('d');await page.waitForTimeout(700);await page.keyboard.up('d');}
   nonblank(await page.screenshot({path:`browser-checks/${name}-after-input.png`}));
   assert.deepStrictEqual(errors,[],'No browser runtime exceptions');
   console.log(`PASS ${name}: web assets, rendered 2D/3D views, view switching and ${spec.hasTouch?'touch':'keyboard'} input`);
   await context.close();
  }
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1);});
