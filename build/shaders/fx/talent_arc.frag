// Progress arc (ui/talent_panel/): a ring round a medallion or badge that
// fills clockwise from the top, with a dim track under it and a bright
// head where the fill ends. The ring sits at ARC_RADIUS of the quad.
//   colour rgb: the fill's colour
//   colour a:   how full, 0..1

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

#define ARC_RADIUS 0.86
#define ARC_WIDTH 0.075
#define TAU 6.2831853

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    float Fill = fragmentColor.a;
    // Angle from the top, clockwise, 0..1 (Y runs down the quad)
    float Turn = atan(P.x, -P.y) / TAU;
    Turn = Turn < 0.0 ? Turn + 1.0 : Turn;

    float Band = 1.0 - smoothstep(ARC_WIDTH * 0.5, ARC_WIDTH * 0.5 + 0.03, abs(R - ARC_RADIUS));
    float Lit = 1.0 - smoothstep(Fill - 0.004, Fill, Turn);
    Lit *= step(0.001, Fill);

    vec3 Track = vec3(0.10, 0.11, 0.15);
    // A glint running round the filled part
    float Glint = fract(Turn - Time * 0.35);
    float Shine = exp(-Glint * Glint * 60.0) * Lit;
    vec3 Color = mix(Track, fragmentColor.rgb * (0.85 + 0.15 * sin(Time * 3.0)), Lit);
    Color += vec3(0.5) * Shine;

    // The head: a dot of light where the fill ends
    vec2 Head = ARC_RADIUS * vec2(sin(Fill * TAU), -cos(Fill * TAU));
    float HeadLight = exp(-dot(P - Head, P - Head) * 220.0) * step(0.001, Fill) * step(Fill, 0.999);
    Color += fragmentColor.rgb * HeadLight * 1.5;

    float Alpha = max(Band * mix(0.55, 1.0, Lit), HeadLight);
    FragColor = vec4(Color, Alpha);
}
