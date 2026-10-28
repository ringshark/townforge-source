const {chromium}=require('playwright'),fs=require('fs'),assert=require('assert');
(async()=>{
const browser=await chromium.launch({headless:true,executablePath:'/usr/bin/chromium',args:['--no-sandbox','--disable-dev-shm-usage','--enable-unsafe-swiftshader','--use-gl=angle','--use-angle=swiftshader']});
const out=require('path').resolve(__dirname,'../docs/cohesive-look');const result={views:[],errors:[]};
for(const spec of [{name:'desktop',viewport:{width:1280,height:720}},{name:'phone',viewport:{width:844,height:390},hasTouch:true,isMobile:true}]){
 const context=await browser.newContext({...spec,serviceWorkers:'block',deviceScaleFactor:1});
 const page=await context.newPage();page.on('pageerror',e=>result.errors.push(e.message));
 await page.goto(process.env.TF_SMOKE_ORIGIN||'http://127.0.0.1:8000/townforge-current/review-site/',{waitUntil:'domcontentloaded',timeout:60000});
 await page.waitForFunction(()=>typeof _TF_ReviewScene==='function'&&Module._TF_persistReady===1&&typeof _TF_GraphicsState==='function'&&JSON.parse(UTF8ToString(_TF_GraphicsState())).heroFrames>0,{timeout:120000});
 await page.evaluate(()=>Module._TF_blockSave=1);
 for(const [scene,count,type] of [[0,4,'town'],[1,6,'dungeon'],[2,13,'interior']])for(let id=0;id<count;id++){
  await page.evaluate(([s,i])=>_TF_ReviewScene(s,i),[scene,id]);await page.waitForTimeout(200);
  const state=await page.evaluate(()=>({graphics:JSON.parse(UTF8ToString(_TF_GraphicsState())),inside:JSON.parse(UTF8ToString(_TF_InteriorState())),outdoor:JSON.parse(UTF8ToString(_TF_OutdoorState()))}));
  if(scene===0)assert(state.outdoor.outdoor && state.outdoor.town===id,JSON.stringify(state));
  if(scene===1)assert(state.graphics.dungeon,JSON.stringify(state));
  if(scene===2)assert(state.inside.inside&&state.inside.wide&&state.inside.vw===960&&state.inside.vh===540,JSON.stringify(state));
  const name=`${spec.name}-${type}-${id}`;await page.screenshot({path:`${out}/${name}.png`});result.views.push({name,...state});
 }
 // Real movement remains live in the native room after presentation changes.
 const start=await page.evaluate(()=>JSON.parse(UTF8ToString(_TF_InteriorState())));
 await page.locator('#canvas').focus();await page.keyboard.down('a');await page.waitForTimeout(350);await page.keyboard.up('a');
 const moved=await page.evaluate(()=>JSON.parse(UTF8ToString(_TF_InteriorState())));assert(Math.hypot(moved.x-start.x,moved.y-start.y)>5,'room movement');
 await context.close();console.log(`PASS ${spec.name}: 4 towns, 6 dungeons, 13 interiors and room movement`);
}
await browser.close();fs.writeFileSync(`${out}/browser-results.json`,JSON.stringify(result,null,2));assert.deepEqual(result.errors,[]);console.log('PASS 46 scene checks; no runtime exceptions');
})().catch(e=>{console.error(e);process.exitCode=1});
