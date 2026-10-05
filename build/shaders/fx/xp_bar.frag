// Experience bar (ui/talent_panel/xp_bar.cpp): a rounded dark track with
// a liquid fill, lighter on top, with a shimmer running along it and a
// bright lip at its front edge.
//   colour rgb: the fill's colour
//   colour a:   how full it is, 0..1

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

void main()
{
    vec2 UV = fragmentUV;
    float Fill = fragmentColor.a;
    vec3 Tint = fragmentColor.rgb;

    // Track: rounded ends from the quad's own height (the bar is long and
    // thin, so the ends are judged on Y only)
    float Edge = abs(UV.y * 2.0 - 1.0);
    float Track = 1.0 - smoothstep(0.75, 1.0, Edge);
    vec3 Color = mix(vec3(0.07, 0.07, 0.10), vec3(0.02, 0.02, 0.04), UV.y);

    float In = 1.0 - smoothstep(Fill - 0.004, Fill, UV.x);
    vec3 Liquid = mix(Tint * 1.25, Tint * 0.55, UV.y);
    Liquid += vec3(0.25) * (1.0 - smoothstep(0.0, 0.35, UV.y));
    float Wave = (fract(UV.x * 3.0 - Time * 0.6) - 0.5) * 6.0;
    Liquid += Tint * 0.35 * exp(-Wave * Wave);
    Color = mix(Color, Liquid, In);

    // The lip: a glow at the fill's front
    float Front = (UV.x - Fill) * 120.0;
    float Lip = exp(-Front * Front) * step(0.001, Fill) * step(Fill, 0.999);
    Color += vec3(1.0, 0.95, 0.8) * Lip;

    FragColor = vec4(Color, Track * 0.95);
}
