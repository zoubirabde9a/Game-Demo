#version 330
//The vertex shader operates on each vertex

//input data from the VBO. Each vertex is 2 floats
in vec3 vertexPosition;
in vec4 vertexColor;
in vec2 vertexUV;

out vec3 fragmentPosition;
out vec4 fragmentColor;
out vec2 fragmentUV;

uniform mat4 P;

void main() {
    gl_Position = (P * vec4(vertexPosition, 1.0));
    gl_Position.w = 1.0;
    fragmentPosition = vertexPosition; 
    fragmentColor = vertexColor; 
    fragmentUV = vec2(vertexUV.x, 1.0 - vertexUV.y);
}
