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

// Output vertex attributes (to fragment shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

void main()
{
    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));

    // Calculate final vertex position
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
