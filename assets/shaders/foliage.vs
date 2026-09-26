#version 100

// Town Forge 3D foliage sway (vertex stage) — 2026-09-26 environment pass.
// lit.vs plus a gentle wind bend for trees and bushes: the offset grows with
// the vertex's height above the ground (world space), so trunks stay planted
// and crowns sway. Applied in model space (divided by the instance scale), so
// each rotated instance sways in its own direction - no two trees in step.

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float time;

varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

void main()
{
    vec3 p = vertexPosition;
    vec4 w = matModel * vec4(p, 1.0);
    float scale = max(length(vec3(matModel[0])), 0.0001);
    float h = max(w.y - 10.0, 0.0);
    float ph = matModel[3].x * 0.031 + matModel[3].z * 0.047; // per-instance phase from its position
    float gust = 0.6 + 0.4 * sin(time * 0.35 + ph * 0.2);
    float amt = h * h * 0.00032 * gust * (sin(time * 1.3 + ph) + 0.35 * sin(time * 2.9 + ph * 1.7 + w.y * 0.05));
    p.x += amt / scale;
    p.z += amt * 0.6 / scale;

    fragPosition = vec3(matModel * vec4(p, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
    gl_Position = mvp * vec4(p, 1.0);
}
