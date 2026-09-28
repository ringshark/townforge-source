#version 100

// Town Forge 3D "lit" shader (vertex stage) — 2026-09-24.
// Identical to assets/shaders/shadowmap.vs (no shadow-specific logic lives
// in the vertex stage anyway); kept as its own file so lit.vs/lit.fs is a
// self-contained pair, paired with assets/shaders/lit.fs (the shadow-map
// sampling stripped out of shadowmap.fs, keeping only the always-safe
// diffuse/specular/fog lighting).

// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// Procedural gait for the unrigged sculpted monsters (2026-09-28). Zero for
// every other model, so they are untouched.
// walk:    x = stride phase (radians), y = amount 0..1, z = 1 for many-legged
// walkBox: model-space x = feet y, y = height, z = half width, w = stride
uniform vec4 walk;
uniform vec4 walkBox;

// Output vertex attributes (to fragment shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

vec3 Gait(vec3 p)
{
    float H = walkBox.y;
    float up = (p.y - walkBox.x)/H;          // 0 at the feet, 1 at the top
    if (walk.z < 0.5) {
        // two legs: below the hips each side swings fore and aft in turn,
        // most at the foot; the leg coming forward lifts off the ground
        float k = clamp((0.44 - up)/0.44, 0.0, 1.0);
        float side = clamp(p.x/(0.05*H), -1.0, 1.0);
        float s = sin(walk.x), c = cos(walk.x);
        p.z += side*s*walkBox.w*k*walk.y;
        p.y += max(0.0, side*c)*0.07*H*k*k*walk.y;
        // arms swing against the legs, a little
        float arm = clamp((abs(p.x)/walkBox.z - 0.55)/0.25, 0.0, 1.0)*clamp((0.78 - up)/0.3, 0.0, 1.0)*step(0.4, up);
        p.z -= side*s*walkBox.w*0.45*arm*walk.y;
    } else {
        // many legs: two alternating sets (by diagonal quadrant) step in turn
        float r = length(p.xz)/walkBox.z;
        float k = clamp((r - 0.3)/0.5, 0.0, 1.0)*clamp((0.6 - up)/0.6, 0.0, 1.0);
        float g = sign(p.x)*sign(p.z);
        float ph = walk.x + (g > 0.0 ? 0.0 : 3.14159);
        p.y += max(0.0, sin(ph))*0.12*H*k*walk.y;
        p.z += cos(ph)*walkBox.w*k*walk.y;
    }
    return p;
}

void main()
{
    vec3 pos = vertexPosition;
    if (walk.y > 0.0) pos = Gait(pos);
    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel*vec4(pos, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 0.0)));

    // Calculate final vertex position
    gl_Position = mvp*vec4(pos, 1.0);
}
