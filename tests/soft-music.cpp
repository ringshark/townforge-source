// The soft music files decode through raylib (dr_wav, IMA ADPCM), are the expected length, are quiet and do not clip,
// and loop without a jump at the seam.
#include "raylib.h"
#include <cassert>
#include <algorithm>
#include <initializer_list>
#include <cmath>
#include <cstdio>
int main() {
    for (const char* n : { "town", "wild", "dungeon" }) {
        char path[128]; snprintf(path, sizeof path, "assets/music/%s_soft.wav", n);
        Wave w = LoadWave(path);
        assert(w.frameCount > 90 * 22050 && w.sampleRate == 22050 && w.channels == 1);
        WaveFormat(&w, 22050, 16, 1);
        short* s = (short*)w.data; double sq = 0; int peak = 0;
        for (unsigned i = 0; i < w.frameCount; i++) { sq += (double)s[i] * s[i]; peak = std::max(peak, abs((int)s[i])); }
        double rms = sqrt(sq / w.frameCount) / 32768.0;
        double seam = fabs((double)s[0] - s[w.frameCount - 1]) / 32768.0;
        printf("%s: %u frames, rms %.3f, peak %.2f, seam jump %.4f\n", n, w.frameCount, rms, peak / 32768.0, seam);
        assert(peak < 0.4 * 32768 && rms > 0.03 && rms < 0.12 && seam < 0.01);
        UnloadWave(w);
        Music m = LoadMusicStream(path);   // the game streams it
        assert(m.frameCount > 90 * 22050);
        UnloadMusicStream(m);
    }
    puts("PASS soft music decodes, is quiet, and loops cleanly");
}
