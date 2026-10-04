// Ability slot frame. The quad is the slot plus a margin for the glow;
// the rounded box fills SLOT_BOX of it.
//   colour rgb: the ability's accent
//   colour a:   how ready it is, 0 recharging .. 1 ready (glow and sheen)

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

#define SLOT_BOX 0.78
#define SLOT_CORNER 0.22
#define EDGE 0.025

float RoundBox(vec2 P, vec2 Half, float Radius)
{
    vec2 Q = abs(P) - Half + Radius;
    return length(max(Q, 0.0)) + min(max(Q.x, Q.y), 0.0) - Radius;
}

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    vec3 Accent = fragmentColor.rgb;
    float Ready = fragmentColor.a;
    float D = RoundBox(P, vec2(SLOT_BOX), SLOT_CORNER);
    float Inside = 1.0 - smoothstep(-EDGE, EDGE, D);

    // Body: dark glass, lighter at the top, with a faint accent tint
    vec3 Body = mix(vec3(0.13, 0.14, 0.19), vec3(0.04, 0.045, 0.07), fragmentUV.y);
    Body += Accent * 0.06;
    // Inner bevel: a light line just inside the top edge
    float Bevel = (1.0 - smoothstep(0.0, 0.05, abs(D + 0.07))) *
                  smoothstep(0.2, -0.6, P.y);
    Body += vec3(0.10) * Bevel;

    // Border: grey while recharging, the accent when ready
    float Border = 1.0 - smoothstep(0.0, 0.045, abs(D + 0.018));
    vec3 BorderColor = mix(vec3(0.30, 0.32, 0.38), Accent, Ready);
    vec3 Color = mix(Body, BorderColor, Border);

    // Sheen: a light band sweeps across every three seconds once ready
    float Sweep = fract(Time * 0.33 + Accent.r * 0.37);
    float Band = (P.x + P.y) * 0.5 - (Sweep * 5.0 - 2.5);
    Color += vec3(0.28) * Ready * exp(-Band * Band * 30.0) * Inside;

    // Glow outside the box, breathing slowly while ready
    float Outside = max(D, 0.0);
    float Breath = 0.75 + 0.25 * sin(Time * 3.2);
    float Glow = Ready * Breath * exp(-Outside * 11.0) * (1.0 - Inside) * 0.8;

    float Alpha = max(Inside * 0.94, Glow);
    vec3 Final = mix(Accent, Color, Inside);
    FragColor = vec4(Final, Alpha);
}
