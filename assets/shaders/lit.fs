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
    // Texel color fetching from texture sampler
    vec4 texelColor = texture2D(texture0, fragTexCoord)*fragColor;
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 specular = vec3(0.0);

    vec3 l = -lightDir;

    float NdotL = max(dot(normal, l), 0.0);
    lightDot += lightColor.rgb*NdotL;

    float specCo = 0.0;
    if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(l), normal))), 16.0); // 16 refers to shine
    specular += specCo;

    vec4 finalColor = (texelColor*((colDiffuse + vec4(specular, 1.0))*vec4(lightDot, 1.0)));

    // Add ambient lighting
    finalColor += texelColor*(ambient/10.0)*colDiffuse;

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0/2.2));

    // Distance fog toward the horizon color (applied in display space so the
    // far ground melts into the sky gradient)
    float fogDist = length(viewPos - fragPosition);
    float fogF = smoothstep(fogRange.x, fogRange.y, fogDist);
    finalColor.rgb = mix(finalColor.rgb, fogColor, clamp(fogF, 0.0, 1.0));

    gl_FragColor = finalColor;
}
