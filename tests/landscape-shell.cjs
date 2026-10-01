const assert=require('node:assert/strict');
const {fit,point,coordinates}=require('../web/landscape.js');
const portrait=fit(390,844,true);assert.equal(portrait.rotated,true);
assert.equal(portrait.canvasWidth,844);assert.equal(portrait.canvasHeight,390);
const rect={left:0,top:0,width:390,height:844};
const near=(a,b)=>assert(Math.abs(a-b)<.01);
let p=point(390,0,rect,true);near(p.x,0);near(p.y,0);
p=point(0,844,rect,true);near(p.x,960);near(p.y,540);
p=point(195,422,rect,true);near(p.x,480);near(p.y,270);
assert.equal(fit(844,390,true).rotated,false);
assert.equal(fit(844,390,true).canvasWidth,844);
assert.equal(fit(844,390,true).canvasHeight,390);
assert.equal(fit(390,844,false).rotated,false);
// Emscripten GLFW uses pageX/pageY for touch events, including scroll offsets.
for (const [x,y] of [[390,0],[0,844],[195,422]]) {
  const t=coordinates({clientX:x,clientY:y},rect,7,19);
  const native=point(x,y,rect,true);
  near((t.pageX-7-rect.left)*960/rect.width,native.x);
  near((t.pageY-19-rect.top)*540/rect.height,native.y);
  near(t.pageX,t.clientX+7);near(t.pageY,t.clientY+19);
}
console.log('PASS landscape startup and inverse touch rotation');
