// Talent node: a round medallion behind a talent's icon in the talent
// panel (ui/talent_panel/). The quad is the node plus a margin for the
// glow; the disc fills NODE_DISC of it.
//   colour rgb: the branch's accent
//   colour a:   the node's state: 0 locked, 0.5 a point can go in (the
//               rim pulses), 1 every rank bought (a gold rim and a slow
//               halo); values between blend

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

#define NODE_DISC 0.74
#define EDGE 0.02

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    vec3 Accent = fragmentColor.rgb;
    float State = fragmentColor.a;
    float Open = smoothstep(0.1, 0.5, State);
    float Full = smoothstep(0.6, 1.0, State);

    float D = R - NODE_DISC;
    float Inside = 1.0 - smoothstep(-EDGE, EDGE, D);

    // Body: dark glass lit from above, warmer with the branch's colour
    // once it is open
    vec3 Body = mix(vec3(0.15, 0.16, 0.21), vec3(0.03, 0.035, 0.06), fragmentUV.y);
    Body += Accent * (0.04 + 0.10 * Open);
    float Bevel = (1.0 - smoothstep(0.0, 0.06, abs(D + 0.09))) * smoothstep(0.3, -0.7, P.y);
    Body += vec3(0.12) * Bevel;

    // Rim: grey and thin while locked, the accent while open, gold when
    // full; it breathes while a point can go in
    float Pulse = 0.65 + 0.35 * sin(Time * 4.0);
    vec3 Gold = vec3(1.0, 0.82, 0.36);
    vec3 RimColor = mix(vec3(0.26, 0.27, 0.32), Accent * (0.8 + 0.4 * Pulse), Open);
    RimColor = mix(RimColor, Gold, Full);
    float RimWidth = mix(0.035, 0.06, Open);
    float Rim = 1.0 - smoothstep(0.0, RimWidth, abs(D + 0.03));
    vec3 Color = mix(Body, RimColor, Rim);

    // Sheen sweeping across a full node now and then
    float Sweep = fract(Time * 0.25 + Accent.g * 0.5);
    float Band = (P.x - P.y) * 0.5 - (Sweep * 5.0 - 2.5);
    Color += vec3(0.25) * Full * exp(-Band * Band * 24.0) * Inside;

    // Halo outside the disc: a breath for an open node, steady gold for a
    // full one, nothing for a locked one
    float Outside = max(D, 0.0);
    float Halo = exp(-Outside * 9.0) * (1.0 - Inside);
    vec3 HaloColor = mix(Accent, Gold, Full);
    float HaloStrength = (Open - Full) * 0.75 * Pulse + Full * 0.45;

    float Alpha = max(Inside * 0.96, Halo * HaloStrength);
    vec3 Final = mix(HaloColor, Color, Inside);
    FragColor = vec4(Final, Alpha);
}
