#version 100

precision mediump float;

// Town Forge 3D dungeon torch-light shader (fragment stage).
// Dungeons are indoors, so the town's sun-directional shadow system is wrong
// for them. Instead: a dark cool ambient plus up to 8 warm flickering point
// lights at the dungeon's torch positions — no shadowmaps, one cheap forward
// pass, mobile-web friendly.
//
// Lighting is half-Lambert-ish ((0.25 + 0.75*NdotL)) so wall faces pointing
// away from every torch still read instead of going pitch black, and the
// whole thing is gamma-corrected like the sun shader.
//
// fragColor (per-vertex color) is honored, so raylib primitives keep their
// colors; meshes without vertex colors default to white (see
// rlSetVertexAttributeDefault in rmodels.c), so textured models are unaffected.

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec4 ambient;          // dark cool ambient, set once at load

// Torch lights (world-space positions at flame height)
uniform vec3 torchPos[8];
uniform int torchCount;
uniform float time;            // seconds, for the flame flicker

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture2D(texture0, fragTexCoord)*fragColor;
    vec3 albedo = texelColor.rgb*colDiffuse.rgb;
    // Immediate-mode primitives (DrawSphere/DrawCapsule) don't set per-vertex
    // normals, so fragNormal can be a stale or zero vector there — fall back to
    // up instead of normalizing a zero vector into NaN.
    vec3 normal = fragNormal;
    float nl = length(normal);
    normal = (nl > 0.0001) ? (normal/nl) : vec3(0.0, 1.0, 0.0);

    vec3 col = albedo*ambient.rgb;

    vec3 warm = vec3(1.0, 0.52, 0.20); // torch-flame orange
    for (int i = 0; i < 8; i++)
    {
        if (i >= torchCount) break;
        vec3 L = torchPos[i] - fragPosition;
        float d = max(length(L), 0.001); // never divide by zero at the flame itself
        L /= d;
        float diff = max(dot(normal, L), 0.0);
        float att = 1.0/(1.0 + 0.000045*d*d); // ~useful radius 400 units
        float flick = 0.82 + 0.18*sin(time*9.0 + float(i)*2.3)*sin(time*5.7 + float(i)*1.1);
        col += albedo*warm*(0.25 + 0.75*diff)*att*flick*1.7;
    }

    // Gamma correction (matches the sun shader's display-space output)
    col = pow(col, vec3(1.0/2.2));

    gl_FragColor = vec4(col, texelColor.a*colDiffuse.a);
}
