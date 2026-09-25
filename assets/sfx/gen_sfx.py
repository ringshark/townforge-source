#!/usr/bin/env python3
"""Synthesize a retro 8-bit SFX pack for Town Forge. Pure stdlib, 22050Hz mono 16-bit WAV."""
import math, os, random, wave

SR = 22050
OUT = os.path.dirname(os.path.abspath(__file__))
random.seed(7)

def env_exp(n, decay):
    return [math.exp(-decay * i / n) for i in range(n)]

def tone(f0, f1, dur, kind="square", vol=0.5, decay=6.0, wobble=None):
    n = int(SR * dur)
    out = [0.0] * n
    phase = 0.0
    for i in range(n):
        t = i / n
        f = f0 + (f1 - f0) * t
        phase += 2 * math.pi * f / SR
        if kind == "square":
            v = 1.0 if math.sin(phase) >= 0 else -1.0
        elif kind == "sine":
            v = math.sin(phase)
        elif kind == "tri":
            v = 2.0 * abs(2.0 * (phase / (2 * math.pi) % 1.0) - 1.0) - 1.0
        else:
            v = math.sin(phase)
        amp = math.exp(-decay * t)
        if wobble:
            amp *= 0.6 + 0.4 * math.sin(2 * math.pi * wobble * t * dur * 4)
        out[i] = v * amp * vol
    return out

def noise(dur, vol=0.5, decay=8.0, smooth=4):
    n = int(SR * dur)
    raw = [random.uniform(-1, 1) for _ in range(n)]
    out = [0.0] * n
    acc = 0.0
    for i in range(n):
        acc += (raw[i] - acc) / smooth
        out[i] = acc * math.exp(-decay * i / n) * vol
    return out

def mix(*tracks):
    n = max(len(t) for t in tracks)
    out = [0.0] * n
    for t in tracks:
        for i, v in enumerate(t):
            out[i] += v
    peak = max(1.0, max(abs(v) for v in out))
    return [v / peak * 0.9 for v in out]

def seq(notes, kind="square", vol=0.4, decay=5.0, gap=0.0):
    """notes: list of (freq, dur)"""
    out = []
    for f, d in notes:
        out += tone(f, f, d, kind, vol, decay)
        if gap:
            out += [0.0] * int(SR * gap)
    return out

def save(name, samples):
    pcm = b"".join(int(max(-1, min(1, s)) * 32767).to_bytes(2, "little", signed=True) for s in samples)
    with wave.open(os.path.join(OUT, name), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes(pcm)
    print(name, len(samples) / SR, "s")

# --- the pack ---
save("swing.wav",  noise(0.14, vol=0.55, decay=14.0, smooth=6))
save("hit.wav",    mix(noise(0.10, vol=0.6, decay=16.0, smooth=3),
                        tone(170, 65, 0.20, "square", 0.5, 9.0)))
save("cast.wav",   tone(240, 1450, 0.35, "square", 0.35, 4.0))
save("fireball.wav", mix(tone(950, 110, 0.45, "square", 0.45, 5.0),
                          noise(0.45, vol=0.25, decay=5.0, smooth=8)))
save("heal.wav",   seq([(523, .12), (659, .12), (784, .12), (1047, .25)], "sine", 0.5, 4.0))
save("coin.wav",   seq([(988, .07), (1319, .28)], "square", 0.35, 6.0))
save("monster_die.wav", tone(360, 55, 0.45, "square", 0.5, 5.0))
save("hurt.wav",   mix(tone(230, 75, 0.22, "square", 0.5, 8.0),
                        noise(0.15, vol=0.35, decay=12.0, smooth=3)))
save("click.wav",  tone(760, 760, 0.055, "square", 0.3, 18.0))
save("victory.wav", seq([(523,.12),(659,.12),(784,.12),(1047,.35)], "square", 0.35, 5.0))
save("ghost.wav",  tone(420, 75, 0.9, "sine", 0.55, 2.5, wobble=6.0))
save("door.wav",   seq([(120, .09), (95, .12)], "square", 0.5, 10.0, gap=0.06))
save("hunt.wav",   mix(tone(98, 98, 0.85, "square", 0.4, 1.8),
                        tone(103.8, 103.8, 0.85, "square", 0.4, 1.8)))
save("buy.wav",    seq([(784, .08), (988, .08), (1175, .2)], "square", 0.35, 6.0))
save("quest.wav",  mix(tone(392, 392, 0.5, "sine", 0.4, 3.5),
                        tone(494, 494, 0.5, "sine", 0.4, 3.5),
                        tone(587, 587, 0.5, "sine", 0.4, 3.5)))
print("done")
