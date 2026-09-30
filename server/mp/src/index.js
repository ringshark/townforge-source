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
    this.rate = new Map(); // ws -> {sec, n}; not persisted - a wake resets it, which is fine
  }

  async fetch(request) {
    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);
    this.ctx.acceptWebSocket(server);
    if (this.players().length >= MAX_PLAYERS) {
      server.send(JSON.stringify({ t: "full" }));
      server.close(1013, "zone full");
      return new Response(null, { status: 101, webSocket: client });
    }
    const id = crypto.randomUUID().replace(/-/g, "").slice(0, 10);
    server.serializeAttachment({ id, name: "", look: "", x: 0, z: 0, yaw: 0, mv: 0, hello: false, lastChat: 0 });
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
    if (typeof raw !== "string" || raw.length > 1024 || !this.allow(ws)) return;
    let m;
    try { m = JSON.parse(raw); } catch (e) { return; }
    const a = ws.deserializeAttachment();
    if (!a || !m || typeof m.t !== "string") return;
    if (m.t === "hello") {
      const first = !a.hello;
      a.name = cleanText(m.name, NAME_MAX) || "Adventurer";
      a.look = cleanText(m.look, LOOK_MAX);
      a.hello = true;
      ws.serializeAttachment(a);
      if (first) {
        const others = this.players().filter(([w]) => w !== ws).map(([, s]) => this.pub(s));
        ws.send(JSON.stringify({ t: "welcome", id: a.id, players: others }));
      }
      this.broadcast({ t: "join", p: this.pub(a) }, ws);
      return;
    }
    if (!a.hello) return;
    if (m.t === "pos") {
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

  async webSocketClose(ws) { this.left(ws); }
  async webSocketError(ws) { this.left(ws); }
  left(ws) {
    const a = ws.deserializeAttachment();
    this.rate.delete(ws);
    try { ws.close(1000, "bye"); } catch (e) { /* already closed */ }
    if (a && a.hello) this.broadcast({ t: "leave", id: a.id }, ws);
  }
}
