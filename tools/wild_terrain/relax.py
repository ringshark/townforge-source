import json, math, re
import numpy as np
P=json.load(open('wildpts.json'))
def dd(l):
    o=[]; [o.append(tuple(p)) for p in l if tuple(p) not in o]; return o
for k in P: P[k]=dd(P[k])
src=open('../../main.cpp').read()
cells={}
for m in re.finditer(r'\{\{(\d+),\s*(\d+)\},\s*(\d+),\s*\d+,\s*"[^"]+Plot"', src): cells[(int(m.group(1)),int(m.group(2)))]=int(m.group(3))
CIRC=[]; SQ=[]
def add(pts,r):
    for p in pts: CIRC.append((p[0],p[1],r))
for k in ['kWildernessGatherNodes','kWildernessCreatureSpots','kWildernessMonsterSpots','kWildernessInnocentSpots','kRivalCampSpots']: add(P[k],60)
add(P['kShrines'],70); add(P['kWildernessFoliage'],40)
ENTS=[(1650,1650),(1650,900),(2500,1100),(200,2100),(900,300),(1050,1900)]
GATES=[(900,1750),(2900,1750),(1400,640),(300,1050)]
add(ENTS,100); add(GATES,110)
for p in P['kHousePlots']: SQ.append((p[0],p[1],cells.get(tuple(p),7)*22.5+15))
CIRC.append((1900,2300,150)); CIRC.append((2750,2450,90))
C=np.array(CIRC,dtype=float); S=np.array(SQ,dtype=float)
def resample(pl,step):
    out=[pl[0]]
    for a,b in zip(pl,pl[1:]):
        L=math.hypot(b[0]-a[0],b[1]-a[1]); n=max(1,int(round(L/step)))
        for k in range(1,n+1): out.append((a[0]+(b[0]-a[0])*k/n,a[1]+(b[1]-a[1])*k/n))
    return out
def clearance(pts,hw):
    p=np.asarray(pts,dtype=float)
    d=np.hypot(p[:,None,0]-C[None,:,0],p[:,None,1]-C[None,:,1])-C[None,:,2]
    qx=np.abs(p[:,None,0]-S[None,:,0])-S[None,:,2]; qz=np.abs(p[:,None,1]-S[None,:,1])-S[None,:,2]
    ds=np.hypot(np.maximum(qx,0),np.maximum(qz,0))+np.minimum(np.maximum(qx,qz),0)
    return min(d.min(),ds.min())-hw
def relax(ctrl,hw,step=20,iters=500,pinned_ends=True):
    pl=np.array(resample(ctrl,step),dtype=float)
    lo=1 if pinned_ends else 0; hi=len(pl)-(1 if pinned_ends else 0)
    for it in range(iters):
        x=pl[lo:hi]
        dx=x[:,None,0]-C[None,:,0]; dz=x[:,None,1]-C[None,:,1]; d=np.hypot(dx,dz)+1e-6
        need=C[None,:,2]+hw; push=np.clip(need-d,0,None)*0.5
        mvx=(dx/d*push).sum(1); mvz=(dz/d*push).sum(1)
        # squares: push along the axis of least penetration
        qx=x[:,None,0]-S[None,:,0]; qz=x[:,None,1]-S[None,:,1]; h=S[None,:,2]+hw
        penx=h-np.abs(qx); penz=h-np.abs(qz); inside=(penx>0)&(penz>0)
        ax=inside&(penx<=penz); az=inside&(penz<penx)
        mvx+=(np.where(ax,np.sign(qx+1e-6)*penx*0.5,0)).sum(1); mvz+=(np.where(az,np.sign(qz+1e-6)*penz*0.5,0)).sum(1)
        pl[lo:hi,0]+=mvx; pl[lo:hi,1]+=mvz
        sm=pl.copy(); sm[1:-1]=pl[1:-1]*0.6+(pl[:-2]+pl[2:])*0.2
        if pinned_ends: sm[0]=pl[0]; sm[-1]=pl[-1]
        pl=sm
        if it%50==49: pl=np.array(resample([tuple(p) for p in pl],step))
        lo=1 if pinned_ends else 0; hi=len(pl)-(1 if pinned_ends else 0)
    return [tuple(round(float(v),1) for v in p) for p in pl], clearance(pl,hw)
