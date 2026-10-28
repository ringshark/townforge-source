"""Audit exterior sprite silhouettes without mistaking white plaster for a matte."""
from pathlib import Path
from collections import deque
from PIL import Image
import json

root = Path(__file__).resolve().parents[1]
report = []
for folder in ('buildings', 'town_buildings', 'saltmere_buildings', 'frostmere_buildings', 'cragmoor_buildings', 'town_polish'):
    for path in sorted((root / 'assets' / folder).glob('*.png')):
        image = Image.open(path).convert('RGBA')
        w, h = image.size
        pixels = image.load()
        # Flood from every border pixel through transparent or near-white pixels.
        # A connected opaque white region larger than 1% suggests an exterior matte.
        border = {(x, y) for x in range(w) for y in (0, h-1)} | {(x, y) for y in range(h) for x in (0, w-1)}
        queue = deque(border)
        visited = set()
        white = 0
        while queue:
            x, y = queue.popleft()
            if (x, y) in visited or not (0 <= x < w and 0 <= y < h):
                continue
            visited.add((x,y))
            r,g,b,a = pixels[x,y]
            if a > 16 and min(r,g,b) < 236:
                continue
            if a > 240:
                white += 1
            queue.extend(((x-1,y),(x+1,y),(x,y-1),(x,y+1)))
        report.append({'asset': str(path.relative_to(root)), 'border_connected_white_pixels': white,
                       'white_background_suspected': white > w*h*0.01,
                       'transparent_border_pixels': sum(pixels[x,y][3] == 0 for x,y in border)})
output = root / 'docs' / 'cohesive-look' / 'building-transparency.json'
output.parent.mkdir(exist_ok=True)
output.write_text(json.dumps(report, indent=2)+'\n')
failures = [item['asset'] for item in report if item['white_background_suspected']]
print(f'Audited {len(report)} building assets; {len(failures)} suspected white backgrounds.')
if failures:
    raise SystemExit('\n'.join(failures))
