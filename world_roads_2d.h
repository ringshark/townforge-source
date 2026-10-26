#pragma once
// Roads and streets for the connected 2D world.
//  - Wilderness roads are soft-edged dirt ribbons: a darker verge that melts into the grass, a worn
//    centre, two wheel ruts, and pebbles and tufts along the edges. The edge wanders a little so a road
//    never looks ruled with a pen.
//  - Town streets are cobble with a stone kerb. All streets are axis-aligned (towns face N/E/S/W), so
//    each is drawn as a rectangle: kerbs of every street first, then every street's cobble over them,
//    which leaves junctions clean.
namespace tfroad {
static Texture2D dirt{}, cobble{}, flag{};
static constexpr int kN = 128;

inline float Hash(int x, int y, int seed) {
    unsigned h = unsigned(x)*374761393u + unsigned(y)*668265263u + unsigned(seed)*2246822519u;
    h = (h^(h>>13))*1274126177u;
    return float(h^(h>>16))/float(0xffffffffu);
}
// Value noise on a lattice that wraps every `cells` cells, so the finished texture tiles.
inline float Tile(float fx, float fy, int cells, int seed) {
    float gx = fx*cells, gy = fy*cells;
    int x0 = (int)floorf(gx), y0 = (int)floorf(gy);
    float tx = gx-x0, ty = gy-y0;
    tx = tx*tx*(3-2*tx); ty = ty*ty*(3-2*ty);
    auto at = [&](int x, int y) { return Hash(((x%cells)+cells)%cells, ((y%cells)+cells)%cells, seed); };
    float a = at(x0,y0), b = at(x0+1,y0), c = at(x0,y0+1), d = at(x0+1,y0+1);
    return (a+(b-a)*tx)*(1-ty) + (c+(d-c)*tx)*ty;
}
inline unsigned char Byte(float v) { return (unsigned char)std::clamp(v, 0.f, 255.f); }

inline void Build() {
    if (dirt.id) return;
    {   // packed earth: broad patches, mid mottling, fine grain, a few stones and dark fibres
        Image img = GenImageColor(kN, kN, BLANK);
        auto* px = (Color*)img.data;
        for (int y = 0; y < kN; y++) for (int x = 0; x < kN; x++) {
            float fx = x/(float)kN, fy = y/(float)kN;
            float broad = Tile(fx,fy,4,1)-.5f, mid = Tile(fx,fy,16,2)-.5f, fine = Hash(x,y,3)-.5f;
            float v = broad*26 + mid*16 + fine*10;
            float r = 164+v, g = 133+v*.9f, b = 91+v*.75f;
            if (Hash(x/2, y/2, 4) > .985f) { r -= 34; g -= 32; b -= 26; }          // dark pebble
            else if (Hash(x/2, y/2, 5) > .987f) { r += 26; g += 24; b += 20; }      // pale pebble
            if (Hash(x/5, y, 6) > .975f) { r -= 14; g -= 13; b -= 11; }             // dry fibre streak
            px[y*kN+x] = { Byte(r), Byte(g), Byte(b), 255 };
        }
        dirt = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureWrap(dirt, TEXTURE_WRAP_REPEAT); SetTextureFilter(dirt, TEXTURE_FILTER_BILINEAR);
    }
    {   // cobble: 16 px stones laid in running bond, each with its own shade and a bevel
        Image img = GenImageColor(kN, kN, BLANK);
        auto* px = (Color*)img.data;
        const int S = 16;
        for (int y = 0; y < kN; y++) for (int x = 0; x < kN; x++) {
            int row = y/S, xx = (x + (row%2)*(S/2)) % kN;
            int cx = xx/S, lx = xx%S, ly = y%S;
            float shade = (Hash(cx, row, 7)-.5f)*30;
            float warm = (Hash(cx, row, 8)-.5f)*10;
            float edge = std::min(std::min(lx, S-1-lx), std::min(ly, S-1-ly));
            float r = 150+shade+warm, g = 144+shade, b = 130+shade-warm;
            if (edge == 0) { r = 88; g = 84; b = 76; }                                // mortar
            else if (edge == 1) { r -= 18; g -= 18; b -= 16; }                         // rounded shoulder
            else if (lx == 2 || ly == 2) { r += 12; g += 12; b += 10; }               // lit top-left bevel
            float grain = (Hash(x,y,9)-.5f)*8;
            px[y*kN+x] = { Byte(r+grain), Byte(g+grain), Byte(b+grain), 255 };
        }
        cobble = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureWrap(cobble, TEXTURE_WRAP_REPEAT); SetTextureFilter(cobble, TEXTURE_FILTER_POINT);
    }
    {   // pale flagstone: the kerb and the gate apron
        Image img = GenImageColor(kN, kN, BLANK);
        auto* px = (Color*)img.data;
        for (int y = 0; y < kN; y++) for (int x = 0; x < kN; x++) {
            float v = (Tile(x/(float)kN, y/(float)kN, 8, 11)-.5f)*22 + (Hash(x,y,12)-.5f)*8;
            bool joint = (x%32) == 0 || (y%32) == 0;
            px[y*kN+x] = { Byte(176+v-(joint?46:0)), Byte(172+v-(joint?46:0)), Byte(154+v-(joint?42:0)), 255 };
        }
        flag = LoadTextureFromImage(img); UnloadImage(img);
        SetTextureWrap(flag, TEXTURE_WRAP_REPEAT); SetTextureFilter(flag, TEXTURE_FILTER_POINT);
    }
}

// ---- Ribbons --------------------------------------------------------------------------------------------
struct Ribbon { std::vector<Vector2> p, n; std::vector<float> hw; };

// A road polyline with a smooth normal at every point and a half-width that drifts a few units.
inline Ribbon MakeRibbon(const std::vector<Vector2>& pts, float halfWidth, float wander) {
    Ribbon r; r.p = pts;
    size_t m = pts.size();
    r.n.resize(m); r.hw.resize(m);
    float along = 0;
    for (size_t i = 0; i < m; i++) {
        Vector2 a = pts[i > 0 ? i-1 : i], b = pts[i+1 < m ? i+1 : i];
        float dx = b.x-a.x, dy = b.y-a.y, l = hypotf(dx, dy);
        if (l < .001f) { dx = 1; dy = 0; l = 1; }
        r.n[i] = { -dy/l, dx/l };
        if (i > 0) along += hypotf(pts[i].x-pts[i-1].x, pts[i].y-pts[i-1].y);
        float w = (Tile(fmodf(along/900.f, 1.f), .37f, 12, 21) - .5f)*2.f;       // smooth drift along the road
        r.hw[i] = halfWidth + w*wander;
    }
    return r;
}
// One lengthwise band between lateral offsets (fraction of half-width + absolute) with an alpha at each side.
struct Band { float f0, a0, f1, a1; float alpha0, alpha1; };
inline void DrawBand(const Ribbon& r, const Band& b, Color tint, const Texture2D& tex, float tile, size_t from, size_t to) {
    rlDisableBackfaceCulling();
    for (size_t i = from; i + 1 <= to && i + 1 < r.p.size(); i++) {
        size_t j = i + 1;
        auto vert = [&](size_t k, float f, float add, float alpha) {
            Vector2 q = { r.p[k].x + r.n[k].x*(f*r.hw[k]+add), r.p[k].y + r.n[k].y*(f*r.hw[k]+add) };
            rlColor4ub(tint.r, tint.g, tint.b, (unsigned char)(tint.a*alpha));
            rlTexCoord2f(q.x/tile, q.y/tile);
            rlVertex2f(q.x, q.y);
        };
        rlSetTexture(tex.id);
        rlBegin(RL_QUADS);
        vert(i, b.f0, b.a0, b.alpha0); vert(i, b.f1, b.a1, b.alpha1);
        vert(j, b.f1, b.a1, b.alpha1); vert(j, b.f0, b.a0, b.alpha0);
        rlEnd();
        rlSetTexture(0);
    }
    rlEnableBackfaceCulling();
}
// The part of a ribbon that can be on screen (points are ~20 apart, so scanning is cheap).
inline bool Visible(const Ribbon& r, Rectangle view, size_t& from, size_t& to) {
    bool any = false; from = 0; to = 0;
    for (size_t i = 0; i < r.p.size(); i++) {
        bool in = r.p[i].x > view.x-80 && r.p[i].x < view.x+view.width+80 && r.p[i].y > view.y-80 && r.p[i].y < view.y+view.height+80;
        if (!in) continue;
        if (!any) { from = i > 0 ? i-1 : 0; any = true; }
        to = std::min(i+1, r.p.size()-1);
    }
    return any;
}
inline Color Mul(Color a, Color b) { return { (unsigned char)(a.r*b.r/255), (unsigned char)(a.g*b.g/255), (unsigned char)(a.b*b.b/255), (unsigned char)(a.a*b.a/255) }; }
static const Texture2D& White() { static Texture2D w{}; if (!w.id) w = LoadTextureFromImage(GenImageColor(2,2,WHITE)); return w; }

// All the wilderness roads. `view` is the visible world rectangle (camera top-left and logical size).
// Everything here is positioned in world units; the camera moves the whole drawing onto the screen.
inline void PushWorld(Vector2 camera) { rlPushMatrix(); rlTranslatef(kViewport.x - camera.x, kViewport.y - camera.y, 0); }
inline void PopWorld() { rlPopMatrix(); }

inline void DrawWildRoads(const std::vector<std::vector<Vector2>>& roads, Rectangle view, float time) {
    Build();
    PushWorld({ view.x, view.y });
    static std::vector<Ribbon> cache; static size_t cachedFor = 0;
    if (cachedFor != roads.size()) {
        cache.clear();
        for (const auto& road : roads) cache.push_back(MakeRibbon(road, 20.f, 3.2f));
        cachedFor = roads.size();
    }
    const Color verge = { 78, 64, 40, 255 }, soil = { 255, 255, 255, 255 }, worn = { 236, 214, 166, 255 }, rut = { 74, 55, 34, 255 };
    // 1. the verge: churned earth that melts into the grass
    for (const auto& r : cache) { size_t a, b; if (!Visible(r, view, a, b)) continue;
        DrawBand(r, {-1, -11, -1, 3, 0.f, .55f}, verge, White(), 64, a, b);
        DrawBand(r, { 1, -3,  1, 11, .55f, 0.f}, verge, White(), 64, a, b); }
    // 2. the body of the road
    for (const auto& r : cache) { size_t a, b; if (!Visible(r, view, a, b)) continue;
        DrawBand(r, {-1, 0, -1, 7, .0f, 1.f}, soil, dirt, 128, a, b);
        DrawBand(r, {-1, 7,  1, -7, 1.f, 1.f}, soil, dirt, 128, a, b);
        DrawBand(r, { 1, -7, 1, 0, 1.f, .0f}, soil, dirt, 128, a, b); }
    // 3. the worn, paler centre and the two wheel ruts
    for (const auto& r : cache) { size_t a, b; if (!Visible(r, view, a, b)) continue;
        DrawBand(r, {-.55f, 0, 0, 0, 0.f, .30f}, worn, White(), 64, a, b);
        DrawBand(r, { 0, 0, .55f, 0, .30f, 0.f}, worn, White(), 64, a, b);
        for (float side : {-.36f, .36f}) {
            DrawBand(r, {side, -3.f, side, 0, 0.f, .16f}, rut, White(), 64, a, b);
            DrawBand(r, {side, 0, side, 3.f, .16f, 0.f}, rut, White(), 64, a, b);
        } }
    // 4. pebbles and grass tufts along the edges, the same every frame
    for (const auto& r : cache) { size_t a, b; if (!Visible(r, view, a, b)) continue;
        for (size_t i = a; i <= b && i < r.p.size(); i++) {
            int ix = (int)r.p[i].x, iy = (int)r.p[i].y;
            for (int side = -1; side <= 1; side += 2) {
                float h = Hash(ix/4, iy/4, 30 + side);
                if (h > .55f) continue;
                float off = r.hw[i] + 1.f + Hash(ix, iy, 31+side)*8.f;
                Vector2 q = { r.p[i].x + r.n[i].x*off*side, r.p[i].y + r.n[i].y*off*side };
                if (h < .22f) {                                           // tuft
                    Color g = { (unsigned char)(70+h*120), (unsigned char)(104+h*100), 58, 230 };
                    DrawLineEx({q.x-1.5f, q.y}, {q.x-2.5f, q.y-5}, 1.3f, g);
                    DrawLineEx({q.x, q.y}, {q.x, q.y-7}, 1.3f, g);
                    DrawLineEx({q.x+1.5f, q.y}, {q.x+2.5f, q.y-5}, 1.3f, g);
                } else {                                                   // pebble
                    float s = 1.2f + h*3.f;
                    DrawEllipse((int)q.x, (int)q.y, s, s*.7f, Color{ 118, 108, 92, 220 });
                    DrawEllipse((int)q.x, (int)(q.y-1), s*.6f, s*.4f, Color{ 168, 158, 138, 200 });
                }
            }
        } }
    PopWorld();
}

// ---- Town streets ---------------------------------------------------------------------------------------
struct Street { Rectangle r; };
// A tiled textured rectangle: the source rectangle is the world rectangle itself, so the pattern is continuous.
inline void DrawTextured(const Texture2D& tex, Rectangle r, Color tint) {
    DrawTexturePro(tex, { r.x, r.y, r.width, r.height }, r, { 0, 0 }, 0, tint);
}
inline Rectangle Inflate(Rectangle r, float d) { return { r.x-d, r.y-d, r.width+2*d, r.height+2*d }; }
// Draws the streets as one cobbled network with a kerb. `shade` tints the stone (snow, harbour planks and so on).
inline void DrawStreets(const std::vector<Rectangle>& streets, Color shade, bool plank, Vector2 camera) {
    Build();
    PushWorld(camera);
    for (const auto& r : streets) DrawRectangleRec(Inflate(r, 8), Color{ 20, 20, 14, 38 });          // soft shadow
    for (const auto& r : streets) DrawTextured(flag, Inflate(r, 4), Mul(shade, Color{ 232, 228, 214, 255 }));   // kerb
    for (const auto& r : streets) DrawRectangleRec(Inflate(r, 1.6f), Color{ 60, 56, 48, 255 });      // shadow line under the kerb's inner edge
    for (const auto& r : streets) DrawTextured(plank ? dirt : cobble, r, shade);
    PopWorld();
}
}
