// Soft music (2026-10-27): three slow looping ambient pieces, synthesized here so there is no licence to track.
//   node tools/gen_soft_music.mjs      -> assets/music/{town,wild,dungeon}_soft.wav (IMA ADPCM, mono, 22.05 kHz)
// Replaces gen_calm_music.py's set, which Mark found more annoying than calming: it scattered random plucked harp,
// flute and bell notes over the pads. This set has no plucks and nothing high or sharp:
//   - warm pads that breathe in and out over twelve seconds or more, each chord melting into the next;
//   - one soft voice that plays a few notes at a time, moving by step, never above G5, with slow attacks;
//   - a long, dark reverb, a low-pass over everything, and a quiet peak level.
// The loop wraps (the reverb is run over the loop twice and the second pass kept), so it never clicks or jumps.
import fs from 'node:fs';
import path from 'node:path';

const SR = 22050;
let seed = 1234567;
const rnd = () => { seed = (seed * 1664525 + 1013904223) >>> 0; return seed / 4294967296; };
const midi = n => 440 * Math.pow(2, (n - 69) / 12);
const TAU = Math.PI * 2;

// A soft pad voice: a few detuned partials, a slow swell and a very slow tremolo.
function addPad(buf, at, freq, dur, amp, attack, release) {
  const n = Math.floor(dur * SR), start = Math.floor(at * SR), len = buf.length;
  const ph = [rnd() * TAU, rnd() * TAU, rnd() * TAU];
  const trem = rnd() * TAU, tremRate = 0.07 + rnd() * 0.06;
  for (let i = 0; i < n; i++) {
    const t = i / SR;
    const a = Math.min(1, t / attack), r = Math.min(1, (dur - t) / release);
    const e = Math.pow(Math.sin(Math.min(a, r) * Math.PI / 2), 2);
    const w = Math.sin(TAU * freq * t + ph[0]) + 0.45 * Math.sin(TAU * freq * 1.002 * t + ph[1]) + 0.18 * Math.sin(TAU * freq * 2 * t + ph[2]) + 0.05 * Math.sin(TAU * freq * 3 * t);
    buf[(start + i) % len] += amp * e * w * (1 + 0.06 * Math.sin(TAU * tremRate * t + trem)) / 1.7;
  }
}
// The one soft voice: slow attack, long release, a faint vibrato that only arrives late in the note.
function addVoice(buf, at, freq, dur, amp) {
  const n = Math.floor(dur * SR), start = Math.floor(at * SR), len = buf.length, attack = 1.3, release = dur * 0.55;
  let phase = 0;
  for (let i = 0; i < n; i++) {
    const t = i / SR;
    const a = Math.min(1, t / attack), r = Math.min(1, (dur - t) / release);
    const e = Math.pow(Math.sin(Math.min(a, r) * Math.PI / 2), 2);
    phase += TAU * freq * (1 + 0.0025 * Math.sin(TAU * 4.6 * t) * Math.min(1, Math.max(0, t - 0.8))) / SR;
    buf[(start + i) % len] += amp * e * (Math.sin(phase) + 0.14 * Math.sin(2 * phase)) ;
  }
}
// One-pole low-pass (twice: 12 dB per octave) so nothing bright survives. Circular: the loop is run through twice and
// the second pass kept, so the filter has settled by the time the loop starts and the seam does not step.
function lowpass(x, cutoff, passes = 2) {
  const k = 1 - Math.exp(-TAU * cutoff / SR), n = x.length;
  for (let p = 0; p < passes; p++) {
    let y = 0;
    for (let i = 0; i < 2 * n; i++) { y += k * (x[i % n] - y); if (i >= n) x[i - n] = y; }
  }
}
// A small feedback-delay reverb (four combs and two all-passes). Runs over the loop twice and keeps the second pass.
function reverb(x, wet) {
  const doubled = new Float32Array(x.length * 2); doubled.set(x); doubled.set(x, x.length);
  const combs = [1557, 1617, 1491, 1422, 1277, 1356].map((d, i) => ({ buf: new Float32Array(d * 2), pos: 0, g: 0.86 - i * 0.005 }));
  const out = new Float32Array(doubled.length);
  for (let i = 0; i < doubled.length; i++) {
    let s = 0;
    for (const c of combs) { const v = c.buf[c.pos]; c.buf[c.pos] = doubled[i] + v * c.g * 0.93 + (c.last || 0) * 0.0; c.last = v; c.pos = (c.pos + 1) % c.buf.length; s += v; }
    out[i] = s / combs.length;
  }
  lowpass(out, 1800, 1);
  const res = new Float32Array(x.length);
  for (let i = 0; i < x.length; i++) res[i] = (1 - wet) * x[i] + wet * out[x.length + i] * 1.6;
  return res;
}

