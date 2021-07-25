precision mediump float;

attribute vec3 vertexPosition;
attribute vec4 vertexColor;

varying vec3 fragmentPosition;
varying vec4 fragmentColor;

uniform mat4 P;

void main() {
    gl_Position = (P * vec4(vertexPosition, 1.0));
    gl_Position.w = 1.0;
    fragmentPosition = vertexPosition; 
    fragmentColor = vertexColor; 
}
