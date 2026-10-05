#version 100

// Keep world-space lighting stable on mobile; retain an ES2 fallback.
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

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
// x: metal coverage, y: detail smoothing, z: material brightness.
uniform vec3 skinFinish;

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
    vec4 original = texture2D(texture0, fragTexCoord);
    // Contrast compression stays within the sampled UV island; neighboring
    // atlas texels can belong to another garment or face.
    float face = step(0.45, original.r) * step(original.g * 1.08, original.r)
               * step(original.b * 1.06, original.g) * step(original.r, original.g * 1.75);
    vec4 surface = original;
    surface.rgb = mix(original.rgb, original.rgb * 0.88 + vec3(0.025),
                      skinFinish.y * (1.0 - face));
    float hi = max(surface.r, max(surface.g, surface.b));
    float lo = min(surface.r, min(surface.g, surface.b));
    float saturation = (hi - lo) / max(hi, 0.001);
    float metal = skinFinish.x * (1.0 - smoothstep(0.12, 0.34, saturation))
                * smoothstep(0.02, 0.18, hi);
    surface.rgb = mix(surface.rgb, vec3(0.62, 0.65, 0.69), metal * 0.35);
    surface.rgb *= skinFinish.z;
    vec4 texelColor = surface * fragColor * colDiffuse;
    vec3 albedo = pow(texelColor.rgb, vec3(2.2));
    float normalLength = length(fragNormal);
    vec3 normal = normalLength > 0.0001 ? fragNormal / normalLength : vec3(0.0, 1.0, 0.0);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 l = -lightDir;
    float NdotL = max(dot(normal, l), 0.0);

    // Hemisphere ambient: cool sky fill from above, darker warm ground bounce
    // from below, so faces turned away from the sun still show their form.
    float up = normal.y*0.5 + 0.5;
    // Lift side-facing detail gently: leather folds and sculpted faces should
    // remain readable in moonlight without flattening the sunlit surfaces.
    vec3 amb = ambient.rgb*0.75*mix(vec3(0.55, 0.50, 0.45), vec3(1.05, 1.08, 1.18), up);
    amb *= 1.0 + 0.20 * (1.0 - NdotL) * (1.0 - up);
    vec3 light = lightColor.rgb*0.80*NdotL + amb;

    float spec = 0.0;
    if (NdotL > 0.0) spec = pow(max(0.0, dot(viewD, reflect(-l, normal))), mix(14.0, 32.0, metal))*mix(0.035, 0.20, metal)*NdotL;

    vec3 col = albedo*light + lightColor.rgb*spec;
    // A restrained sky reflection outlines grazing surfaces. Tied to the
    // existing sky fill, so it follows night/day instead of glowing in darkness.
    float edge = pow(1.0 - max(dot(normal, viewD), 0.0), 3.0);
    col += ambient.rgb * vec3(0.82, 0.94, 1.12) * edge * mix(0.015, 0.10, metal);

    // Gamma encode (linear -> display)
    col = pow(max(col, vec3(0.0)), vec3(1.0/2.2));

    // Preserve model transparency and draw tint.
    float alpha = texelColor.a;

    // Distance fog toward the horizon color (applied in display space so the
    // far ground melts into the sky gradient)
    float fogDist = length(viewPos - fragPosition);
    float fogF = smoothstep(fogRange.x, fogRange.y, fogDist);
    col = mix(col, fogColor, clamp(fogF, 0.0, 1.0));

    gl_FragColor = vec4(col, alpha);
}
