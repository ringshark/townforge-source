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

  function url() { return String(window.TF_MP_URL || '').replace(/\/+$/, ''); }
  function clean(s) { return String(s == null ? '' : s).replace(/[|\r\n]/g, ' '); }
  function send(o) { if (ws && ws.readyState === 1) { try { ws.send(JSON.stringify(o)); } catch (e) {} } }

  function close() {
    if (retryT) { clearTimeout(retryT); retryT = null; }
    if (ws) { var w = ws; ws = null; w.onclose = null; try { w.close(); } catch (e) {} }
    players = {}; myId = ''; status = 0; zone = '';
  }
  function connect() {
    close();
    if (!url() || !wantZone) return;
    zone = wantZone; status = 1;
    var sock;
    try { sock = new WebSocket(url() + '/zone/' + encodeURIComponent(zone)); } catch (e) { schedule(); return; }
    ws = sock;
    sock.onopen = function () {
      retryMs = 1000;
      send({ t: 'hello', name: name, look: look });
      if (!isNaN(last.x)) { send({ t: 'pos', x: last.x, z: last.z, yaw: last.yaw, mv: last.mv }); lastSent = Date.now(); }
    };
    sock.onmessage = function (e) {
      var m; try { m = JSON.parse(e.data); } catch (err) { return; }
      if (m.t === 'welcome') { myId = m.id; status = 2; players = {}; (m.players || []).forEach(function (p) { players[p.id] = p; }); }
      else if (m.t === 'join' && m.p) players[m.p.id] = m.p;
      else if (m.t === 'leave') delete players[m.id];
      else if (m.t === 'pos') { var p = players[m.id]; if (p) { p.x = m.x; p.z = m.z; p.yaw = m.yaw; p.mv = m.mv; } }
      else if (m.t === 'chat') { chats.push(m); if (chats.length > 20) chats.shift(); }
    };
    sock.onclose = function () { if (ws === sock) { ws = null; players = {}; status = 1; schedule(); } };
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
    chat: function (text) { text = String(text || '').trim().slice(0, 140); if (text) send({ t: 'chat', text: text }); },
    state: function () {
      var n = 1, out = [];
      Object.keys(players).forEach(function (id) {
        var p = players[id]; n++;
        out.push('p=' + clean(p.id) + '|' + clean(p.name) + '|' + clean(p.look) + '|' + (+p.x || 0) + '|' + (+p.z || 0) + '|' + (+p.yaw || 0) + '|' + (p.mv ? 1 : 0));
      });
      chats.forEach(function (c) { out.push('c=' + clean(c.id) + '|' + clean(c.name) + '|' + clean(c.text)); });
      chats = [];
      return 'st=' + status + '|' + (status === 2 ? n : 0) + '|' + clean(myId) + '|' + clean(zone) + '\n' + out.join('\n');
    },
  };
})();
