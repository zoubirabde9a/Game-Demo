// Ground crack: the broken ground a Launch or a slam leaves behind, drawn
// flat on the ground under units (client/ground_cracks.cpp). The quad is
// the crack's whole circle. Jagged splits run out from the middle and
// fork, over a darker scorched patch with a few rubble chips; each split
// has a dark gap and a pale lip along one side, so it reads as cut into
// the ground. Fresh cracks glow hot in their gaps for a moment.
//   colour r: the crack's seed, 0..1, so each one has its own shape
//   colour g: heat, 1 just after the hit, falling to 0
//   colour a: strength, falling to 0 as it fades out

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

float Hash(float N)
{
    return fract(sin(N) * 43758.5453);
}

float Noise(float X)
{
    float I = floor(X);
    float F = fract(X);
    F = F * F * (3.0 - 2.0 * F);
    return mix(Hash(I), Hash(I + 1.0), F);
}

// distance from P to a split that leaves the middle at angle Angle and
// wanders as it goes, out to Length; Seed picks the wander
float SplitDistance(vec2 P, float Angle, float Length, float Seed, float From)
{
    float R = length(P);
    if (R < From || R > Length)
    {
        return 1.0;
    }
    float Wander = (Noise(R * 7.0 + Seed) - 0.5) * 0.9 +
                   (Noise(R * 23.0 + Seed * 3.1) - 0.5) * 0.25;
    float Along = Angle + Wander * R;
    vec2 Dir = vec2(cos(Along), sin(Along));
    vec2 Side = vec2(-Dir.y, Dir.x);
    float Across = abs(dot(P, Side));
    if (dot(P, Dir) < 0.0)
    {
        return 1.0;
    }
    // thick near the middle, a hairline at its tip
    float Width = mix(0.036, 0.004, clamp((R - From) / (Length - From), 0.0, 1.0));
    return Across / Width;
}

// 1 inside a split, 0 outside; P in the circle's units (-1..1)
float CrackAt(vec2 P, float Seed)
{
    float Result = 0.0;
    for (int Index = 0; Index < 9; Index++)
    {
        float I = float(Index);
        float Angle = (I + 0.7 * Hash(Seed + I * 1.3)) * 6.2831 / 9.0;
        float Length = 0.5 + 0.45 * Hash(Seed * 1.7 + I);
        float D = SplitDistance(P, Angle, Length, Seed + I * 11.0, 0.08);
        // a fork off the middle of every other split
        float Fork = 0.45 * Length;
        float Turn = (Hash(Seed + I * 5.7) > 0.5 ? 1.0 : -1.0) * (0.35 + 0.3 * Hash(I + Seed));
        vec2 ForkStart = Fork * vec2(cos(Angle), sin(Angle));
        float DF = 1.0;
        if (mod(I, 2.0) < 1.0)
        {
            DF = SplitDistance(P - ForkStart, Angle + Turn, Length * 0.55, Seed + I * 7.0, 0.0) * 1.4;
        }
        Result = max(Result, 1.0 - smoothstep(0.55, 1.0, min(D, DF)));
    }
    // a ring of short breaks round the middle, where the ground caved in
    float R = length(P);
    float Ring = abs(R - (0.2 + 0.03 * Noise(atan(P.y, P.x) * 4.0 + Seed)));
    float Gaps = step(0.45, Noise(atan(P.y, P.x) * 6.0 + Seed * 2.0));
    Result = max(Result, (1.0 - smoothstep(0.008, 0.02, Ring)) * Gaps);
    return Result;
}

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    float Seed = fragmentColor.r * 97.0 + 3.0;
    float Heat = fragmentColor.g;
    float Strength = fragmentColor.a;
    if (R > 1.0)
    {
        discard;
    }
    float Angle = atan(P.y, P.x);

    // scorched, broken ground with a ragged edge
    float Edge = 0.62 + 0.14 * Noise(Angle * 2.5 + Seed) + 0.08 * Noise(Angle * 9.0 + Seed * 2.0);
    float Scorch = (1.0 - smoothstep(Edge * 0.45, Edge, R)) * 0.38;
    Scorch *= 0.75 + 0.25 * Noise(R * 18.0 + Angle * 3.0 + Seed);

    float Crack = CrackAt(P, Seed);
    // the lip: lit ground just above a gap, as if the edge stood up
    float Lip = clamp(CrackAt(P + vec2(0.0, 0.03), Seed) - Crack, 0.0, 1.0);

    // rubble chips thrown round the middle, each a small square with a
    // shadow under it
    float Chip = 0.0;
    float ChipShadow = 0.0;
    for (int Index = 0; Index < 8; Index++)
    {
        float I = float(Index);
        float ChipAngle = Hash(Seed * 2.3 + I) * 6.2831;
        float ChipR = 0.28 + 0.6 * Hash(Seed * 4.1 + I);
        vec2 Centre = ChipR * vec2(cos(ChipAngle), sin(ChipAngle));
        float Size = 0.018 + 0.022 * Hash(Seed + I * 9.0);
        vec2 D = abs(P - Centre);
        Chip = max(Chip, 1.0 - step(Size, max(D.x, D.y)));
        vec2 S = abs(P - Centre - vec2(0.012, 0.018));
        ChipShadow = max(ChipShadow, 1.0 - step(Size, max(S.x, S.y)));
    }

    vec3 Ground = vec3(0.10, 0.08, 0.06);
    vec3 Gap = mix(vec3(0.04, 0.03, 0.025), vec3(1.0, 0.55, 0.15), Heat * (1.0 - 0.6 * R));
    vec3 LipColour = vec3(0.78, 0.70, 0.58);
    vec3 ChipColour = vec3(0.55, 0.47, 0.38);

    vec3 Colour = Ground;
    float Alpha = Scorch;
    Colour = mix(Colour, vec3(0.02), ChipShadow * (1.0 - Chip));
    Alpha = max(Alpha, ChipShadow * 0.45);
    Colour = mix(Colour, LipColour, Lip);
    Alpha = max(Alpha, Lip * 0.5);
    Colour = mix(Colour, Gap, Crack);
    Alpha = max(Alpha, Crack * (0.85 + 0.15 * Heat));
    Colour = mix(Colour, ChipColour, Chip);
    Alpha = max(Alpha, Chip * 0.9);

    FragColor = vec4(Colour, Alpha * Strength);
}
