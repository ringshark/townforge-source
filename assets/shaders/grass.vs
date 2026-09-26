#version 100

// Town Forge 3D grass sway shader (vertex stage).
// Same varyings/uniforms contract as shadowmap.vs so it pairs with the
// existing shadowmap.fs: grass gets the same sun, shadow-receive, and
// distance fog as every other model. The only addition is a cheap GPU wind
// wobble: sway weight is 0 at the tuft base (y=0) and 1 at the tip, with a
// world-space phase so neighboring tufts don't move in lockstep.

// Input vertex attributes
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float time; // seconds, set per frame like the torch shader's

// Output vertex attributes (to fragment shader)
varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

void main()
{
    vec3 p = vertexPosition;

    // Blades are ~7-15 units tall; weight the bend quadratically toward the tip.
    float w = clamp(p.y / 15.0, 0.0, 1.0);
    w = w * w;

    // World-space phase (grass is drawn at identity, so model == world).
    vec4 wp4 = matModel * vec4(p, 1.0);
    float ph = wp4.x * 0.043 + wp4.z * 0.061;

    float s1 = sin(time * 1.6 + ph);
    float s2 = sin(time * 2.7 + ph * 1.7 + 1.3);
    p.x += w * (s1 * 2.2 + s2 * 0.8);
    p.z += w * (cos(time * 1.2 + ph * 1.3) * 1.4);

    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel * vec4(p, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));

    // Calculate final vertex position (from the swayed position)
    gl_Position = mvp * vec4(p, 1.0);
}
