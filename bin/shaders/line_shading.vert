#version 130


in vec3 vertexPosition;
in vec4 vertexColor;

out vec3 fragmentPosition;
out vec4 fragmentColor;

uniform mat4 P;

void main() {
    gl_Position = (P * vec4(vertexPosition, 1.0));
    gl_Position.w = 1.0;
    fragmentPosition = vertexPosition; 
    fragmentColor = vertexColor; 
}