// The scale-walk melody: stepwise, mostly small moves, resting in between.
function walkVoice(buf, scale, startIdx, from, to, spacing, amp, dur) {
  let idx = startIdx, t = from;
  while (t < to) {
    addVoice(buf, t, midi(scale[idx]), dur, amp * (0.75 + 0.25 * rnd()));
    const step = rnd() < 0.62 ? (rnd() < 0.5 ? 1 : -1) : (rnd() < 0.5 ? 2 : -2);
    idx = Math.min(scale.length - 1, Math.max(0, idx + step));
    t += spacing * (0.8 + rnd() * 0.7) + (rnd() < 0.25 ? spacing * 0.9 : 0);   // now and then a longer rest
  }
}

function piece({ chords, chordLen, repeats, scale, startIdx, voiceSpacing, voiceAmp, padAmp, voiceDur, cutoff, wet }) {
  const total = chordLen * chords.length * repeats;
  const buf = new Float32Array(Math.floor(total * SR));
  for (let r = 0; r < repeats; r++) chords.forEach((chord, c) => {
    const at = (r * chords.length + c) * chordLen;
    chord.forEach((n, j) => addPad(buf, at - 1.5 + j * 0.25, midi(n), chordLen + 5, padAmp, 4.5, 6));
  });
  walkVoice(buf, scale, startIdx, 3, total - 4, voiceSpacing, voiceAmp, voiceDur);
  lowpass(buf, cutoff);
  return reverb(buf, wet);
}

// ---- IMA ADPCM WAV writer (4 bits a sample: a quarter of the size of 16-bit PCM; raylib's dr_wav decodes it) ----
const STEPS = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767];
const INDEX_ADJ = [-1, -1, -1, -1, 2, 4, 6, 8];
function encodeAdpcm(pcm, sampleRate) {
  const blockAlign = 512, perBlock = (blockAlign - 4) * 2 + 1;
  const blocks = Math.ceil(pcm.length / perBlock);
  const data = Buffer.alloc(blocks * blockAlign);
  for (let b = 0; b < blocks; b++) {
    const o = b * blockAlign, base = b * perBlock;
    let pred = pcm[base] || 0, index = 0;
    data.writeInt16LE(pred, o); data[o + 2] = index; data[o + 3] = 0;
    let nibbleBuf = 0, nibbleHalf = false, outPos = o + 4;
    for (let i = 1; i < perBlock; i++) {
      const s = base + i < pcm.length ? pcm[base + i] : pred;
      let diff = s - pred, code = 0; const step = STEPS[index];
      if (diff < 0) { code = 8; diff = -diff; }
      let d = step >> 3;
      if (diff >= step) { code |= 4; diff -= step; d += step; }
      if (diff >= (step >> 1)) { code |= 2; diff -= step >> 1; d += step >> 1; }
      if (diff >= (step >> 2)) { code |= 1; d += step >> 2; }
      pred = Math.max(-32768, Math.min(32767, code & 8 ? pred - d : pred + d));
      index = Math.max(0, Math.min(88, index + INDEX_ADJ[code & 7]));
      if (!nibbleHalf) { nibbleBuf = code; nibbleHalf = true; } else { data[outPos++] = nibbleBuf | (code << 4); nibbleHalf = false; }
    }
    if (nibbleHalf) data[outPos++] = nibbleBuf;
  }
  const fmt = Buffer.alloc(20);
  fmt.writeUInt16LE(0x11, 0); fmt.writeUInt16LE(1, 2); fmt.writeUInt32LE(sampleRate, 4);
  fmt.writeUInt32LE(Math.floor(sampleRate * blockAlign / perBlock), 8); fmt.writeUInt16LE(blockAlign, 12);
  fmt.writeUInt16LE(4, 14); fmt.writeUInt16LE(2, 16); fmt.writeUInt16LE(perBlock, 18);
  const fact = Buffer.alloc(4); fact.writeUInt32LE(pcm.length, 0);
  const chunk = (id, body) => { const h = Buffer.alloc(8); h.write(id, 0, 'ascii'); h.writeUInt32LE(body.length, 4); return Buffer.concat([h, body, body.length & 1 ? Buffer.alloc(1) : Buffer.alloc(0)]); };
  const body = Buffer.concat([Buffer.from('WAVE'), chunk('fmt ', fmt), chunk('fact', fact), chunk('data', data)]);
  const riff = Buffer.alloc(8); riff.write('RIFF', 0, 'ascii'); riff.writeUInt32LE(body.length, 4);
  return Buffer.concat([riff, body]);
}

