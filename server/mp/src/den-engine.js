// Server-owned test economy and consent-only combat. No character gold or gear.
export const LADDER_STAGES = [
  {name:'Dockhand Jory',style:'Close-range basics',hp:100,pace:100,damage:8,guardEvery:0,lunge:false},
  {name:'Cutlass Mira',style:'Fast lunges',hp:110,pace:130,damage:10,guardEvery:5,lunge:true},
  {name:'Bosun Rook',style:'Counters and guards',hp:120,pace:145,damage:11,guardEvery:4,lunge:true,counter:true},
  {name:'Captain Vale',style:'Spacing and pressure',hp:130,pace:165,damage:12,guardEvery:3,lunge:true,counter:true,kite:true},
  {name:'The Blackwake Champion',style:'Master of the pit',hp:145,pace:185,damage:14,guardEvery:3,lunge:true,counter:true,kite:true}
];
export function ladderWeek(now) {
  const date=new Date(now),day=(date.getUTCDay()+6)%7;
  return Date.UTC(date.getUTCFullYear(),date.getUTCMonth(),date.getUTCDate()-day);
}
// IDs match the character spellbook. Arena effects/costs are server-owned.
export const DEN_SPELLS = {
  0:{name:'Spark Dart',mana:4,damage:4,circle:1},
  1:{name:'Mending Word',mana:4,heal:4,circle:1},
  5:{name:'Wounding Touch',mana:6,damage:8,circle:2},
  7:{name:'Ember Burst',mana:9,damage:12,circle:3},
  8:{name:'Venom Sting',mana:9,damage:8,poison:true,circle:3},
  9:{name:'Greater Mending',mana:11,heal:16,circle:4},
  10:{name:'Storm Lance',mana:11,damage:16,circle:4},
  11:{name:'Psychic Shatter',mana:14,damage:20,circle:5},
  12:{name:'Arc Bolt',mana:20,damage:24,circle:6},
  13:{name:'Detonation',mana:20,damage:24,circle:6},
  14:{name:'Inferno Strike',mana:40,damage:28,circle:7},
  17:{name:'Grave Chill',mana:4,damage:5,circle:1},
  21:{name:"Sorrow's Lance",mana:12,damage:16,circle:4},
  27:{name:'Close Wounds',mana:8,heal:14,circle:1},
  29:{name:'Holy Light',mana:14,damage:18,circle:3},
  31:{name:'Cure',mana:6,cure:true,circle:2}
};
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
  peersFrom(peers) {this.peers=new Map(peers.map(p=>[p.id,p]));const d=this.state.duel;if(d?.npc)this.peers.set(d.b,{...d.npc,key:'training-npc'});}
  wallet(key) {return this.state.wallets[key] ?? (this.state.wallets[key]=500);}
  peer(id) {const p=this.peers.get(id);if(!p || !p.key) throw Error('Reconnect to Blackwake Den.');return p;}
  busy(id) {const d=this.state.duel;return !!d && (d.a===id || d.b===id);}
  inPit(p) {return Math.abs(p.x-PIT.x)<=PIT.half+100 && Math.abs(p.z-PIT.z)<=PIT.half+100;}
  snapshot(id) {
    const p=this.peer(id),d=this.state.duel;
    return {t:'den_state',version:1,spellsVersion:1,gold:this.wallet(p.key),offers:this.state.offers.filter(o=>o.a===id || o.b===id),
      duel:d ? {id:d.id,a:d.a,b:d.b,nameA:d.nameA,nameB:d.nameB,hpA:d.hpA,hpB:d.hpB,staminaA:d.staminaA,staminaB:d.staminaB,
        manaA:d.manaA ?? 100,manaB:d.manaB ?? 100,castA:d.castA || null,castB:d.castB || null,poisonA:d.poisonA || 0,poisonB:d.poisonB || 0,starts:d.starts,ends:d.ends,stake:d.stake,guardA:d.guardA,guardB:d.guardB,swingA:d.swingA,swingB:d.swingB,npc:d.npc || null} : null,
      ladder:this.ladderSnapshot(p),
      rollSeq:this.state.casino?.[p.key]?.sequence || 0,results:this.state.results.slice(-5)};
  }
  weekly() {
    const week=ladderWeek(this.clock());
    if(this.state.weekly?.week!==week)this.state.weekly={week,records:{}};
    return this.state.weekly;
  }
  ladderSnapshot(p) {
    const weekly=this.weekly(),record=p.account ? weekly.records[p.account]:null;
    const leaders=Object.values(weekly.records).filter(r=>r.cleared>0).sort((a,b)=>b.cleared-a.cleared || a.clearMs-b.clearMs || a.name.localeCompare(b.name)).slice(0,5).map(r=>({name:r.name,cleared:r.cleared,clearMs:r.clearMs}));
    return {version:1,verified:!!p.account,resetAt:weekly.week+7*86400000,cleared:record?.cleared || 0,wins:record?.wins || 0,losses:record?.losses || 0,
      title:p.account ? this.state.cosmetics?.[p.account] || '':'',leaders,stages:LADDER_STAGES.map(s=>({name:s.name,style:s.style,hp:s.hp}))};
  }
  practice(id,ranked=false) {
    this.tick();const a=this.peer(id);
    if(this.state.duel || this.state.offers.some(o=>o.a===id || o.b===id)) throw Error('Finish the pending match or challenge first.');
    if(!this.inPit(a)) throw Error('Walk to the pit before starting practice.');
    let stage=0,weekly;
    if(ranked) {
      if(!a.account)throw Error('Sign in with Cloud save for the weekly ladder.');
      weekly=this.weekly();stage=(weekly.records[a.account]?.cleared || 0)+1;
      if(stage>LADDER_STAGES.length)throw Error('This week is complete. Practice or return after the reset.');
    }
    const profile=ranked ? LADDER_STAGES[stage-1]:{name:'Captain Vale (practice)',hp:100,pace:125,damage:10,guardEvery:4,lunge:false};
    const bot={id:'pit-trainer',name:profile.name,key:'training-npc',x:850,z:750,yaw:Math.PI};
    this.peers.set(bot.id,bot);this.wallet(bot.key);this.challenge(id,bot.id,0);
    const placement=this.answer(bot.id,this.state.offers.find(o=>o.a===id).id,true);
    this.state.duel.npc={...profile,id:bot.id,x:850,z:750,yaw:Math.PI,stage};
    this.state.duel.hpB=profile.hp;this.state.duel.botTick=this.clock();
    if(ranked)this.state.duel.ladder={account:a.account,week:weekly.week,stage};
    return placement;
  }
  practiceTick() {
    const d=this.state.duel;if(!d?.npc || this.clock()<d.starts)return;
    const a=this.peers.get(d.a);if(!a) {this.leave(d.a);return;}
    const b=this.peer(d.b),bot=d.npc,dt=Math.max(0,Math.min(1,(this.clock()-d.botTick)/1000));d.botTick=this.clock();
    const dist=distance(a,b),desired=bot.kite && d.guardA>this.clock() ? 160:85;
    const step=Math.max(-55*dt,Math.min(dist-desired,(bot.pace || 125)*dt));
    if(dist>0){b.x=Math.max(535,Math.min(965,b.x+(a.x-b.x)*step/dist));b.z=Math.max(535,Math.min(965,b.z+(a.z-b.z)*step/dist));b.yaw=Math.atan2(a.z-b.z,a.x-b.x);}
    Object.assign(bot,{x:b.x,z:b.z,yaw:b.yaw});
    if(this.clock()>=(d.readyB || 0)) {
      const close=distance(a,b),counter=bot.counter && this.clock()-d.swingA<1200 && this.clock()-d.lastB>2000;
      const guard=bot.guardEvery && (counter || Math.floor(this.clock()/1000)%bot.guardEvery===0);
      const action=guard && d.staminaB>=20 ? 'guard':bot.lunge && close>110 && close<=170 ? 'lunge':close<=115 ? 'strike':'';
      if(action)try {this.fight(d.b,action);}catch(e) {}
    }
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
    this.state.duel={...o,keyA:a.key,keyB:b.key,hpA:100,hpB:100,staminaA:100,staminaB:100,manaA:100,manaB:100,castA:null,castB:null,poisonA:0,poisonB:0,
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
    if(now>=d.ends) {this.finish(null,'Time limit: stakes refunded.');return;}
    for(const side of ['A','B']) {
      d['mana'+side]=Math.min(100,(d['mana'+side] ?? 100)+dt*3);
      const cast=d['cast'+side];
      if(cast && now>=cast.ends) {
        d['cast'+side]=null;
        this.resolveSpell(side,cast.spell);
        if(!this.state.duel)return;
      }
      if(d['poison'+side]>0 && now>=(d['poisonNext'+side] || Infinity)) {
        // At most four ticks, including when a persisted duel wakes late.
        const until=Math.min(now,d['poison'+side]);
        const ticks=Math.max(0,Math.floor((until-d['poisonNext'+side])/2000)+1);
        d['poisonNext'+side]+=ticks*2000;
        d['hp'+side]=Math.max(0,d['hp'+side]-ticks*2);
        if(d['hp'+side]===0){this.finish(side==='A' ? d.b:d.a,'Poison');return;}
        if(now>=d['poison'+side])d['poison'+side]=0;
      }
    }
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
    if(d['cast'+side]) throw Error('Finish casting before your next action.');
    if(now<(d['ready'+side] || 0)) throw Error('Recover before your next action.');
    if(d['stamina'+side]<cost) throw Error('Not enough stamina.');
    const a=this.peer(id),b=this.peer(side==='A' ? d.b:d.a);
    if(action!=='guard' && distance(a,b)>(action==='lunge' ? 170:115)) throw Error('Move closer to your opponent.');
    d['last'+side]=now;d['ready'+side]=now+cooldown;d['stamina'+side]-=cost;
    if(action==='guard') {d['guard'+side]=now+1200;return;}
    d['swing'+side]=now;
    let damage=action==='lunge' ? 16:10;
    if(d.npc && side==='B')damage=action==='lunge' ? Math.ceil((d.npc.damage || 10)*1.6):(d.npc.damage || 10);
    if(d['guard'+other]>now) {damage=Math.ceil(damage/2);d['guard'+other]=0;}
    d['cast'+other]=null; // A landed melee hit interrupts the opponent's cast.
    d['hp'+other]=Math.max(0,d['hp'+other]-damage);
    if(d['hp'+other]===0) this.finish(id,'Knockout');
  }
  cast(id,spellId) {
    this.tick();const d=this.state.duel;
    if(!d || !this.busy(id))throw Error('Accept a duel before casting.');
    const now=this.clock();if(now<d.starts)throw Error('Wait for the countdown.');
    const spell=Number.isSafeInteger(spellId) ? DEN_SPELLS[spellId]:null;
    if(!spell)throw Error('That spell is not available in the arena.');
    const side=id===d.a ? 'A':'B',other=side==='A' ? 'B':'A';
    if(d['cast'+side] || now<(d['ready'+side] || 0))throw Error('Recover before your next cast.');
    if(d['mana'+side]<spell.mana)throw Error('Not enough arena mana.');
    if(spell.damage && distance(this.peer(id),this.peer(d[other.toLowerCase()]))>400)throw Error('Opponent is out of spell range.');
    d['mana'+side]-=spell.mana;d['guard'+side]=0;
    const ends=now+450+spell.circle*180;
    d['cast'+side]={spell:spellId,starts:now,ends};d['ready'+side]=ends+350;
  }
  resolveSpell(side,spellId) {
    const d=this.state.duel,spell=DEN_SPELLS[spellId];if(!d || !spell)return;
    const other=side==='A' ? 'B':'A',now=this.clock();
    if(spell.heal)d['hp'+side]=Math.min(side==='B' && d.npc ? d.npc.hp:100,d['hp'+side]+spell.heal);
    if(spell.cure){d['poison'+side]=0;d['poisonNext'+side]=0;}
    if(spell.damage) {
      if(distance(this.peer(d[side.toLowerCase()]),this.peer(d[other.toLowerCase()]))>400)return;
      d['hp'+other]=Math.max(0,d['hp'+other]-spell.damage);
      d['cast'+other]=null;
      if(spell.poison){d['poison'+other]=now+8000;d['poisonNext'+other]=now+2000;}
      if(d['hp'+other]===0)this.finish(d[side.toLowerCase()],'Spell knockout');
    }
  }
  finish(winner,reason) {
    const d=this.state.duel;if(!d) return;
    if(winner===d.a) this.state.wallets[d.keyA]+=d.stake*2;
    else if(winner===d.b) this.state.wallets[d.keyB]+=d.stake*2;
    else {this.state.wallets[d.keyA]+=d.stake;this.state.wallets[d.keyB]+=d.stake;}
    let ladderNote='';
    if(d.ladder) {
      const weekly=this.weekly(),entry=d.ladder;
      if(entry.week!==weekly.week)ladderNote='Weekly reset: this match does not advance the new ladder.';
      else {
        const r=weekly.records[entry.account] || (weekly.records[entry.account]={name:d.nameA,cleared:0,wins:0,losses:0,clearMs:0});
        r.name=d.nameA;
        if(winner===d.a && reason==='Knockout' && r.cleared===entry.stage-1) {
          r.cleared=entry.stage;r.wins++;r.clearMs+=Math.max(0,this.clock()-d.starts);
          if(r.cleared>=3) {const titles=this.state.cosmetics || (this.state.cosmetics={});titles[entry.account]=r.cleared===5 ? 'Blackwake Champion':titles[entry.account] || 'Pit Contender';}
          ladderNote='Ladder stage '+entry.stage+' cleared.';
          if(r.cleared===5)ladderNote+=' Blackwake Champion title unlocked.';
        } else if(this.clock()>=d.starts) {r.losses++;ladderNote='Retry this stage for free.';}
      }
    }
    this.state.results.push({id:d.id,winner:winner || '',name:winner===d.a ? d.nameA:winner===d.b ? d.nameB:'Draw',reason:reason+(ladderNote ? ' / '+ladderNote:''),stake:d.stake});
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
