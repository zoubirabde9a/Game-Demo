precision mediump float;

attribute vec3 vertexPosition;
attribute vec4 vertexColor;
attribute vec2 vertexUV;

varying vec3 fragmentPosition;
varying vec4 fragmentColor;
varying vec2 fragmentUV;

uniform mat4 P;

void main() {
    gl_Position = (P * vec4(vertexPosition, 1.0));
    gl_Position.w = 1.0;
    fragmentPosition = vertexPosition; 
    fragmentColor = vertexColor; 
    fragmentUV = vec2(vertexUV.x, 1.0 - vertexUV.y);
}
