// Ability cooldown: darkens the part of the slot still recharging, as a
// clock hand that sweeps clockwise from twelve o'clock, with a bright
// edge on the hand. Drawn over the icon on the same quad as slot_frame.
//   colour rgb: the colour of the hand's edge
//   colour a:   share of the cooldown left, 1 just used .. 0 ready

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

#define SLOT_BOX 0.78
#define SLOT_CORNER 0.22
#define PI 3.14159265

float RoundBox(vec2 P, vec2 Half, float Radius)
{
    vec2 Q = abs(P) - Half + Radius;
    return length(max(Q, 0.0)) + min(max(Q.x, Q.y), 0.0) - Radius;
}

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float Left = fragmentColor.a;
    float D = RoundBox(P, vec2(SLOT_BOX - 0.04), SLOT_CORNER);
    float Inside = 1.0 - smoothstep(-0.02, 0.02, D);

    // 0 at twelve o'clock, growing clockwise to 1 (UV y points down)
    float Turn = atan(P.x, -P.y) / (2.0 * PI);
    Turn = fract(Turn + 1.0);
    float Done = 1.0 - Left;
    float Dark = smoothstep(Done - 0.004, Done + 0.004, Turn);

    // The hand: a thin line from the centre along angle Done
    float HandAngle = Done * 2.0 * PI;
    vec2 HandDir = vec2(sin(HandAngle), -cos(HandAngle));
    float Along = dot(P, HandDir);
    float Across = abs(P.x * HandDir.y - P.y * HandDir.x);
    float Hand = (1.0 - smoothstep(0.0, 0.05, Across)) * step(0.0, Along);
    Hand *= step(0.001, Left) * step(Left, 0.999);

    float Alpha = Inside * max(Dark * 0.72, Hand * 0.9);
    vec3 Color = mix(vec3(0.0), fragmentColor.rgb, Hand);
    FragColor = vec4(Color, Alpha);
}
