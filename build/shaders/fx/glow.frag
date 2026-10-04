// Soft round light: brightest at the centre of the quad, gone at its
// edge. Usually drawn additive.
//   colour rgb: the light's colour
//   colour a:   its strength

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    float Light = 1.0 - smoothstep(0.0, 1.0, R);
    Light = Light * Light;
    FragColor = vec4(fragmentColor.rgb, fragmentColor.a * Light);
}
