#!/usr/bin/env python3
"""Instrument sounds for the bard skills (2026-09-28, #65): a hand drum, a lute
and a harp. Plucked strings are Karplus-Strong; pure stdlib, 22050Hz mono WAV."""
import math, os, random, wave
SR = 22050
OUT = os.path.dirname(os.path.abspath(__file__))
random.seed(11)

def pluck(freq, dur, bright=0.5, damp=0.996):
    n = int(SR * dur); p = max(2, int(SR / freq))
    buf = [random.uniform(-1, 1) for _ in range(p)]
    for _ in range(3):  # soften the attack
        buf = [(buf[i] + buf[i - 1]) * 0.5 for i in range(p)]
    out = []
    for i in range(n):
        v = buf[i % p]
        nxt = buf[(i + 1) % p]
        buf[i % p] = damp * (bright * v + (1 - bright) * 0.5 * (v + nxt))
        out.append(v)
    return out

def drum(dur, f0=130, f1=55):
    n = int(SR * dur); out = []; ph = 0.0
    for i in range(n):
        t = i / SR
        f = f1 + (f0 - f1) * math.exp(-t * 18)
        ph += 2 * math.pi * f / SR
        body = math.sin(ph) * math.exp(-t * 9)
        slap = random.uniform(-1, 1) * math.exp(-t * 60) * 0.35
        out.append(body + slap)
    return out

def place(track, sound, at, vol=1.0):
    s = int(SR * at)
    need = s + len(sound)
    if len(track) < need: track.extend([0.0] * (need - len(track)))
    for i, v in enumerate(sound): track[s + i] += v * vol

def write(name, track):
    peak = max(1e-6, max(abs(v) for v in track))
    fade = int(SR * 0.05)
    for i in range(fade): track[-1 - i] *= i / fade
    with wave.open(os.path.join(OUT, name), 'w') as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(b''.join(int(v / peak * 0.85 * 32767).to_bytes(2, 'little', signed=True) for v in track))

# Hand drum: a little four-beat pattern.
t = []
for at, f0, v in [(0.0, 140, 1.0), (0.22, 180, 0.6), (0.36, 180, 0.55), (0.5, 120, 1.0), (0.78, 180, 0.6)]:
    place(t, drum(0.45, f0), at, v)
write('drum.wav', t)

# Lute: a strummed minor chord, then a short phrase.
t = []
A = 220.0
for k, f in enumerate([A, A * 1.2, A * 1.5, A * 2]):
    place(t, pluck(f, 1.2, 0.55, 0.995), k * 0.025, 0.7)
for k, f in enumerate([A * 2, A * 1.78, A * 1.5, A * 1.2 * 1.5]):
    place(t, pluck(f, 0.7, 0.6, 0.994), 0.45 + k * 0.17, 0.8)
write('lute.wav', t)

# Harp: a bright rising arpeggio that rings.
t = []
base = 261.6
for k, r in enumerate([1, 1.25, 1.5, 2, 2.5, 3, 4]):
    place(t, pluck(base * r, 1.6, 0.8, 0.9985), k * 0.09, 0.75)
write('harp.wav', t)
print('ok')
