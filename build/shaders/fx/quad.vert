// Shared vertex shader for every effect in fx/: passes the colour and
// the UV through. The loader adds the prelude that defines ATTRIBUTE and
// VARYING (engine/shader_library.cpp).

ATTRIBUTE vec3 vertexPosition;
ATTRIBUTE vec4 vertexColor;
ATTRIBUTE vec2 vertexUV;

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform mat4 P;

void main()
{
    gl_Position = P * vec4(vertexPosition, 1.0);
    gl_Position.w = 1.0;
    fragmentColor = vertexColor;
    fragmentUV = vertexUV;
}
