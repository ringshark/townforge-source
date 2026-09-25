#version 100

precision mediump float;

// Town Forge 3D "lit" shader (fragment stage) — 2026-09-24.
// This is assets/shaders/shadowmap.fs with the shadow-map sampling section
// removed. Shadows themselves stayed disabled (kT3DShadowsEnabled=false)
// after WebGL/mobile GPUs turned out to honor this shader's
// `precision mediump float` far more literally than desktop drivers,
// causing depth-precision banding ("shadow acne") in the shadow-map
// comparison specifically — but that bug was isolated to the shadow-map
// sampling code below, never the plain diffuse/specular/fog lighting math,
// which is exactly as safe on mobile as anywhere else. This shader keeps
// only that safe part, so every model gets real per-vertex directional
// shading (and the ground/sky fog) instead of the flat, unlit fallback
// that shipped while shadows were off.

// Input vertex attributes (from vertex shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Input lighting values
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec4 ambient;
uniform vec3 viewPos;

// Distance fog (display space, matched to the sky horizon color)
uniform vec3 fogColor;
uniform vec2 fogRange; // x = fog starts, y = fully fogged

void main()
{
    // Linear-space lighting (2026-09-25). Textures, vertex colors and the
    // material tint are all authored in sRGB, so they are decoded to linear
    // (pow 2.2) before lighting and encoded exactly once at the end. The
    // previous version lit the raw sRGB values and then gamma-encoded them a
    // second time, which lifted every midtone and made the whole scene look
    // pale and washed out.
    vec4 texelColor = texture2D(texture0, fragTexCoord)*fragColor*colDiffuse;
    vec3 albedo = pow(texelColor.rgb, vec3(2.2));
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 l = -lightDir;
    float NdotL = max(dot(normal, l), 0.0);

    // Hemisphere ambient: cool sky fill from above, darker warm ground bounce
    // from below, so faces turned away from the sun still show their form.
    float up = normal.y*0.5 + 0.5;
    vec3 amb = ambient.rgb*0.75*mix(vec3(0.55, 0.50, 0.45), vec3(1.05, 1.08, 1.18), up);
    vec3 light = lightColor.rgb*0.80*NdotL + amb;

    float spec = 0.0;
    if (NdotL > 0.0) spec = pow(max(0.0, dot(viewD, reflect(-l, normal))), 24.0)*0.10;

    vec3 col = albedo*light + lightColor.rgb*spec;

    // Gamma encode (linear -> display)
    col = pow(max(col, vec3(0.0)), vec3(1.0/2.2));

    // Distance fog toward the horizon color (applied in display space so the
    // far ground melts into the sky gradient)
    float fogDist = length(viewPos - fragPosition);
    float fogF = smoothstep(fogRange.x, fogRange.y, fogDist);
    col = mix(col, fogColor, clamp(fogF, 0.0, 1.0));

    gl_FragColor = vec4(col, texelColor.a);
}
