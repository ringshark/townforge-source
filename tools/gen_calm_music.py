#!/usr/bin/env python3
"""Calm music (2026-09-29): three slow, looping ambient pieces synthesized in-house,
so no licence to track. Soft pads, a sparse plucked harp or flute, bells in the deep.

    python3 tools/gen_calm_music.py      # writes assets/music/{town,wild,dungeon}_calm.ogg

Needs numpy and soundfile (pip install soundfile - it bundles an Ogg Vorbis encoder).
"""
import numpy as np
import soundfile as sf

SR = 22050
rng = np.random.default_rng(7)


def midi(n):
    return 440.0 * 2 ** ((n - 69) / 12.0)


def env(n, attack, release):
    e = np.ones(n)
    a, r = min(n, int(attack * SR)), min(n, int(release * SR))
    if a: e[:a] = np.sin(np.linspace(0, np.pi / 2, a)) ** 2
    if r: e[-r:] *= np.cos(np.linspace(0, np.pi / 2, r)) ** 2
    return e


def pad(freq, dur, amp):
    t = np.arange(int(dur * SR)) / SR
    w = (np.sin(2 * np.pi * freq * t) + 0.5 * np.sin(2 * np.pi * freq * 1.003 * t + 1.0)
         + 0.25 * np.sin(2 * np.pi * freq * 2 * t) + 0.12 * np.sin(2 * np.pi * freq * 0.997 * 2 * t))
    wob = 1.0 + 0.08 * np.sin(2 * np.pi * 0.15 * t + rng.random() * 6)  # slow breathing
    return amp * w * wob * env(len(t), 2.5, 3.0) / 1.9


def pluck(freq, dur, amp):
    t = np.arange(int(dur * SR)) / SR
    w = np.zeros_like(t)
    for k in range(1, 7):
        w += np.sin(2 * np.pi * freq * k * t) * np.exp(-t * (1.2 + 1.3 * k)) / k ** 1.4
    return amp * w * env(len(t), 0.004, 0.3)


def flute(freq, dur, amp):
    t = np.arange(int(dur * SR)) / SR
    vib = 1.0 + 0.004 * np.sin(2 * np.pi * 4.8 * t) * np.clip(t - 0.4, 0, 1)
    ph = 2 * np.pi * np.cumsum(freq * vib) / SR
    w = np.sin(ph) + 0.18 * np.sin(2 * ph) + 0.05 * np.sin(3 * ph)
    breath = rng.normal(0, 0.006, len(t))
    return amp * (w + breath) * env(len(t), 0.35, 0.8)


def bell(freq, dur, amp):
    t = np.arange(int(dur * SR)) / SR
    w = np.zeros_like(t)
    for ratio, a, d in ((1.0, 1.0, 0.9), (2.76, 0.45, 1.6), (5.4, 0.2, 2.8), (0.5, 0.35, 0.6)):
        w += a * np.sin(2 * np.pi * freq * ratio * t) * np.exp(-t * d)
    return amp * w * env(len(t), 0.01, 0.5) / 2.0


def add(buf, at, sig):
    i = int(at * SR) % len(buf)
    n = min(len(sig), len(buf) - i)
    buf[i:i + n] += sig[:n]
    if n < len(sig):  # wrap past the end: the loop stays seamless
        buf[:len(sig) - n] += sig[n:]


def reverb(x, seconds=2.8, wet=0.35):
    n = int(seconds * SR)
    ir = rng.normal(0, 1, n) * np.exp(-np.linspace(0, 7, n))
    ir[:int(0.02 * SR)] = 0
    ir /= np.sqrt(np.sum(ir ** 2))
    L = len(x) + n
    size = 1 << (L - 1).bit_length()
    y = np.fft.irfft(np.fft.rfft(x, size) * np.fft.rfft(ir, size), size)[:L]
    y[:n] += y[len(x):len(x) + n]  # wrap the tail round to the start
    return (1 - wet) * x + wet * y[:len(x)] * 0.5


def finish(buf, name, peak=0.55):
    buf = reverb(buf)
    buf *= peak / max(1e-9, np.max(np.abs(buf)))
    x = buf.astype(np.float32)
    with sf.SoundFile(f"assets/music/{name}_calm.ogg", "w", SR, 1, format="OGG", subtype="VORBIS",
                      compression_level=0.75) as f:
        for i in range(0, len(x), 4096):  # one big write crashes libsndfile's Vorbis encoder
            f.write(x[i:i + 4096])
    print(name, f"{len(buf) / SR:.0f}s")


def piece(chords, chord_len, repeats, voice, scale, notes_per_chord, voice_amp, rest=0.35, voice_dur=2.6):
    total = chord_len * len(chords) * repeats
    buf = np.zeros(int(total * SR))
    for r in range(repeats):
        for c, chord in enumerate(chords):
            at = (r * len(chords) + c) * chord_len
            for n in chord:
                add(buf, at, pad(midi(n), chord_len + 3.0, 0.09))
            step = chord_len / notes_per_chord
            for k in range(notes_per_chord):
                if rng.random() < rest:
                    continue
                pool = [n for n in scale if (n % 12) in [m % 12 for m in chord]] if rng.random() < 0.6 else scale
                pool = pool or scale
                n = pool[rng.integers(len(pool))]
                add(buf, at + k * step + rng.normal(0, 0.03), voice(midi(n), voice_dur, voice_amp * (0.7 + 0.3 * rng.random())))
    return buf


if __name__ == "__main__":
    # Town: warm C major, a slow harp. I - vi - IV - V - I - iii - IV - V
    C = [[48, 55, 64], [45, 52, 60], [41, 53, 57], [43, 50, 59], [48, 55, 64], [40, 52, 59], [41, 53, 57], [43, 50, 62]]
    town = piece(C, 6.0, 3, pluck, [67, 69, 72, 74, 76, 79, 81], 6, 0.22)
    finish(town, "town")
    # Wilds: airy D, a lone flute over open fifths.
    D = [[38, 45, 57], [43, 50, 59], [36, 43, 55], [38, 45, 54], [40, 47, 55], [43, 50, 59], [36, 43, 52], [38, 45, 57]]
    wild = piece(D, 7.0, 3, flute, [69, 71, 74, 76, 78, 81], 3, 0.16, rest=0.3, voice_dur=3.2)
    finish(wild, "wild")
    # Dungeon: a low drone in A minor, far-off bells.
    A = [[33, 40, 48], [33, 40, 45], [29, 36, 45], [31, 38, 47], [33, 40, 48], [28, 35, 43], [29, 36, 45], [33, 40, 45]]
    dung = piece(A, 8.0, 2, bell, [57, 60, 62, 64, 67, 69, 72], 2, 0.2, rest=0.45, voice_dur=5.0)
    finish(dung, "dungeon", peak=0.5)
