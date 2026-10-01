// Town Forge multiplayer, phase 1 (2026-09-29): presence + chat.
//
// One Durable Object per zone ("wild", "town0".."town3"). Each player holds a
// WebSocket to their zone's object. The object uses the hibernation API, so it
// costs nothing while nobody moves: it wakes on a message, relays it, and goes
// back to sleep. There is no server tick - positions are relayed as they come.
//
// Protocol (JSON text frames):
//   client -> server
//     {t:"hello", name, look}                 once, right after connecting
//     {t:"pos", x, z, yaw, mv}                a few times a second while moving
//     {t:"chat", text}                        at most one a second
//   server -> client
//     {t:"welcome", id, players:[P...]}       your id and everyone already here
//     {t:"join", p:P}   {t:"leave", id}
//     {t:"pos", id, x, z, yaw, mv}
//     {t:"chat", id, name, text}
//     {t:"full"}                              the zone is full (then closed)
//   P = {id, name, look, x, z, yaw, mv}
import { DurableObject } from "cloudflare:workers";
import { verifyLadderAccount } from "./ladder-auth.js";
import { DenEngine } from "./den-engine.js";

const MAX_PLAYERS = 60;       // per zone
const MAX_MSGS_PER_SEC = 12;  // a client flooding past this gets dropped messages
const CHAT_MAX = 140;
const NAME_MAX = 24;
const LOOK_MAX = 96;
const ZONE_RE = /^\/zone\/([a-z0-9_-]{1,24})$/;

function cleanText(s, max) {
  return String(s == null ? "" : s).replace(/[\u0000-\u001f\u007f|]/g, " ").trim().slice(0, max);
}
function num(v, lo, hi) {
  const n = Number(v);
  return Number.isFinite(n) ? Math.min(hi, Math.max(lo, n)) : 0;
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === "/health") return new Response("ok", { headers: { "content-type": "text/plain" } });
    const m = url.pathname.match(ZONE_RE);
    if (!m) return new Response("not found", { status: 404 });
    // Header values are case-insensitive: some clients and proxies send "WebSocket".
    if ((request.headers.get("Upgrade") || "").toLowerCase() !== "websocket") return new Response("expected a websocket", { status: 426 });
    const allowed = String(env.ALLOWED_ORIGINS || "").split(",").map((o) => o.trim()).filter(Boolean);
    const origin = request.headers.get("Origin") || "";
    if (allowed.length && !allowed.includes(origin)) return new Response("origin not allowed", { status: 403 });
    const stub = env.ZONES.get(env.ZONES.idFromName(m[1]));
    return stub.fetch(request);
  },
};

export class Zone extends DurableObject {
  constructor(ctx, env) {
    super(ctx, env);
    this.env=env;
    this.ctx.blockConcurrencyWhile(async()=>{this.den=new DenEngine(await this.ctx.storage.get("den") || {});});
    this.rate = new Map(); // ws -> {sec, n}; not persisted - a wake resets it, which is fine
  }

