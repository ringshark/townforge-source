#version 100

// Town Forge 3D dungeon torch-light shader (vertex stage).
// Same attribute/uniform conventions as assets/shaders/shadowmap.vs
// (raylib 5.x GLSL100), so raylib primitives (DrawCube, DrawSphere, ...)
// and textured models both work under it.

// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// (2026-09-29) UO-style wall cut-away: walls between the camera and the player
// sink to a knee-high stump so you're never hidden behind a block of rock.
// cutInfo.xy = player (world x,z), cutInfo.zw = unit direction player -> camera.
// Only the wall mesh turns it on (its model matrix is identity, so local = world).
uniform vec4 cutInfo;
uniform float cutOn;

// Output vertex attributes (to fragment shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

void main()
{
    // Send vertex attributes to fragment shader
    vec3 p = vertexPosition;
    fragPosition = vec3(matModel*vec4(p, 1.0));
    if (cutOn > 0.5 && fragPosition.y > 1.0) {
        vec2 d = fragPosition.xz - cutInfo.xy;
        float along = dot(d, cutInfo.zw);                       // toward the camera
        float side = abs(d.x*cutInfo.w - d.y*cutInfo.z);         // off the sight line
        float halfW = 120.0 + max(along, 0.0)*0.5;               // a wedge that widens toward the camera
        float k = smoothstep(-50.0, 10.0, along) * (1.0 - smoothstep(halfW - 45.0, halfW, side));
        float cutY = mix(fragPosition.y, 14.0, k);
        p.y += cutY - fragPosition.y;
        fragPosition.y = cutY;
    }
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 0.0)));

    // Calculate final vertex position
    gl_Position = mvp*vec4(p, 1.0);
}
