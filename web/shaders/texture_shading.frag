precision mediump float;

varying vec3 fragmentPosition;
varying vec4 fragmentColor;
varying vec2 fragmentUV;

uniform sampler2D textureSampler;

void main() {    
    gl_FragColor = vec4(1, 1, 1, 1) * texture2D(textureSampler, fragmentUV);
}
