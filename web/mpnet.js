// Town Forge multiplayer client, phase 1 (2026-09-29): presence + chat.
// Talks to the zone server in server/mp (a Cloudflare Worker). Inert until
// window.TF_MP_URL is set (web/mp-config.js), e.g. "wss://townforge-mp.<you>.workers.dev".
//
// The game polls TFMp.state(): plain "key=value" lines it can parse without JSON.
//   st=<0 off|1 connecting|2 online>|<players here, you included>|<my id>|<zone>
//   p=<id>|<name>|<look>|<x>|<z>|<yaw>|<moving 0|1>     (everyone else in your zone)
//   c=<id>|<name>|<text>                                (chat lines since the last poll)
(function () {
  var ws = null, zone = '', wantZone = '', name = '', look = '', myId = '', status = 0;
  var players = {}, chats = [], retryMs = 1000, retryT = null;
  var last = { x: NaN, z: NaN, yaw: 0, mv: 0 }, lastSent = 0;

  var den={version:0,gold:0,offers:[],duel:null,results:[],rollSeq:0},denText='',denPos=null,denSerial=0,recovered=false,authUser=null,authBusy=false;
  function denToken() {
    var token=localStorage.getItem('tf-den-purse');
    if(!/^[a-f0-9]{64}$/.test(token||'')) {token=Array.from(crypto.getRandomValues(new Uint8Array(32)),n=>n.toString(16).padStart(2,'0')).join('');localStorage.setItem('tf-den-purse',token);}
    return token;
  }
  function pendingRoll() {try{return JSON.parse(localStorage.getItem('tf-den-roll')||'null');}catch(e){return null;}}
  function url() { return String(window.TF_MP_URL || '').replace(/\/+$/, ''); }
  function clean(s) { return String(s == null ? '' : s).replace(/[|\r\n]/g, ' '); }
  function send(o) { if (ws && ws.readyState === 1) { try { ws.send(JSON.stringify(o)); } catch (e) {} } }

  function accountAction(kind) {
    if(authBusy)return;
    var client=window.TFCloud && TFCloud.client(),socket=ws;
    if(!client) {if(kind==='den_ladder')denText='Sign in using Cloud save for the weekly ladder.';return;}
    authBusy=true;
    client.auth.getSession().then(function(r) {
      if(ws!==socket || zone!=='den')return;
      var session=r.data && r.data.session;
      if(kind==='den_ladder' && !session){denText='Sign in using Cloud save for the weekly ladder.';return;}
      authUser=session ? session.user.id:'';
      send({t:kind,accessToken:session ? session.access_token:null});
    }).catch(function(){denText='Cloud sign-in could not be checked. Try again.';authUser=null;}).finally(function(){authBusy=false;});
  }
  function close() {
    if (retryT) { clearTimeout(retryT); retryT = null; }
    if (ws) { var w = ws; ws = null; w.onclose = null; try { w.close(); } catch (e) {} }
    players = {}; myId = ''; status = 0; zone = '';
  }
  function connect() {
    close();
    if (!url() || !wantZone) return;
    zone = wantZone; status = 1;den={version:0,gold:0,offers:[],duel:null,results:[],rollSeq:0};recovered=false;authUser=null;authBusy=false;denPos=null;denText='';
    var sock;
    try { sock = new WebSocket(url() + '/zone/' + encodeURIComponent(zone)); } catch (e) { schedule(); return; }
    ws = sock;
    sock.onopen = function () {
      if(ws!==sock) return;
      retryMs = 1000;
      try {send({ t: 'hello', name: name, look: look,token:denToken() });} catch(e) {denText='Enable browser storage to keep your Den purse.';return;}
      if (!isNaN(last.x)) { send({ t: 'pos', x: last.x, z: last.z, yaw: last.yaw, mv: last.mv }); lastSent = Date.now(); }
    };
    sock.onmessage = function (e) {
      if(ws!==sock) return;
      var m; try { m = JSON.parse(e.data); } catch (err) { return; }
      if (m.t === 'welcome') { myId = m.id; status = 2; players = {}; (m.players || []).forEach(function (p) { if(p.id!==myId) players[p.id] = p; }); }
      else if (m.t === 'join' && m.p && m.p.id!==myId) players[m.p.id] = m.p;
      else if (m.t === 'leave') delete players[m.id];
      else if (m.t === 'pos') { var p = players[m.id]; if (p) { p.x = m.x; p.z = m.z; p.yaw = m.yaw; p.mv = m.mv; } }
      else if(m.t==='den_state') {
        var before=den.duel;den=m;
        if(before && !m.duel) {var result=(m.results||[]).find(r=>r.id===before.id);if(result) denText=result.name+' / '+result.reason+' / purse '+m.gold+' test gold';}
        if(!recovered) {recovered=true;var p=pendingRoll();if(p) send(p);accountAction('den_auth');}
      }
      else if(m.t==='den_pos') {denPos={serial:++denSerial,x:m.x,z:m.z};}
      else if(m.t==='den_roll') {
        den.gold=m.gold;den.rollSeq=m.sequence;
        denText='Die: '+m.roll+'. '+(m.payout ? 'Returned '+m.payout+' test gold.':'Lost '+m.stake+' test gold.');
        var p=pendingRoll();if(p && p.request===m.request) localStorage.removeItem('tf-den-roll');
      }
      else if(m.t==='den_error') {denText=String(m.text||'Den action rejected.');var p=pendingRoll();if(m.rejected && p && p.request===m.request) localStorage.removeItem('tf-den-roll');}
      else if (m.t === 'chat') { chats.push(m); if (chats.length > 20) chats.shift(); }
    };
    sock.onclose = function (e) { if (ws === sock) { ws = null; players = {}; if(e && (e.code===1008 || e.reason==='replaced by current browser connection')) {status=0;return;} status = 1; schedule(); } };
    sock.onerror = function () { /* onclose follows */ };
  }
  function schedule() {
    if (retryT || !wantZone) return;
    retryT = setTimeout(function () { retryT = null; if (wantZone) connect(); }, retryMs);
    retryMs = Math.min(30000, retryMs * 2);
  }

  window.TFMp = {
    configured: function () { return url() ? 1 : 0; },
    // Enter a zone ("" = none: in a dungeon, a building, or multiplayer switched off).
    zone: function (z, playerName, playerLook) {
      name = String(playerName || 'Adventurer').slice(0, 24);
      var newLook = String(playerLook || '').slice(0, 96);
      if (z !== wantZone) { wantZone = z; if (z) connect(); else close(); }
      else if (newLook !== look && ws && ws.readyState === 1) { look = newLook; send({ t: 'hello', name: name, look: look }); }
      look = newLook;
      if(zone==='den' && den.version===1 && window.TFCloud) {
        var user=TFCloud.user(),id=user ? user.id:'';
        if(id!==authUser)accountAction('den_auth');
      }
    },
    // Called every frame; sends ~5 times a second while moving, and a heartbeat every 5 s.
    pos: function (x, z, yaw, mv) {
      var now = Date.now();
      var moved = isNaN(last.x) || Math.abs(x - last.x) > 1 || Math.abs(z - last.z) > 1 || Math.abs(yaw - last.yaw) > 0.05 || mv !== last.mv;
      last = { x: x, z: z, yaw: yaw, mv: mv };
      if ((moved && now - lastSent > 200) || now - lastSent > 5000) {
        send({ t: 'pos', x: Math.round(x * 10) / 10, z: Math.round(z * 10) / 10, yaw: Math.round(yaw * 100) / 100, mv: mv });
        lastSent = now;
      }
    },
    denAction:function(action,target,value) {
      if(status!==2 || den.version!==1) {denText='The Den server needs its combat update before you can play.';return;}
      denText='';
      if(action==='ladder')accountAction('den_ladder');
      else if(action==='practice') send({t:'den_practice'});
      else if(action==='challenge') send({t:'den_challenge',target:target,stake:value});
      else if(action==='accept' || action==='decline') send({t:'den_answer',offer:target,accept:action==='accept'});
      else if(action==='cast') {if(den.spellsVersion===1)send({t:'den_cast',spell:value});else denText='Arena spells are updating. Reconnect shortly.';}
      else if(action==='strike' || action==='lunge' || action==='guard' || action==='surrender') send({t:'den_fight',action:action});
      else if(action==='roll') {
        var p=pendingRoll();
        if(!p) {p={t:'den_roll',request:crypto.randomUUID(),face:Number(target),stake:value,sequence:den.rollSeq+1};localStorage.setItem('tf-den-roll',JSON.stringify(p));}
        send(p);
      } else if(action==='retry') {var p=pendingRoll();if(p)send(p);}
    },
    chat: function (text) { text = String(text || '').trim().slice(0, 140); if (text) send({ t: 'chat', text: text }); },
    state: function () {
      var n = 1, out = [];
      Object.keys(players).forEach(function (id) {
        var p = players[id]; if(id===myId)return; n++;
        out.push('p=' + clean(p.id) + '|' + clean(p.name) + '|' + clean(p.look) + '|' + (+p.x || 0) + '|' + (+p.z || 0) + '|' + (+p.yaw || 0) + '|' + (p.mv ? 1 : 0));
      });
      chats.forEach(function (c) { out.push('c=' + clean(c.id) + '|' + clean(c.name) + '|' + clean(c.text)); });
      chats = [];
      if(zone==='den') {
        var now=Date.now();
        var ladder=den.ladder;
        if(ladder && ladder.version===1) {
          out.push('ladder='+[1,ladder.verified ? 1:0,ladder.cleared,Math.max(0,(ladder.resetAt-now)/1000),clean(ladder.title),ladder.wins,ladder.losses].join('|'));
          (ladder.stages||[]).forEach((s,i)=>out.push('ladderstage='+[i+1,clean(s.name),clean(s.style),s.hp].join('|')));
          (ladder.leaders||[]).forEach(s=>out.push('ladderleader='+[clean(s.name),s.cleared,Math.round(s.clearMs/1000)].join('|')));
        }
        out.push('den='+[den.version,den.gold,den.rollSeq,clean(denText),pendingRoll() ? 1:0,den.spellsVersion || 0].join('|'));
        if(denPos) out.push('denpos='+[denPos.serial,denPos.x,denPos.z].join('|'));
        (den.offers||[]).forEach(o=>out.push('offer='+[o.id,o.a,o.b,clean(o.nameA),clean(o.nameB),o.stake,Math.max(0,(o.expires-now)/1000)].join('|')));
        var d=den.duel;
        if(d && d.npc) {var bot=d.npc;out.push('p='+[bot.id,clean(bot.name),'hero,2,2,-1,0,0',bot.x,bot.z,bot.yaw,1].join('|'));}
        if(d) out.push('duel='+[d.id,d.a,d.b,clean(d.nameA),clean(d.nameB),d.hpA,d.hpB,Math.round(d.staminaA),Math.round(d.staminaB),Math.max(0,(d.starts-now)/1000),Math.max(0,(d.ends-now)/1000),d.stake,d.guardA>now ? 1:0,d.guardB>now ? 1:0,(now-d.swingA)/1000,(now-d.swingB)/1000,d.npc && d.npc.stage>0 ? 1:0,Math.round(d.manaA ?? 100),Math.round(d.manaB ?? 100),d.castA ? Math.max(0,(d.castA.ends-now)/1000):0,d.castB ? Math.max(0,(d.castB.ends-now)/1000):0,d.castA ? d.castA.spell:-1,d.castB ? d.castB.spell:-1,d.poisonA>now ? 1:0,d.poisonB>now ? 1:0,100,d.npc ? d.npc.hp:100].join('|'));
        (den.results||[]).forEach(r=>out.push('denresult='+[r.id,clean(r.name),clean(r.reason),r.stake].join('|')));
      }
      return 'st=' + status + '|' + (status === 2 ? n : 0) + '|' + clean(myId) + '|' + clean(zone) + '\n' + out.join('\n');
    },
  };
})();
