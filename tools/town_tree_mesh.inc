// Cached town trees, authored in the same meter scale as the Nature Kit.
// One vertex-colored mesh per species; no textures or per-frame allocations.
static Model Town3DBuildTree(int kind) {
    T3CMeshBuilder b;
    const Color bark{ 106, 77, 51, 255 };
    const float heights[] = { 1.18f, 1.50f, 1.28f, 1.66f, 1.10f };
    const float widths[] = { 0.36f, 0.26f, 0.41f, 0.37f, 0.37f };
    float h = heights[kind], w = widths[kind];
    T3CCylinder(b, 0, 0, 0, h * 0.74f, 0.065f, 0.023f, 8, bark);
    T3CCylinder(b, 0, 0, 0, 0.13f, 0.10f, 0.06f, 8, bark);
    if (kind == 1) {
        // Layered evergreen skirts with offset tips rather than a single cone.
        for (int i = 0; i < 5; ++i) {
            float t = i / 4.0f;
            float base[3] = { 0.018f * sinf(i * 2.4f), h * (0.23f + 0.135f * i), 0 };
            float dir[3] = { 0.03f * cosf(i * 1.7f), 1, 0.02f };
            T3CConeDir(b, base, dir, h * (0.31f - t * 0.08f), w * (1.0f - t * 0.69f),
                       9, Color{ (unsigned char)(47 + i * 4), (unsigned char)(104 + i * 7),
                                 (unsigned char)(67 + i * 2), 255 });
        }
    } else {
        // Forked branches remain visible between staggered overlapping leaf masses.
        for (int i = 0; i < 7; ++i) {
            float a = i * 2.399963f + kind * 0.7f;
            float tier = i / 6.0f;
            float r = w * (0.43f - tier * 0.13f);
            float y = h * (0.50f + tier * 0.28f);
            float base[3] = { 0, y - h * 0.19f, 0 };
            float dir[3] = { r * cosf(a), h * 0.19f, r * sinf(a) };
            T3CConeDir(b, base, dir, sqrtf(r * r + h * h * 0.0361f), 0.025f, 6, bark);
            T3CSphere(b, r * cosf(a), y, r * sinf(a),
                      w * (0.56f - tier * 0.06f), h * 0.185f, w * 0.53f,
                      5, 8, Color{ (unsigned char)(70 + i * 3 + kind * 2),
                                   (unsigned char)(125 + i * 5), (unsigned char)(57 + i * 2), 255 });
        }
        T3CSphere(b, -w * 0.06f, h * 0.84f, w * 0.03f, w * 0.54f, h * 0.16f,
                  w * 0.52f, 5, 8, Color{ 108, 159, 72, 255 });
    }
    // Small color variation per face and darker crown undersides give volume
    // without making each leaf cluster a separate material/draw call.
    for (size_t v = 0; v < b.pos.size() / 3; ++v) {
        if (b.col[v * 4 + 1] < 100) continue; // leave the bark alone
        float shade = 0.88f + 0.12f * (b.nor[v * 3 + 1] * 0.5f + 0.5f)
                    + 0.06f * T3CHash01((float)(v / 3), (float)kind);
        for (int c = 0; c < 3; ++c) b.col[v * 4 + c] = (unsigned char)(b.col[v * 4 + c] * shade);
    }
    return T3CFinish(b);
}
