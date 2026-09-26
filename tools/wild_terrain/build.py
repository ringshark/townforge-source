import json, math, os, sys
import numpy as np
exec(open('relax.py').read())
def catmull(pts, step=20):
    out=[]; P_=[pts[0]]+pts+[pts[-1]]
    for i in range(1,len(P_)-2):
        p0,p1,p2,p3=P_[i-1],P_[i],P_[i+1],P_[i+2]
        L=math.hypot(p2[0]-p1[0],p2[1]-p1[1]); n=max(2,int(L/step))
        for k in range(n):
            t=k/n; t2=t*t; t3=t2*t
            out.append(tuple(0.5*((2*p1[j])+(-p0[j]+p2[j])*t+(2*p0[j]-5*p1[j]+4*p2[j]-p3[j])*t2+(-p0[j]+3*p1[j]-3*p2[j]+p3[j])*t3) for j in (0,1)))
    out.append(pts[-1]); return out
exec(open('spec.py').read())   # SPEC, ROADCTRL, FORDS, LAKE, COAST params
CACHE='relaxed.json'
R=json.load(open(CACHE)) if os.path.exists(CACHE) else {}
for k,(ctrl,hw,pin) in SPEC.items():
    key=repr((ctrl,hw,pin))
    if R.get(k,{}).get('key')!=key:
        pl,cl=relax(catmull(ctrl,20),hw,pinned_ends=pin)
        R[k]={'key':key,'pl':pl,'cl':float(cl)}
    print('%-8s clearance %.1f pts %d'%(k,R[k]['cl'],len(R[k]['pl'])))
json.dump(R,open(CACHE,'w'))
ROADS=[catmull(c,20) for c in ROADCTRL]
# ---- raster
Sx=4.0; N=800
gx,gz=np.meshgrid(np.arange(N)*Sx+Sx/2,np.arange(N)*Sx+Sx/2)
def poly_dist(pl):
    pl=np.asarray(pl,float); best=np.full(gx.shape,1e9)
    for a,b in zip(pl[:-1],pl[1:]):
        vx,vz=b-a; L=vx*vx+vz*vz
        t=np.clip(((gx-a[0])*vx+(gz-a[1])*vz)/(L if L>0 else 1),0,1)
        best=np.minimum(best,np.hypot(gx-a[0]-vx*t,gz-a[1]-vz*t))
    return best
road=np.full(gx.shape,1e9)
for r in ROADS: road=np.minimum(road,poly_dist(r))
water=np.zeros(gx.shape,bool); riverm=np.zeros(gx.shape,bool)
def widths(pl,hw):
    p=np.asarray(pl,float)
    d=np.hypot(p[:,None,0]-C[None,:,0],p[:,None,1]-C[None,:,1])-C[None,:,2]
    qx=np.abs(p[:,None,0]-S[None,:,0])-S[None,:,2]; qz=np.abs(p[:,None,1]-S[None,:,1])-S[None,:,2]
    ds=np.hypot(np.maximum(qx,0),np.maximum(qz,0))+np.minimum(np.maximum(qx,qz),0)
    room=np.minimum(d.min(1),ds.min(1))-4
    w=np.clip(room,14,hw)
    for _ in range(6): w[1:-1]=np.minimum(w[1:-1],(w[:-2]+w[2:])/2+4)   # ease in/out of narrows
    return w
def poly_dist_w(pl,w):
    pl=np.asarray(pl,float); best=np.full(gx.shape,1e9)
    for i,(a,b) in enumerate(zip(pl[:-1],pl[1:])):
        vx,vz=b-a; L=vx*vx+vz*vz
        t=np.clip(((gx-a[0])*vx+(gz-a[1])*vz)/(L if L>0 else 1),0,1)
        dd=np.hypot(gx-a[0]-vx*t,gz-a[1]-vz*t)-(w[i]*(1-t)+w[i+1]*t)
        best=np.minimum(best,dd)
    return best
RW={}
for k,(ctrl,hw,pin) in SPEC.items():
    if k.startswith('w_'):
        RW[k]=widths(R[k]['pl'],hw)
        riverm|=poly_dist_w(R[k]['pl'],RW[k])<0
        print(k,'min width',round(float(RW[k].min()),1))
