// Server-owned test economy and consent-only combat. No character gold or gear.
export const PIT = { x: 750, z: 750, half: 250 };
export function secureRoll(sides) {
  const limit = Math.floor(4294967296 / sides) * sides;
  let n; do { n = crypto.getRandomValues(new Uint32Array(1))[0]; } while (n >= limit);
  return n % sides + 1;
}
const distance = (a,b) => Math.hypot(a.x-b.x,a.z-b.z);
const integer = (n,lo,hi) => Number.isSafeInteger(n) && n>=lo && n<=hi;
export class DenEngine {
  constructor(saved={},clock=()=>Date.now(),roll=secureRoll) {
    this.state={wallets:{},offers:[],duel:null,results:[],...saved};
    this.clock=clock;this.roll=roll;this.peers=new Map();
  }
  peersFrom(peers) {this.peers=new Map(peers.map(p=>[p.id,p]));}
  wallet(key) {return this.state.wallets[key] ?? (this.state.wallets[key]=500);}
  peer(id) {const p=this.peers.get(id);if(!p || !p.key) throw Error('Reconnect to Blackwake Den.');return p;}
  busy(id) {const d=this.state.duel;return !!d && (d.a===id || d.b===id);}
  inPit(p) {return Math.abs(p.x-PIT.x)<=PIT.half+100 && Math.abs(p.z-PIT.z)<=PIT.half+100;}
  snapshot(id) {
    const p=this.peer(id),d=this.state.duel;
    return {t:'den_state',version:1,gold:this.wallet(p.key),offers:this.state.offers.filter(o=>o.a===id || o.b===id),
      duel:d ? {id:d.id,a:d.a,b:d.b,nameA:d.nameA,nameB:d.nameB,hpA:d.hpA,hpB:d.hpB,staminaA:d.staminaA,staminaB:d.staminaB,
        starts:d.starts,ends:d.ends,stake:d.stake,guardA:d.guardA,guardB:d.guardB,swingA:d.swingA,swingB:d.swingB} : null,
      rollSeq:this.state.casino?.[p.key]?.sequence || 0,results:this.state.results.slice(-5)};
  }
  challenge(id,target,stake) {
    const a=this.peer(id),b=this.peer(target);this.tick();
    if(a.id===b.id || a.key===b.key) throw Error('Challenge another player.');
    if(!integer(stake,0,100)) throw Error('Choose a stake from 0 to 100 test gold.');
    if(this.state.duel) throw Error('The pit is occupied.');
    if(!this.inPit(a) || !this.inPit(b)) throw Error('Both players must stand near the dueling pit.');
    if(this.state.offers.some(o=>[o.a,o.b].some(x=>x===id || x===target))) throw Error('Finish the pending challenge first.');
    if(this.wallet(a.key)<stake || this.wallet(b.key)<stake) throw Error('Both players need enough test gold.');
    this.state.offers.push({id:crypto.randomUUID(),a:id,b:target,nameA:a.name,nameB:b.name,stake,expires:this.clock()+30000});
  }
  answer(id,offerId,accept) {
    this.tick();const o=this.state.offers.find(o=>o.id===offerId);
    if(!o || (o.a!==id && o.b!==id)) throw Error('This challenge has expired.');
    if(accept && o.b!==id) throw Error('Only the challenged player can accept.');
    this.state.offers=this.state.offers.filter(x=>x.id!==offerId);
    if(!accept) return;
    const a=this.peer(o.a),b=this.peer(o.b);
    if(this.state.duel || !this.inPit(a) || !this.inPit(b)) throw Error('Both players must remain near the empty pit.');
    if(this.wallet(a.key)<o.stake || this.wallet(b.key)<o.stake) throw Error('The agreed stake is no longer available.');
    this.state.wallets[a.key]-=o.stake;this.state.wallets[b.key]-=o.stake;
    this.state.duel={...o,keyA:a.key,keyB:b.key,hpA:100,hpB:100,staminaA:100,staminaB:100,
      starts:this.clock()+3000,ends:this.clock()+183000,lastTick:this.clock(),lastA:0,lastB:0,
      guardA:0,guardB:0,swingA:0,swingB:0,readyA:0,readyB:0};
    a.x=650;a.z=750;b.x=850;b.z=750;
    return {a:{id:a.id,x:a.x,z:a.z},b:{id:b.id,x:b.x,z:b.z}};
  }
  tick() {
    const now=this.clock();this.state.offers=this.state.offers.filter(o=>o.expires>now);
    const d=this.state.duel;if(!d) return;
    const dt=Math.max(0,Math.min(10,(now-d.lastTick)/1000));d.lastTick=now;
    d.staminaA=Math.min(100,d.staminaA+dt*12);d.staminaB=Math.min(100,d.staminaB+dt*12);
    if(now>=d.ends) this.finish(null,'Time limit: stakes refunded.');
  }
  move(id,x,z,elapsed) {
    const p=this.peer(id);if(!Number.isFinite(x) || !Number.isFinite(z)) return {x:p.x,z:p.z};
    const d=this.state.duel;let to={x:Math.max(120,Math.min(1380,x)),z:Math.max(120,Math.min(1380,z))};
    if(d && this.busy(id)) {
      if(this.clock()<d.starts) return {x:p.x,z:p.z};
      to.x=Math.max(PIT.x-PIT.half+35,Math.min(PIT.x+PIT.half-35,to.x));
      to.z=Math.max(PIT.z-PIT.half+35,Math.min(PIT.z+PIT.half-35,to.z));
    }
    const max=260*Math.max(0,Math.min(.6,elapsed))+12,dist=distance(p,to);
    if(dist>max) {to.x=p.x+(to.x-p.x)*max/dist;to.z=p.z+(to.z-p.z)*max/dist;}
    p.x=to.x;p.z=to.z;return to;
  }
  fight(id,action) {
    this.tick();const d=this.state.duel;if(!d || !this.busy(id)) throw Error('Accept a duel before fighting.');
    if(action==='surrender') {this.finish(id===d.a ? d.b : d.a,'Surrender');return;}
    const now=this.clock();if(now<d.starts) throw Error('Wait for the countdown.');
    if(!['strike','lunge','guard'].includes(action)) throw Error('Unknown arena action.');
    const side=id===d.a ? 'A':'B',other=side==='A' ? 'B':'A';
    const cooldown=action==='lunge' ? 1600:900,cost=action==='lunge' ? 30:action==='guard' ? 20:10;
    if(now<(d['ready'+side] || 0)) throw Error('Recover before your next action.');
    if(d['stamina'+side]<cost) throw Error('Not enough stamina.');
    const a=this.peer(id),b=this.peer(side==='A' ? d.b:d.a);
    if(action!=='guard' && distance(a,b)>(action==='lunge' ? 170:115)) throw Error('Move closer to your opponent.');
    d['last'+side]=now;d['ready'+side]=now+cooldown;d['stamina'+side]-=cost;
    if(action==='guard') {d['guard'+side]=now+1200;return;}
    d['swing'+side]=now;
    let damage=action==='lunge' ? 16:10;
    if(d['guard'+other]>now) {damage=Math.ceil(damage/2);d['guard'+other]=0;}
    d['hp'+other]=Math.max(0,d['hp'+other]-damage);
    if(d['hp'+other]===0) this.finish(id,'Knockout');
  }
  finish(winner,reason) {
    const d=this.state.duel;if(!d) return;
    if(winner===d.a) this.state.wallets[d.keyA]+=d.stake*2;
    else if(winner===d.b) this.state.wallets[d.keyB]+=d.stake*2;
    else {this.state.wallets[d.keyA]+=d.stake;this.state.wallets[d.keyB]+=d.stake;}
    this.state.results.push({id:d.id,winner:winner || '',name:winner===d.a ? d.nameA:winner===d.b ? d.nameB:'Draw',reason,stake:d.stake});
    this.state.results=this.state.results.slice(-20);this.state.duel=null;
  }
  leave(id) {
    this.state.offers=this.state.offers.filter(o=>o.a!==id && o.b!==id);
    const d=this.state.duel;if(!d || !this.busy(id)) return;
    if(this.clock()<d.starts) this.finish(null,'Countdown interrupted: stakes refunded.');
    else this.finish(id===d.a ? d.b:d.a,'Opponent disconnected');
  }
  casino(id,request,stake,face,sequence) {
    const p=this.peer(id);this.tick();
    if(this.busy(id)) throw Error('Finish your duel first.');
    if(distance(p,{x:1150,z:650})>190) throw Error('Walk to the casino entrance.');
    if(!/^[a-zA-Z0-9-]{16,64}$/.test(request)) throw Error('Invalid casino request.');
    const history=this.state.casino || (this.state.casino={});
    // One outstanding roll per wallet, sequenced by the server; a repeated UUID replays its result.
    const old=history[p.key];if(old && old.request===request) return old;
    if(sequence!==(old ? old.sequence+1:1)) throw Error('Casino state changed. Refresh before rolling.');
    if(!integer(stake,1,50) || !integer(face,1,6)) throw Error('Choose 1-50 test gold and a face from 1 to 6.');
    if(this.wallet(p.key)<stake) throw Error('Not enough test gold.');
    const roll=this.roll(6),payout=roll===face ? stake*5:0;
    this.state.wallets[p.key]+=payout-stake;
    const result={t:'den_roll',request,sequence,roll,face,stake,payout,gold:this.wallet(p.key)};
    history[p.key]=result;return result;
  }
}
