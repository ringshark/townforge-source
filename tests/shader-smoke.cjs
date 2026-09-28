// Requires Playwright. Set PLAYWRIGHT_MODULE / BROWSER_EXECUTABLE when needed.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
(async()=>{
 const browser=await chromium.launch({headless:true,executablePath:process.env.BROWSER_EXECUTABLE||undefined,
 args:['--no-sandbox','--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader']});
 try {
 const page=await browser.newPage();
 const pairs=[['lit.vs','lit.fs'],['foliage.vs','lit.fs'],['grass.vs','lit.fs'],['lit.vs','ground.fs'],['torchlight.vs','torchlight.fs']];
 for(const [v,f] of pairs){
 const sources=[v,f].map(n=>fs.readFileSync(path.join(__dirname,'../assets/shaders',n),'utf8'));
 const result=await page.evaluate(([vs,fs])=>{
  const gl=document.createElement('canvas').getContext('webgl');if(!gl)throw Error('WebGL unavailable');
  const program=gl.createProgram();
  for(const [type,source] of [[gl.VERTEX_SHADER,vs],[gl.FRAGMENT_SHADER,fs]]){
   const shader=gl.createShader(type);gl.shaderSource(shader,source);gl.compileShader(shader);
   if(!gl.getShaderParameter(shader,gl.COMPILE_STATUS))return gl.getShaderInfoLog(shader);
   gl.attachShader(program,shader);
  }
  gl.linkProgram(program);const error=gl.getProgramParameter(program,gl.LINK_STATUS)?'':gl.getProgramInfoLog(program);
  gl.getExtension('WEBGL_lose_context')?.loseContext();return error;
 },sources);
 assert.equal(result,'',`${v} + ${f}: ${result}`);console.log(`PASS ${v} + ${f}`);
 }
 } finally {await browser.close()}
})().catch(e=>{console.error(e);process.exit(1)});
