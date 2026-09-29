// Smoke test against `wrangler dev` (npm run dev, then npm test).
// Two players join the same zone; each must see the other arrive, move, chat and leave.
const base = process.env.MP_URL || "ws://127.0.0.1:8787";
const zone = "test" + Math.floor(Math.random() * 1e6);

function open(name) {
  return new Promise((resolve, reject) => {
    const ws = new WebSocket(`${base}/zone/${zone}`);
    const inbox = [];
    ws.onmessage = (e) => inbox.push(JSON.parse(e.data));
    ws.onerror = (e) => reject(new Error("socket error " + (e.message || "")));
    ws.onopen = () => { ws.send(JSON.stringify({ t: "hello", name, look: "hero_knight|3" })); resolve({ ws, inbox }); };
  });
}
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
async function waitFor(inbox, pred, what) {
  for (let i = 0; i < 50; i++) {
    const m = inbox.find(pred);
    if (m) return m;
    await sleep(100);
  }
  throw new Error("timed out waiting for " + what + "; got " + JSON.stringify(inbox));
}

let failed = false;
function check(ok, label) { console.log((ok ? "PASS " : "FAIL ") + label); if (!ok) failed = true; }

const a = await open("Alice");
const wa = await waitFor(a.inbox, (m) => m.t === "welcome", "Alice welcome");
check(Array.isArray(wa.players) && wa.players.length === 0, "first player sees an empty zone");
const b = await open("Bob|<script>\u0007");
const wb = await waitFor(b.inbox, (m) => m.t === "welcome", "Bob welcome");
check(wb.players.length === 1 && wb.players[0].name === "Alice", "second player sees the first");
const join = await waitFor(a.inbox, (m) => m.t === "join", "join at Alice");
check(join.p.name === "Bob <script>", "names are cleaned: " + JSON.stringify(join.p.name));
b.ws.send(JSON.stringify({ t: "pos", x: 1234.5, z: 678, yaw: 1.5, mv: 1 }));
const pos = await waitFor(a.inbox, (m) => m.t === "pos", "pos at Alice");
check(pos.x === 1234.5 && pos.z === 678 && pos.mv === 1 && pos.id === wb.id, "moves are relayed");
b.ws.send(JSON.stringify({ t: "pos", x: "NaN", z: 1e12, yaw: 0 }));
const pos2 = await waitFor(a.inbox, (m) => m.t === "pos" && m !== pos, "clamped pos");
check(pos2.x === 0 && pos2.z === 100000, "bad numbers are clamped");
a.ws.send(JSON.stringify({ t: "chat", text: "hail, traveller!" }));
const chatB = await waitFor(b.inbox, (m) => m.t === "chat", "chat at Bob");
const chatA = await waitFor(a.inbox, (m) => m.t === "chat", "chat echo at Alice");
check(chatB.text === "hail, traveller!" && chatB.name === "Alice" && chatA.text === chatB.text, "chat reaches everyone, sender included");
a.ws.send(JSON.stringify({ t: "chat", text: "spam" }));
await sleep(400);
check(b.inbox.filter((m) => m.t === "chat").length === 1, "chat is limited to one a second");
b.ws.close();
const leave = await waitFor(a.inbox, (m) => m.t === "leave", "leave at Alice");
check(leave.id === wb.id, "leaving is announced");
a.ws.close();
await sleep(200);
console.log(failed ? "SOME TESTS FAILED" : "ALL PASSED");
process.exit(failed ? 1 : 0);
