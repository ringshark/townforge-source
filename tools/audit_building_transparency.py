"""Inspect decoded exterior assets, including framed and semi-opaque mattes.

Raw export mattes are reported separately from verified loader cleanup. The
native exterior-mattes.cpp test and browser GPU readback validate the loader.
"""
from pathlib import Path
from collections import deque
from PIL import Image
import json
root=Path(__file__).resolve().parents[1]
report=[]
for folder in ('buildings','town_buildings','saltmere_buildings','frostmere_buildings','cragmoor_buildings','town_polish'):
    for path in sorted((root/'assets'/folder).glob('*.png')):
        image=Image.open(path).convert('RGBA');w,h=image.size;pixels=image.load()
        framed=folder in ('frostmere_buildings','cragmoor_buildings')
        queue=deque([(x,y) for x in range(w) for y in (0,h-1)]+[(x,y) for y in range(h) for x in (0,w-1)])
        visited=set();matte=0;opaque=0
        while queue:
            x,y=queue.popleft()
            if not (0<=x<w and 0<=y<h) or (x,y) in visited:continue
            visited.add((x,y));r,g,b,a=pixels[x,y];lo=min(r,g,b);hi=max(r,g,b)
            allowed=(a<=112 or (lo>=210 and hi-lo<=55)) if framed else (a<=16 or (lo>=236 and hi-lo<=20))
            if not allowed:continue
            matte+=a>16;opaque+=a>240
            queue.extend(((x-1,y),(x+1,y),(x,y-1),(x,y+1)))
        suspected=matte>w*h*.01
        report.append({'asset':str(path.relative_to(root)),'raw_connected_matte_pixels':matte,
                       'raw_opaque_matte_pixels':opaque,'raw_background_suspected':suspected,
                       'uses_framed_matte_loader':framed,'unexpected_raw_background':suspected and not framed})
out=root/'docs/compact-interiors/building-transparency.json';out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(report,indent=2)+'\n')
failures=[r['asset'] for r in report if r['unexpected_raw_background']]
print(f"Inspected {len(report)} images; {sum(r['raw_background_suspected'] for r in report)} known framed mattes require loader cleanup; {len(failures)} unexpected mattes.")
if failures:raise SystemExit('\n'.join(failures))
