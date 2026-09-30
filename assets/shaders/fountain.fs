#version 100
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

// Opaque shallow basin water: animated reflection and spill-stream highlights.
// No refraction buffer, extra textures, or transparent surface sorting.
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 lightDir;
uniform vec4 lightColor;
uniform vec4 ambient;
uniform vec3 viewPos;
uniform vec3 fogColor;
uniform vec2 fogRange;
uniform vec3 skyColor;
uniform float time;

void main() {
    vec3 n = normalize(fragNormal);
    float horizontal = smoothstep(0.4, 0.9, abs(n.y));
    vec2 p = fragPosition.xz * 0.23;
    float waveX = sin(p.x + p.y * 0.7 - time * 1.7);
    float waveZ = cos(p.y - p.x * 0.6 + time * 1.3);
    vec3 rippleN = normalize(n + vec3(waveX, 0.0, waveZ) * 0.10 * horizontal);
    vec3 v = normalize(viewPos - fragPosition), l = -lightDir;
    float fresnel = pow(1.0 - max(dot(rippleN, v), 0.0), 4.0);
    vec3 base = pow((texture2D(texture0, fragTexCoord) * fragColor * colDiffuse).rgb, vec3(2.2));
    vec3 col = base * (ambient.rgb * 0.75 + lightColor.rgb * 0.8 * max(dot(rippleN, l), 0.0));
    // Sky reflection is scaled by ambient fill so water dims after sunset.
    col = mix(col, pow(skyColor, vec3(2.2)) * min(ambient.rgb * 1.6, vec3(1.0)),
              horizontal * (0.14 + fresnel * 0.42));
    float glint = pow(max(dot(reflect(-l, rippleN), v), 0.0), 64.0);
    col += lightColor.rgb * glint * 0.32;
    // Bright bands travel down the spill tubes. Day/night light controls them.
    float flow = 0.85 + 0.15 * sin(fragPosition.y * 1.8 + time * 10.0);
    col *= mix(flow, 0.96 + waveX * waveZ * 0.04, horizontal);
    col = pow(max(col, vec3(0.0)), vec3(1.0 / 2.2));
    float fog = smoothstep(fogRange.x, fogRange.y, length(viewPos - fragPosition));
    gl_FragColor = vec4(mix(col, fogColor, fog), 1.0);
}
