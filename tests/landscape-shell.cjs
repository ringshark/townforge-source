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
// Tablets fit a proportionate landscape canvas inside either orientation.
for(const [width,height] of [[1024,768],[1180,820],[1366,1024],[768,1024],[820,1180],[600,768]]) {
  const tablet=fit(width,height,true);
  assert(tablet.canvasWidth<=tablet.width && tablet.canvasHeight<=tablet.height);
  near(tablet.canvasWidth/tablet.canvasHeight,16/9);
  assert.equal(tablet.rotated,height>width);
}
// Letterboxing offsets belong to the canvas bounds, not the whole tablet.
let tabletRect={left:0,top:96,width:1024,height:576};
p=point(512,384,tabletRect,false);near(p.x,480);near(p.y,270);
tabletRect={left:96,top:0,width:576,height:1024};
p=point(384,512,tabletRect,true);near(p.x,480);near(p.y,270);
console.log('PASS phone fullscreen, tablet fit, landscape startup and inverse touch rotation');
