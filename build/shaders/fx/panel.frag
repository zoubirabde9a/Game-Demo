// Rounded panel for HUD plates and screens: dark glass with a soft drop
// shadow, a thin tinted border and a lit top edge. UV runs 0..1 over the
// panel and past that over the margin DrawUIPanel (engine/ui/panel.cpp)
// leaves round it for the shadow; the panel's size in pixels comes from
// how fast UV changes per pixel, so corners stay round at any size and
// any display scale.
//   colour rgb: the border's tint
//   colour a:   the panel's opacity, for fading in and out

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
    vec2 P = fragmentUV * Size;
    vec2 Half = 0.5 * Size;
    vec2 Centre = P - Half;
    float Radius = min(14.0, 0.5 * min(Half.x, Half.y));
    float D = RoundBox(Centre, Half, Radius);
    float Inside = 1.0 - smoothstep(-0.75, 0.75, D);

    // NOTE(zoubir): the shadow falls a little below the panel and fades
    // out over the margin
    float Below = RoundBox(Centre - vec2(0.0, 4.0), Half, Radius);
    float Shadow = 0.42 * (1.0 - smoothstep(-6.0, 16.0, Below));

    float Down = clamp(fragmentUV.y, 0.0, 1.0);
    vec3 Body = mix(vec3(0.115, 0.125, 0.170), vec3(0.040, 0.045, 0.065), Down);
    float Border = 1.0 - smoothstep(0.4, 1.6, abs(D + 1.2));
    float Top = Border * (1.0 - smoothstep(0.0, 0.35, Down));
    vec3 Colour = mix(Body, fragmentColor.rgb * 0.85, Border * 0.85) + vec3(0.25) * Top;

    float Glass = 0.88 * Inside;
    float Alpha = Glass + Shadow * (1.0 - Glass);
    Colour = Alpha > 0.0 ? Colour * (Glass / Alpha) : vec3(0.0);
    FragColor = vec4(Colour, Alpha * fragmentColor.a);
}
