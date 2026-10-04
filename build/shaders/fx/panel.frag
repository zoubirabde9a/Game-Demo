// Rounded panel for HUD plates: translucent dark glass, a thin border
// and a lit top edge. The corners stay round at any size because the
// colour's alpha carries the panel's height over its width
// (DrawUIPanel, engine/ui/panel.cpp).
//   colour rgb: the border's tint
//   colour a:   the quad's height divided by its width, over 4

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

float RoundBox(vec2 P, vec2 Half, float Radius)
{
    vec2 Q = abs(P) - Half + Radius;
    return length(max(Q, 0.0)) + min(max(Q.x, Q.y), 0.0) - Radius;
}

void main()
{
    float Aspect = max(fragmentColor.a * 4.0, 0.02);
    vec2 P = (fragmentUV * 2.0 - 1.0) * vec2(1.0, Aspect);
    float Corner = min(0.35 * Aspect, 0.06);
    float D = RoundBox(P, vec2(1.0, Aspect) - 0.01, Corner);
    float Edge = 0.012;
    float Inside = 1.0 - smoothstep(-Edge, Edge, D);
    vec3 Body = mix(vec3(0.09, 0.10, 0.14), vec3(0.03, 0.035, 0.05), fragmentUV.y);
    float Border = 1.0 - smoothstep(0.0, 0.02, abs(D + 0.012));
    float Top = (1.0 - smoothstep(0.0, 0.25, fragmentUV.y)) * Border;
    vec3 Color = mix(Body, fragmentColor.rgb * 0.7, Border * 0.8) + vec3(0.18) * Top;
    FragColor = vec4(Color, Inside * 0.86);
}
