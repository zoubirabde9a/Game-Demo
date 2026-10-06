// The screen's edge: a soft dark vignette that frames the world, and a
// red tint over it when the player is hurt. Drawn once over the whole
// screen, alpha blended, below the HUD.
//   colour rgb: the tint's colour
//   colour a:   the tint's strength, 0 for the plain vignette

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    // NOTE(zoubir): squarer than a circle, so the corners darken most and
    // the middle of each side only a little
    float D = length(P * P * P * P + P * P) * 0.75;
    float Dark = 0.45 * smoothstep(0.35, 1.1, D);
    float Tint = fragmentColor.a * smoothstep(0.15, 1.0, D);
    float Alpha = Dark + Tint - Dark * Tint;
    vec3 Colour = Tint > 0.0 ? fragmentColor.rgb * (Tint / Alpha) : vec3(0.0);
    FragColor = vec4(Colour, Alpha);
}
