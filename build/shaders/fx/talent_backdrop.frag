// Talent branch backdrop (ui/talent_panel/): a column of the talent panel
// glowing faintly in its branch's colour, brightest near the top where
// the branch's name sits, with slow drifting light so it never looks flat.
//   colour rgb: the branch's accent
//   colour a:   strength, 0..1

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

float RoundBox(vec2 P, vec2 Half, float Radius)
{
    vec2 Q = abs(P) - Half + Radius;
    return length(max(Q, 0.0)) + min(max(Q.x, Q.y), 0.0) - Radius;
}

void main()
{
    vec2 UV = fragmentUV;
    vec3 Accent = fragmentColor.rgb;
    float Strength = fragmentColor.a;
    vec2 P = UV * 2.0 - 1.0;
    float D = RoundBox(P, vec2(1.0) - 0.01, 0.08);
    float Inside = 1.0 - smoothstep(-0.01, 0.01, D);

    float Top = exp(-UV.y * 3.0);
    float DriftA = 0.5 + 0.5 * sin(UV.y * 9.0 - Time * 0.7 + UV.x * 3.0);
    float DriftB = 0.5 + 0.5 * sin(UV.y * 5.0 + Time * 0.4 - UV.x * 7.0);
    float Light = 0.18 + 0.55 * Top + 0.12 * DriftA * DriftB;
    vec3 Color = mix(vec3(0.03, 0.035, 0.05), Accent, Light * 0.5);

    float Border = 1.0 - smoothstep(0.0, 0.012, abs(D + 0.006));
    Color = mix(Color, Accent * 0.9, Border * 0.7);

    FragColor = vec4(Color, Inside * Strength * 0.9);
}
