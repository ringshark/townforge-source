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
uniform float time;      // seconds: cloud drift and water ripples
uniform vec3 skyColor;   // zenith tint, reflected by water

// Smooth value noise for the drifting cloud shadows (highp world coordinates).
float hash2(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float vnoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash2(i), b = hash2(i + vec2(1.0, 0.0)), c = hash2(i + vec2(0.0, 1.0)), d = hash2(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main()
{
    vec4 mac = texture2D(texture0, fragTexCoord);
    vec2 wuv = fragPosition.xz * (1.0 / 128.0);
    vec3 grassD = texture2D(texture1, wuv).rgb;
    vec4 soilD = texture2D(texture2, wuv * 0.8);
    // A second, larger-scale grass sample breaks up visible tiling.
    grassD = mix(grassD, texture2D(texture1, wuv * 0.37 + vec2(0.31, 0.17)).rgb, 0.35);

    float a = mac.a;
    // alpha ~0.25 marks open water (grass 1, dirt 0.5, stone 0)
    float wWater = 1.0 - smoothstep(0.03, 0.08, abs(a - 0.25));
    float wGrass = smoothstep(0.62, 0.95, a);
    float wStone = (1.0 - smoothstep(0.10, 0.35, a)) * (1.0 - wWater);
    float wDirt = clamp(1.0 - wGrass - wStone - wWater, 0.0, 1.0);
    // Cobbles at a finer scale than the dirt, and softened: stones ~8 units across.
    float cob = mix(0.5, texture2D(texture2, wuv * 2.6 + vec2(0.5, 0.25)).r, 0.7);
    vec3 det = grassD * wGrass + vec3(cob) * wStone + vec3(soilD.g) * wDirt + vec3(0.5) * wWater;

    float fogDist = length(viewPos - fragPosition);
    det = mix(det, vec3(0.5), smoothstep(900.0, 1900.0, fogDist));

    vec3 base = clamp(mac.rgb * det * 2.0, 0.0, 1.0) * fragColor.rgb * colDiffuse.rgb;
    vec3 albedo = pow(base, vec3(2.2));

    vec3 normal = normalize(fragNormal);
    vec3 l = -lightDir;
    float NdotL = max(dot(normal, l), 0.0);
    float up = normal.y * 0.5 + 0.5;
    vec3 amb = ambient.rgb * 0.75 * mix(vec3(0.55, 0.50, 0.45), vec3(1.05, 1.08, 1.18), up);
    // Drifting cloud shadows: two octaves of soft noise sliding with the wind.
    vec2 cp = fragPosition.xz * 0.0011 + vec2(time * 0.010, time * 0.004);
    float cloud = vnoise(cp) * 0.65 + vnoise(cp * 2.3 + 7.1) * 0.35;
    float shade = 1.0 - 0.42 * smoothstep(0.52, 0.78, cloud);
    vec3 col = albedo * (lightColor.rgb * 0.80 * NdotL * shade + amb);

    if (wWater > 0.01) {
        // Water: two ripple layers scrolling against each other, sky reflection
        // toward grazing angles, and sun glints where the ripples line up.
        vec2 r1 = wuv * 1.3 + vec2(time * 0.020, time * 0.011);
        vec2 r2 = wuv * 0.9 - vec2(time * 0.014, time * 0.019);
        float rip = texture2D(texture2, r1).g + texture2D(texture2, r2).g;       // ~0.5..1.5
        vec3 viewD = normalize(viewPos - fragPosition);
        float fres = pow(1.0 - clamp(viewD.y, 0.0, 1.0), 3.0);
        vec3 wcol = col * (0.85 + (rip - 1.0) * 0.35);
        wcol = mix(wcol, pow(skyColor, vec3(2.2)) * shade, 0.18 + 0.45 * fres);
        wcol += lightColor.rgb * smoothstep(1.28, 1.45, rip) * 0.55 * shade;    // glints
        col = mix(col, wcol, wWater);
    }

    col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
    float fogF = smoothstep(fogRange.x, fogRange.y, fogDist);
    col = mix(col, fogColor, clamp(fogF, 0.0, 1.0));
    gl_FragColor = vec4(col, 1.0);
}
