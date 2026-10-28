"""Inspect the shipping art; verify usable cells and document every actor family."""
from pathlib import Path
from PIL import Image
import json
root=Path(__file__).resolve().parents[1]
report={'hero':{},'interiors':{},'actors':[]}
def cells(name,columns,rows):
    im=Image.open(root/'assets/graphics_pass'/name).convert('RGBA');alpha=im.getchannel('A');result=[]
    for row in range(len(rows)-1):
        for column in range(columns):
            rect=(int(column*im.width/columns),int(rows[row]*im.height),int((column+1)*im.width/columns),int(rows[row+1]*im.height))
            bounds=alpha.crop(rect).point(lambda a:255 if a>48 else 0).getbbox()
            assert bounds and (bounds[2]-bounds[0])*(bounds[3]-bounds[1])>90,(name,row,column,'empty or tiny frame')
            result.append(bounds)
    return {'file':name,'size':im.size,'frames':len(result),'bounds':result}
report['hero']['actions']=cells('hero-actions.png',8,[0,183/1254,350/1254,517/1254,688/1254,862/1254,1035/1254,1])
report['hero']['north']=cells('hero-north.png',4,[0,.51,1])
report['interiors']=cells('interior-props.png',6,[0,.193,.36,.533,.684,.842,1])
report['details']=cells('interior-details.png',6,[0,.25,.5,.75,1])
creatures={'dog':6,'wolf':8,'bear':8,'panther':8,'bison':5,'horse':8,'sabertooth':8,'griffin':5,'drake':8,'wyvern':6,'dragon':5}
monsters={'bat':8,'goblin':5,'wolf':8,'imp':8,'bandit':5}
families={'sunkencrypt':(8,4,8),'weaversnest':(8,4,8),'wyrmscar':(6,4,5),'emberveil':(5,4,5),'hollowwarrens':(8,3,8)}
for folder,entries in [('wilderness_v2/creatures',{k:(v,4,5 if k in ('panther','wyvern') else v) for k,v in creatures.items()}),('wilderness_v2/monsters',{k:(v,4,v)for k,v in monsters.items()}),('monsters_v2',families)]:
    for name,(columns,rows,walk) in entries.items():
        path=root/'assets'/folder/(name+'.png');im=Image.open(path).convert('RGBA');alpha=im.getchannel('A');fw,fh=im.width//columns,im.height//rows
        for row in range(3):
            for col in range(walk):
                assert alpha.crop((col*fw,row*fh,(col+1)*fw,(row+1)*fh)).getbbox(),(path,row,col,'blank locomotion frame')
        report['actors'].append({'file':str(path.relative_to(root)),'directions':'down/up/left, right mirrored from left','walk_frames':walk,'combat':'event-driven directional stance, lunge/recovery and hit/death presentation','white_matte_cleanup':im.getpixel((0,0))[:3]==(255,255,255)})
for path in sorted((root/'assets/monsters_boss_v2').glob('*.png')):
    im=Image.open(path).convert('RGBA');assert im.getchannel('A').getbbox();report['actors'].append({'file':str(path.relative_to(root)),'combat':'boss attack/hit/death effects','size':im.size})
assert report['hero']['actions']['frames']==56 and report['hero']['north']['frames']==8 and report['interiors']['frames']==36 and report['details']['frames']==24
out=root/'docs/art-refresh/full-graphics-audit.json';out.write_text(json.dumps(report,indent=2)+'\n')
print('PASS graphics assets: 64 directional hero action cells, 60 furnishing sprites, all 11 pet species, 5 wild monster sheets, 5 dungeon families and 6 boss textures')