bridge=riverm&(road<26+12)
fordm=np.zeros(gx.shape,bool)
for f in FORDS: fordm|=np.hypot(gx-f[0],gz-f[1])<55
water|=riverm&~bridge&~fordm
lx,lz,la,lb=LAKE
ang=np.arctan2((gz-lz)/lb,(gx-lx)/la); rr=1+0.12*np.sin(3*ang)+0.07*np.sin(5*ang+1)
lakem=((gx-lx)/la)**2+((gz-lz)/lb)**2<rr*rr; water|=lakem
coastx=COAST(gz); water|=gx>coastx
ridgem=np.zeros(gx.shape,bool)
for k,(ctrl,hw,pin) in SPEC.items():
    if k.startswith('r_'): ridgem|=poly_dist(R[k]['pl'])<hw
blocked=water|ridgem|(gx<30)|(gz<30)|(gx>3170)|(gz>3170)
# ---- validation: flood fill from Emberhold gate, and clearance of gameplay points
from collections import deque
ok=~blocked; seen=np.zeros_like(ok); sx,sz=int(900/Sx),int(1750/Sx)
q=deque([(sz,sx)]); seen[sz,sx]=True
while q:
    z,x=q.popleft()
    for dz,dx in ((1,0),(-1,0),(0,1),(0,-1)):
        a,b=z+dz,x+dx
        if 0<=a<N and 0<=b<N and ok[a,b] and not seen[a,b]: seen[a,b]=True; q.append((a,b))
bad=[]; near=[]
allpts=[]
for k in ['kWildernessGatherNodes','kWildernessCreatureSpots','kWildernessMonsterSpots','kWildernessInnocentSpots','kShrines','kRivalCampSpots','kWildernessFoliage','kHousePlots','kSaltDocks']:
    for p in P[k]: allpts.append((k,p))
for p in ENTS: allpts.append(('ENT',p))
for p in GATES: allpts.append(('GATE',p))
allpts.append(('REFUGE',(2750,2450))); allpts.append(('SORROW',(1900,2300)))
for k,p in allpts:
    i,j=min(N-1,int(p[0]/Sx)),min(N-1,int(p[1]/Sx))
    if not seen[j,i]: bad.append((k,p))
    r=int((25 if k in ('kSaltDocks',) else 40)/Sx)
    if blocked[max(0,j-r):j+r+1,max(0,i-r):i+r+1].any() and k!='kSaltDocks': near.append((k,p))
print('UNREACHABLE',bad); print('NEAR',near)
# ---- image
from PIL import Image, ImageDraw, ImageFont
img=np.zeros((N,N,3),np.uint8)
reg=np.where(gz<700,0,np.where(gx<500,1,np.where(gx>1950,2,3)))
pal=np.array([(226,234,242),(150,146,132),(206,190,140),(118,156,92)],np.uint8)
img[:]=pal[reg]
img[road<22]=(214,190,136)
img[ridgem]=(96,86,76); img[water]=(40,92,124); img[bridge&~water&riverm]=(150,105,60)
im=Image.fromarray(img); d=ImageDraw.Draw(im)
font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',11)
cols={'kWildernessGatherNodes':(250,250,0),'kWildernessCreatureSpots':(0,200,255),'kWildernessMonsterSpots':(255,40,40),'kWildernessFoliage':(0,90,0),'kWildernessInnocentSpots':(255,255,255),'kShrines':(255,255,180),'kRivalCampSpots':(120,0,0),'kSaltDocks':(90,50,20)}
for k,c in cols.items():
    for p in P[k]:
        x,y=p[0]/Sx,p[1]/Sx; d.ellipse((x-4,y-4,x+4,y+4),fill=c,outline=(0,0,0))
for cx,cz,h in SQ: d.rectangle(((cx-h)/Sx,(cz-h)/Sx,(cx+h)/Sx,(cz+h)/Sx),outline=(255,150,0))
for i,p in enumerate(ENTS):
    x,y=p[0]/Sx,p[1]/Sx; d.rectangle((x-6,y-6,x+6,y+6),fill=(160,60,200),outline=(0,0,0)); d.text((x+7,y-6),'D%d'%i,fill=(0,0,0),font=font)
for p in GATES:
    x,y=p[0]/Sx,p[1]/Sx; d.rectangle((x-7,y-7,x+7,y+7),fill=(255,220,120),outline=(0,0,0))
d.ellipse(((1900-240)/Sx,(2300-240)/Sx,(1900+240)/Sx,(2300+240)/Sx),outline=(60,60,60))
for k,p in bad+near:
    x,y=p[0]/Sx,p[1]/Sx; d.ellipse((x-9,y-9,x+9,y+9),outline=(255,0,255),width=3)
im.save('map.png')
np.save('blocked.npy',blocked)
json.dump({k:{'pl':R[k]['pl'],'w':[round(float(x),1) for x in RW[k]]} for k in RW},open('rivers_out.json','w'))
