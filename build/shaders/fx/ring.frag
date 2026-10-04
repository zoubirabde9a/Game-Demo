// Thin bright ring touching the quad's edge, with a soft halo inside.
// Grow the quad over time for a shockwave. Usually drawn additive.
//   colour rgb: the ring's colour
//   colour a:   its strength

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    float Ring = 1.0 - smoothstep(0.0, 0.08, abs(R - 0.9));
    float Halo = smoothstep(0.3, 0.9, R) * (1.0 - smoothstep(0.9, 1.0, R)) * 0.35;
    FragColor = vec4(fragmentColor.rgb, fragmentColor.a * max(Ring, Halo));
}
