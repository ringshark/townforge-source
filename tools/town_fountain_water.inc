// Upward-facing annulus, also used for the carved rim and impact foam.
static void TPWaterRing(T3CMeshBuilder& b, float x, float y, float z,
                        float inner, float outer, Color c) {
    for (int i = 0; i < 32; ++i) {
        float a = i * 6.2831853f / 32, d = (i + 1) * 6.2831853f / 32;
        float p[4][3] = {{x + inner*cosf(a), y, z + inner*sinf(a)},
                         {x + inner*cosf(d), y, z + inner*sinf(d)},
                         {x + outer*cosf(d), y, z + outer*sinf(d)},
                         {x + outer*cosf(a), y, z + outer*sinf(a)}};
        T3CQuad(b, p[0], p[1], p[2], p[3], c);
    }
}
static Model TPBuildWater() {
    T3CMeshBuilder b;
    const Color water{ 62, 132, 153, 255 }, foam{ 163, 216, 219, 255 };
    TPWaterRing(b, 0, 15.35f, 0, 7.1f, 44, water);
    TPWaterRing(b, 0, 36.55f, 0, 0, 16, water);
    // Six curved spill streams and their impact rings, merged into one mesh.
    // Closed narrow tubes are visible from every orbit-camera angle.
    for (int i = 0; i < 6; ++i) {
        float a = i * 6.2831853f / 6;
        for (int j = 0; j < 8; ++j) {
            float t0 = j / 8.0f, t1 = (j + 1) / 8.0f;
            float r0 = 15.7f + 15 * t0, r1 = 15.7f + 15 * t1;
            float f0 = std::max(0.0f, (t0 - 0.25f) / 0.75f), f1 = std::max(0.0f, (t1 - 0.25f) / 0.75f);
            float y0 = 36.55f - 21.2f * f0 * f0, y1 = 36.55f - 21.2f * f1 * f1;
            float p0[4][3], p1[4][3];
            for (int k = 0; k < 4; ++k) {
                float q = k * 6.2831853f / 4;
                float side = cosf(q) * 0.85f, dy = sinf(q) * 0.65f;
                p0[k][0] = r0*cosf(a) - side*sinf(a); p0[k][1] = y0 + dy; p0[k][2] = r0*sinf(a) + side*cosf(a);
                p1[k][0] = r1*cosf(a) - side*sinf(a); p1[k][1] = y1 + dy; p1[k][2] = r1*sinf(a) + side*cosf(a);
            }
            for (int k = 0; k < 4; ++k) T3CQuad(b, p0[k], p1[k], p1[(k+1)%4], p0[(k+1)%4], foam);
        }
        TPWaterRing(b, 30.7f*cosf(a), 15.50f, 30.7f*sinf(a), 1.2f, 3.0f, foam);
    }
    return T3CFinish(b);
}
static Model g_townFountainWater{};
static Shader g_townWaterShader{};
static int g_townWaterTimeLoc = -1;
static void TPWaterShaderEnsure() {
    g_townWaterShader = LoadShader("assets/shaders/lit.vs", "assets/shaders/fountain.fs");
    // raylib returns the default shader on compilation failure.
    if (g_townWaterShader.id == 0 || g_townWaterShader.id == rlGetShaderIdDefault()) {
        g_townWaterShader = {}; Town3DApplyLitShader(g_townFountainWater); return;
    }
    g_townWaterShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(g_townWaterShader, "viewPos");
    g_townWaterTimeLoc = GetShaderLocation(g_townWaterShader, "time");
    T3DSetLightUniforms(g_townWaterShader);
    for (int i = 0; i < g_townFountainWater.materialCount; ++i)
        g_townFountainWater.materials[i].shader = g_townWaterShader;
}
