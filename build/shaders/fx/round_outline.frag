// Rounded outline: the border of a round_rect.frag shape alone, two pixels
// wide, for focus and selection (DrawRoundOutline, engine/ui/panel.cpp).
//   colour: the line, alpha included

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
    float Line = 1.0 - smoothstep(0.6, 1.6, abs(D + 1.25));
    FragColor = vec4(fragmentColor.rgb, fragmentColor.a * Line);
}