function finish(buf, name, peak) {
  let max = 1e-9; for (const v of buf) max = Math.max(max, Math.abs(v));
  const pcm = new Int16Array(buf.length);
  let sq = 0; for (let i = 0; i < buf.length; i++) { const v = buf[i] * peak / max; pcm[i] = Math.round(v * 32767); sq += v * v; }
  const file = path.join('assets', 'music', `${name}_soft.wav`);
  fs.writeFileSync(file, encodeAdpcm(pcm, SR));
  console.log(name, (buf.length / SR).toFixed(0) + 's', 'rms', Math.sqrt(sq / buf.length).toFixed(3), (fs.statSync(file).size / 1024).toFixed(0) + ' KB');
}

// Town: warm C major, wide and slow. I - vi - IV - V (12 s a chord), a soft voice in the middle of the range.
finish(piece({
  chords: [[48, 55, 64, 67], [45, 52, 60, 64], [41, 53, 57, 60], [43, 50, 59, 62]], chordLen: 12, repeats: 2,
  scale: [60, 62, 64, 67, 69, 72, 74, 76], startIdx: 2, voiceSpacing: 6.5, voiceAmp: 0.13, padAmp: 0.1, voiceDur: 7, cutoff: 1400, wet: 0.42,
}), 'town', 0.34);
// Wilds: open D, long fifths and a lonely, quiet voice. D - Bm - G - A
finish(piece({
  chords: [[38, 45, 57, 61], [35, 47, 54, 59], [43, 50, 59, 62], [45, 52, 61, 64]], chordLen: 12, repeats: 2,
  scale: [62, 64, 66, 69, 71, 74, 76], startIdx: 3, voiceSpacing: 8, voiceAmp: 0.12, padAmp: 0.1, voiceDur: 8, cutoff: 1300, wet: 0.46,
}), 'wild', 0.33);
// Dungeon: a low, dark drift in A minor with no bells; the voice stays below middle C's octave and is barely there.
finish(piece({
  chords: [[33, 40, 45, 52], [33, 40, 47, 52], [29, 41, 45, 48], [31, 38, 43, 47]], chordLen: 12, repeats: 2,
  scale: [57, 60, 62, 64, 67, 69], startIdx: 1, voiceSpacing: 11, voiceAmp: 0.09, padAmp: 0.11, voiceDur: 9, cutoff: 900, wet: 0.5,
}), 'dungeon', 0.3);