  async fetch(request) {
    await this.expirePresence();
    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);
    this.ctx.acceptWebSocket(server);
    if (this.players().length >= MAX_PLAYERS) {
      server.send(JSON.stringify({ t: "full" }));
      server.close(1013, "zone full");
      return new Response(null, { status: 101, webSocket: client });
    }
    const id = crypto.randomUUID().replace(/-/g, "").slice(0, 10);
    server.serializeAttachment({ id, name: "", look: "", x: 0, z: 0, yaw: 0, mv: 0, hello: false, lastChat: 0, den: new URL(request.url).pathname.endsWith("/den"), lastPos: Date.now() });
    return new Response(null, { status: 101, webSocket: client });
  }

  // Everyone who has said hello, as [ws, state].
  players() {
    const out = [];
    for (const ws of this.ctx.getWebSockets()) {
      const a = ws.deserializeAttachment();
      if (a && a.hello) out.push([ws, a]);
    }
    return out;
  }
  broadcast(msg, except) {
    const s = JSON.stringify(msg);
    for (const [ws] of this.players()) {
      if (ws === except) continue;
      try { ws.send(s); } catch (e) { /* closing */ }
    }
  }
  pub(a) { return { id: a.id, name: a.name, look: a.look, x: a.x, z: a.z, yaw: a.yaw, mv: a.mv }; }

  allow(ws) {
    const sec = Math.floor(Date.now() / 1000);
    let r = this.rate.get(ws);
    if (!r || r.sec !== sec) { r = { sec, n: 0 }; this.rate.set(ws, r); }
    return ++r.n <= MAX_MSGS_PER_SEC;
  }

  async webSocketMessage(ws, raw) {
    if (typeof raw !== "string" || raw.length > 16384 || !this.allow(ws)) return;
    let m;
    try { m = JSON.parse(raw); } catch (e) { return; }
    if(raw.length>1024 && !["den_auth","den_ladder"].includes(m?.t))return;
    const a = ws.deserializeAttachment();
    if (!a || !m || typeof m.t !== "string") return;
    a.lastSeen=Date.now();ws.serializeAttachment(a);
    if (m.t === "hello") {
      const first = !a.hello;
      a.name = cleanText(m.name, NAME_MAX) || "Adventurer";
      a.look = cleanText(m.look, LOOK_MAX);
      if(!a.den && first && /^[a-f0-9]{64}$/.test(String(m.token || ""))) {
        const hash=await crypto.subtle.digest("SHA-256",new TextEncoder().encode(m.token));
        a.clientKey=Array.from(new Uint8Array(hash),n=>n.toString(16).padStart(2,"0")).join("");
        for(const [old,p] of this.players()) if(old!==ws && p.clientKey===a.clientKey) {
          p.hello=false;old.serializeAttachment(p);this.broadcast({t:"leave",id:p.id},old);
          try {old.close(1000,"replaced by current browser connection");} catch(e) {}
        }
      }
      if(a.den && first) {
        if(!/^[a-f0-9]{64}$/.test(String(m.token || ""))) {ws.send(JSON.stringify({t:"den_error",text:"Update the game to enter the Den."}));return;}
        const hash=await crypto.subtle.digest("SHA-256",new TextEncoder().encode(m.token));
        a.key=Array.from(new Uint8Array(hash),n=>n.toString(16).padStart(2,"0")).join("");
        if(this.players().some(([,p])=>p.key===a.key)) {ws.send(JSON.stringify({t:"den_error",text:"This Den purse is already open in another tab."}));ws.close(1008,"duplicate purse");return;}
        a.x=750;a.z=1250;a.lastPos=Date.now();
      }
      a.hello = true;
      if(!a.den) await this.ctx.storage.setAlarm(Date.now()+15000);
      ws.serializeAttachment(a);
      if (first) {
        const others = this.players().filter(([w]) => w !== ws).map(([, s]) => this.pub(s));
        ws.send(JSON.stringify({ t: "welcome", id: a.id, players: others }));
      }
      this.broadcast({ t: "join", p: this.pub(a) }, ws);
      if(a.den) {this.denPeers();this.den.wallet(a.key);await this.saveDen();ws.send(JSON.stringify({t:"den_pos",x:a.x,z:a.z}));this.denSync();}
      return;
    }
    if (!a.hello) return;
    if(a.den && m.t.startsWith("den_")) {
      let placement,applied=false;
      try {
        if(m.t==="den_auth" || m.t==="den_ladder") {
          const account=m.accessToken ? await verifyLadderAccount(this.env,m.accessToken):null;
          if(m.t==="den_ladder" && !account)throw Error('Sign in with Cloud save for the ladder.');
          const fresh=ws.deserializeAttachment();if(!fresh?.hello)throw Error('Reconnect before entering the ladder.');
          fresh.account=account;ws.serializeAttachment(fresh);Object.assign(a,fresh);
        }
        this.denPeers();
        if(m.t==="den_ladder")placement=this.den.practice(a.id,true);
        else if(m.t==="den_auth") {}
        else if(m.t==="den_practice") placement=this.den.practice(a.id);
        else if(m.t==="den_challenge") this.den.challenge(a.id,String(m.target),m.stake);
        else if(m.t==="den_answer") placement=this.den.answer(a.id,String(m.offer),m.accept===true);
        else if(m.t==="den_cast") this.den.cast(a.id,m.spell);
        else if(m.t==="den_fight") this.den.fight(a.id,String(m.action));
        else if(m.t==="den_roll") {const result=this.den.casino(a.id,String(m.request),m.stake,m.face,m.sequence);applied=true;await this.saveDen();ws.send(JSON.stringify(result));}
        else if(m.t!=="den_poll") return;
        this.den.tick();
        if(placement) for(const [w,p] of this.players()) {const at=placement.a.id===p.id ? placement.a:placement.b.id===p.id ? placement.b:null;if(at) {p.x=at.x;p.z=at.z;p.lastPos=Date.now();w.serializeAttachment(p);w.send(JSON.stringify({t:"den_pos",x:p.x,z:p.z}));this.broadcast({t:"pos",...this.pub(p)},w);}}
        await this.saveDen();this.denSync();
      } catch(e) {ws.send(JSON.stringify({t:"den_error",text:e.message,request:m.request,rejected:!applied}));}
      return;
    }
    if (m.t === "pos") {
      if(a.den) {
        this.denPeers();const at=this.den.move(a.id,Number(m.x),Number(m.z),(Date.now()-a.lastPos)/1000);
        a.x=at.x;a.z=at.z;a.lastPos=Date.now();a.yaw=num(m.yaw,-10,10);a.mv=m.mv ? 1:0;
        ws.serializeAttachment(a);this.broadcast({t:"pos",...this.pub(a)},ws);
        if(Math.hypot(a.x-Number(m.x),a.z-Number(m.z))>24) ws.send(JSON.stringify({t:"den_pos",x:a.x,z:a.z}));
        return;
      }
      a.x = num(m.x, -100000, 100000); a.z = num(m.z, -100000, 100000);
      a.yaw = num(m.yaw, -10, 10); a.mv = m.mv ? 1 : 0;
      ws.serializeAttachment(a);
      this.broadcast({ t: "pos", id: a.id, x: a.x, z: a.z, yaw: a.yaw, mv: a.mv }, ws);
      return;
    }
    if (m.t === "chat") {
      const now = Date.now();
      if (now - a.lastChat < 1000) return;
      const text = cleanText(m.text, CHAT_MAX);
      if (!text) return;
      a.lastChat = now;
      ws.serializeAttachment(a);
      this.broadcast({ t: "chat", id: a.id, name: a.name, text }); // the sender sees it echoed too
    }
  }

  denPeers() {this.den.peersFrom(this.players().filter(([,a])=>a.den).map(([,a])=>a));}
  async saveDen() {await this.ctx.storage.put("den",this.den.state);const d=this.den.state.duel;await this.ctx.storage.setAlarm(Date.now()+(d ? 1000:30000));}
  denSync() {this.denPeers();for(const [w,a] of this.players()) if(a.den) {try {w.send(JSON.stringify(this.den.snapshot(a.id)));}catch(e) {}}}
  async expirePresence() {
    for(const [w,p] of this.players()) if(!p.den && Date.now()-(p.lastSeen || p.lastPos || 0)>45000) {
      p.hello=false;w.serializeAttachment(p);this.broadcast({t:"leave",id:p.id},w);
      try {w.close(1001,"presence heartbeat expired");}catch(e) {}
    }
  }
  async alarm() {await this.expirePresence();this.denPeers();this.den.tick();this.den.practiceTick();this.den.weekly();await this.ctx.storage.put("den",this.den.state);this.denSync();if(this.den.state.duel || this.den.state.offers.length) await this.ctx.storage.setAlarm(Date.now()+1000);else if(this.players().length) await this.ctx.storage.setAlarm(Date.now()+15000);}
  async webSocketClose(ws) { await this.left(ws); }
  async webSocketError(ws) { await this.left(ws); }
  async left(ws) {
    const a = ws.deserializeAttachment();
    this.rate.delete(ws);
    if(a && a.den && a.hello) {this.den.leave(a.id);await this.saveDen();}
    try { ws.close(1000, "bye"); } catch (e) { /* already closed */ }
    if (a && a.hello) this.broadcast({ t: "leave", id: a.id }, ws);
  }
}
