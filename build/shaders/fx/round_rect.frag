// Rounded rectangle for small UI parts: bars, buttons, fields, key tabs
// and chips (DrawRoundRect, engine/ui/panel.cpp). Filled with the colour,
// lighter at the top and darker at the foot, with a lit inner edge along
// the top and a shaded one along the bottom, so it reads as a raised part
// rather than a flat box. Measures itself from how fast its UV changes per
// pixel; corners are 6 pixels, or a full pill on anything 12 or thinner.
//   colour: the fill, alpha included

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

float RoundBox(vec2 P, vec2 Half, float Radius)
{
    vec2 Q = abs(P) - Half + Radius;
    return length(max(Q, 0.0)) + min(max(Q.x, Q.y), 0.0) - Radius;
}

void main()
{
    vec2 Size = 1.0 / max(fwidth(fragmentUV), vec2(1e-5));
    vec2 Half = 0.5 * Size;
    vec2 Centre = fragmentUV * Size - Half;
    float Radius = min(6.0, min(Half.x, Half.y));
    float D = RoundBox(Centre, Half - 0.5, Radius);
    float Inside = 1.0 - smoothstep(-0.7, 0.7, D);

    float Down = fragmentUV.y;
    vec3 Colour = fragmentColor.rgb * (1.10 - 0.22 * Down);
    float Edge = 1.0 - smoothstep(0.0, 1.2, abs(D + 1.0));
    Colour = mix(Colour, Colour * 1.3 + 0.05, Edge * (1.0 - Down));
    Colour = mix(Colour, Colour * 0.6, Edge * Down);
    FragColor = vec4(Colour, fragmentColor.a * Inside);
}
