#version 100

// highp where the GPU offers it: the detail UVs are world position / 128,
// up to ~25, and fp16 "mediump" (honored literally on most phones) would
// quantize them into visibly blocky texture far from the world origin.
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

// Town Forge 3D ground shader (fragment stage) — 2026-09-26 environment pass.
// Paired with lit.vs. The baked ground map (texture0) holds the large-scale
// colors — meadow blotches, roads, plaza, water, baked building/tree shadows —
// and its alpha channel says what the surface is made of:
//   1.0 grass   ~0.5 packed dirt   0.0 cobblestone
// Two small tiling detail textures add the close-up texture the 1.5 world
// units/pixel ground map can't hold: grass (texture1, rgb) and soil
// (texture2: r = cobbles, g = dirt and pebbles). Detail is tiled in world
// space and fades to neutral with distance so it never shimmers far away.
// Lighting and fog are the same as lit.fs.

varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

uniform sampler2D texture0; // baked ground map
uniform sampler2D texture1; // grass detail
uniform sampler2D texture2; // soil detail (r cobbles, g dirt)
uniform vec4 colDiffuse;

uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec4 ambient;
uniform vec3 viewPos;

uniform vec3 fogColor;
uniform vec2 fogRange;

void main()
{
    vec4 mac = texture2D(texture0, fragTexCoord);
    vec2 wuv = fragPosition.xz * (1.0 / 128.0);
    vec3 grassD = texture2D(texture1, wuv).rgb;
    vec4 soilD = texture2D(texture2, wuv * 0.8);
    // A second, larger-scale grass sample breaks up visible tiling.
    grassD = mix(grassD, texture2D(texture1, wuv * 0.37 + vec2(0.31, 0.17)).rgb, 0.35);

    float a = mac.a;
    float wGrass = smoothstep(0.62, 0.95, a);
    float wStone = 1.0 - smoothstep(0.10, 0.35, a);
    float wDirt = clamp(1.0 - wGrass - wStone, 0.0, 1.0);
    // Cobbles at a finer scale than the dirt, and softened: stones ~8 units across.
    float cob = mix(0.5, texture2D(texture2, wuv * 2.6 + vec2(0.5, 0.25)).r, 0.7);
    vec3 det = grassD * wGrass + vec3(cob) * wStone + vec3(soilD.g) * wDirt;

    float fogDist = length(viewPos - fragPosition);
    det = mix(det, vec3(0.5), smoothstep(900.0, 1900.0, fogDist));

    vec3 base = clamp(mac.rgb * det * 2.0, 0.0, 1.0) * fragColor.rgb * colDiffuse.rgb;
    vec3 albedo = pow(base, vec3(2.2));

    vec3 normal = normalize(fragNormal);
    vec3 l = -lightDir;
    float NdotL = max(dot(normal, l), 0.0);
    float up = normal.y * 0.5 + 0.5;
    vec3 amb = ambient.rgb * 0.75 * mix(vec3(0.55, 0.50, 0.45), vec3(1.05, 1.08, 1.18), up);
    vec3 col = albedo * (lightColor.rgb * 0.80 * NdotL + amb);

    col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
    float fogF = smoothstep(fogRange.x, fogRange.y, fogDist);
    col = mix(col, fogColor, clamp(fogF, 0.0, 1.0));
    gl_FragColor = vec4(col, 1.0);
}
