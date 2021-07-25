#version 130

in vec3 fragmentPosition;
in vec4 fragmentColor;

out vec4 color;

void main() {    
    color = fragmentColor;
}
