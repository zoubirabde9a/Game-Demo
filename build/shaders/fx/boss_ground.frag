// Boss ground: the marks boss abilities paint on the floor, drawn in the
// world pass under the bodies so they are graded and glow
// (client/dungeon/boss_fx/). One quad per mark, its circle touching the
// quad's edge.
//   colour r: progress, 0 to 1 (what it means depends on the look)
//   colour g: the look, in steps of 16 (BossGround_* in boss_fx_draw.cpp)
//     0 gravity well: a violet whirlpool turning in on a red core
//     1 eclipse light: a pool of gold with a countdown round its rim
//     2 void maw: a black pit opening, fangs round it closing in
//     3 falling star: the star's mark, a gold sigil and a growing shadow
//     4 sunken dark: a black pool rippling where Ommoroth went down
//     5 black sun: a black disc ringed with gold fire, where Nyxara rose
//     6 smite: a crimson rune circle closing on its victim
//     7 eclipse dark: the dark spreading out from the caster
//     8 wave: a band rolling out to be jumped, its front at the progress
//     9 void brand: a violet ring the size of the burst round the
//       branded, its countdown closing in gold
//    10 mirror: a shell of glass round a body, a glint sweeping over it,
//       the time it has left in a gold arc
//   colour b: a second number (the core's share of the radius for a well,
//             the time left for an eclipse light or a mirror, for a wave
//             0 frost and 1 fire)
//   colour a: strength

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

#define TAU 6.2831853

// NOTE(zoubir): a point K out on the circle at angle A: noise read this
// way wraps round, where noise read along the angle itself leaves a seam
// where atan jumps from pi to -pi
vec2 Around(float A, float K)
{
    return vec2(cos(A), sin(A)) * K;
}

// NOTE(zoubir): 1 on a line of half width W round Distance 0, soft edged
float Line(float Distance, float W)
{
    return 1.0 - smoothstep(W * 0.5, W, abs(Distance));
}

vec4 GravityWell(vec2 P, float R, float A, float Progress, float Core)
{
    // the whirlpool: arms that wind tighter toward the middle and run in
    float Swirl = A + 2.6 / (R + 0.25) - Time * (1.2 + 2.0 * Progress);
    float Arms = smoothstep(0.2, 0.9, 0.5 + 0.5 * sin(5.0 * Swirl));
    float Flow = Fbm(Around(Swirl, 1.6) + vec2(log(R + 0.05) * 3.0 + Time * 1.5, 0.0));
    float Lit = Arms * (0.4 + 0.6 * Flow);
    float Fade = smoothstep(1.0, 0.6, R);
    vec3 Deep = vec3(0.1, 0.02, 0.2);
    vec3 Violet = vec3(0.55, 0.32, 0.95);
    vec3 Colour = mix(Deep, Violet, Lit);
    float Alpha = Fade * (0.3 + 0.45 * Lit) * (0.6 + 0.4 * Progress);
    // flecks of the dead star's gold caught in the arms
    float Fleck = step(0.88, Noise(Around(Swirl, 6.0) + vec2(R * 14.0 - Time * 3.0, 0.0))) * Fade * Arms;
    Colour = mix(Colour, vec3(1.0, 0.82, 0.45), Fleck * 0.7);
    Alpha = max(Alpha, Fleck * 0.55);
    // the reach, a thin rim
    float Rim = Line(R - 0.985, 0.02) * (0.45 + 0.2 * sin(Time * 4.0 + A * 3.0));
    Colour = mix(Colour, Violet, Rim);
    Alpha = max(Alpha, Rim);
    // the core: dark, fringed in red that burns brighter as it collapses
    float Inside = 1.0 - smoothstep(Core - 0.03, Core, R);
    Colour = mix(Colour, vec3(0.03, 0.0, 0.06), Inside);
    Alpha = max(Alpha, Inside * (0.55 + 0.3 * Progress));
    float CoreEdge = Line(R - Core, 0.012 + 0.012 * Progress) * (0.55 + 0.35 * Progress);
    Colour = mix(Colour, vec3(0.95, 0.28, 0.4), CoreEdge);
    Alpha = max(Alpha, CoreEdge);
    return vec4(Colour, Alpha);
}

vec4 EclipseLight(vec2 P, float R, float A, float Progress, float Left)
{
    vec3 Gold = vec3(1.0, 0.82, 0.4);
    vec3 Pale = vec3(1.0, 0.96, 0.82);
    // light moving on the floor like through water
    float Caustic = Fbm(P * 3.0 + vec2(Time * 0.4, -Time * 0.3));
    Caustic = smoothstep(0.35, 0.75, Caustic);
    float Fill = smoothstep(1.0, 0.0, R) * 0.45 + 0.35 * Caustic * smoothstep(1.0, 0.5, R);
    // rays turning slowly from the middle
    float Rays = pow(0.5 + 0.5 * sin(A * 12.0 + Time * 0.8), 6.0) * smoothstep(1.0, 0.2, R) * 0.35;
    float Rim = Line(R - 0.92, 0.045);
    // the countdown: an arc round the outside, from the top, shrinking
    float Turn = fract(-(A / TAU) + 0.25);
    float Arc = Line(R - 0.99, 0.03) * step(Turn, Left);
    float Alpha = (Fill + Rays) * (0.6 + 0.4 * Progress) + Rim + Arc;
    vec3 Colour = mix(Gold, Pale, clamp(Rim + Arc + Rays, 0.0, 1.0));
    return vec4(Colour * (1.0 + 0.6 * Progress), clamp(Alpha, 0.0, 1.0));
}

vec4 VoidMaw(vec2 P, float R, float A, float Progress)
{
    // the pit opens as the windup runs, its edge torn
    float Torn = 0.08 * (Noise(Around(A, 4.0) + vec2(Time * 0.8, 0.0)) - 0.5);
    float Mouth = 0.18 + 0.62 * Progress + Torn;
    float Inside = 1.0 - smoothstep(Mouth - 0.05, Mouth, R);
    float Lip = Line(R - Mouth, 0.045);
    // fangs round the reach, pointing in, sliding in as it closes
    float Fangs = 9.0;
    float Slot = fract(A / TAU * Fangs + 0.5) - 0.5;
    float Tip = 0.98 - 0.28 * Progress;
    float FangShape = step(abs(Slot) * 6.0, (R - Tip) / (1.0 - Tip + 0.001) + 0.0) *
        step(Tip, R) * step(R, 0.99);
    float Reach = Line(R - 0.99, 0.02);
    vec3 Colour = vec3(0.02, 0.0, 0.04);
    float Alpha = Inside * 0.92;
    vec3 Glow = vec3(0.75, 0.22, 0.9);
    Colour = mix(Colour, Glow * 1.1, Lip);
    Alpha = max(Alpha, Lip);
    Colour = mix(Colour, vec3(0.86, 0.88, 1.0), FangShape);
    Alpha = max(Alpha, FangShape * (0.5 + 0.5 * Progress));
    Colour = mix(Colour, Glow, Reach * (1.0 - FangShape));
    Alpha = max(Alpha, Reach * 0.6);
    // the floor round the mouth darkens toward it
    float Shade = smoothstep(1.0, Mouth, R) * 0.35 * Progress;
    Alpha = max(Alpha, Shade);
    return vec4(Colour, Alpha);
}

vec4 StarMark(vec2 P, float R, float A, float Progress)
{
    vec3 Gold = vec3(1.0, 0.8, 0.35);
    float Shadow = (1.0 - smoothstep(0.15 + 0.6 * Progress - 0.1, 0.15 + 0.6 * Progress, R)) * 0.55;
    float Ring = Line(R - 0.96, 0.035);
    float Inner = Line(R - 0.8, 0.02);
    // ticks round the sigil, turning
    float Ticks = step(0.8, fract(A / TAU * 24.0 + Time * 0.3)) * step(0.82, R) * step(R, 0.94);
    // four lines converging on the middle as it comes down
    float Cross = Line(abs(sin(A * 2.0 + 0.785)) * R, 0.015) * step(R, 0.8) *
        smoothstep(0.0, 0.3, R) * Progress;
    float Light = max(max(Ring, Inner * 0.7), max(Ticks * 0.8, Cross));
    vec3 Colour = mix(vec3(0.03, 0.02, 0.01), Gold * (1.0 + Progress), clamp(Light * 2.0, 0.0, 1.0));
    return vec4(Colour, max(Shadow, Light));
}

vec4 SunkenDark(vec2 P, float R, float A)
{
    // ripples running out from the middle, broken by noise
    float Ripple = 0.5 + 0.5 * sin(R * 22.0 - Time * 3.0 + 2.0 * Noise(P * 3.0 + Time * 0.2));
    float Pool = smoothstep(1.0, 0.7, R);
    vec3 Colour = mix(vec3(0.02, 0.0, 0.05), vec3(0.4, 0.2, 0.75), pow(Ripple, 6.0) * 0.7);
    // the light deep down, red, beating slowly
    float Heart = exp(-R * R * 28.0) * (0.7 + 0.3 * sin(Time * 2.0));
    Colour = mix(Colour, vec3(1.0, 0.3, 0.4) * 1.5, Heart);
    float Rim = Line(R - 0.9, 0.06) * (0.4 + 0.2 * sin(A * 5.0 - Time * 1.5));
    Colour = mix(Colour, vec3(0.6, 0.35, 1.0), Rim);
    return vec4(Colour, max(Pool * 0.85, max(Heart, Rim)));
}

vec4 BlackSun(vec2 P, float R, float A)
{
    float Disc = 1.0 - smoothstep(0.58, 0.62, R);
    // the corona: tongues of gold fire licking out from the disc
    float Tongues = Fbm(Around(A, 3.0) + vec2(0.0, R * 4.0 - Time * 1.4));
    float Corona = smoothstep(0.98, 0.6, R) * step(0.58, R) * smoothstep(0.25, 0.8, Tongues);
    float Edge = Line(R - 0.6, 0.04);
    vec3 Colour = vec3(0.02, 0.01, 0.03);
    Colour = mix(Colour, vec3(1.0, 0.72, 0.28) * 1.5, clamp(Corona * 1.4 + Edge, 0.0, 1.0));
    return vec4(Colour, max(Disc * 0.92, max(Corona, Edge)));
}

vec4 SmiteMark(vec2 P, float R, float A, float Progress)
{
    vec3 Crimson = vec3(1.0, 0.16, 0.42);
    float Outer = Line(R - 0.94, 0.05);
    // the inner ring closes on the victim as the windup runs out
    float Close = 0.9 - 0.75 * Progress;
    float Inner = Line(R - Close, 0.05);
    // runes: broken marks between the rings, turning against each other
    float Turn = A / TAU + Time * 0.05;
    float Runes = step(0.45, Noise(vec2(floor(Turn * 24.0), 3.0))) *
        step(0.76, R) * step(R, 0.88) * step(0.25, fract(Turn * 24.0));
    float Spokes = Line(abs(sin(A * 3.0 - Time * 0.6)) * R, 0.03) * step(Close, R) * step(R, 0.76);
    float Fill = (1.0 - smoothstep(Close - 0.05, Close, R)) * (0.18 + 0.4 * Progress * Progress);
    float Stain = smoothstep(1.0, 0.85, R) * 0.22;
    float Light = max(max(Outer, Inner), max(Runes, Spokes * 0.6));
    vec3 Colour = mix(Crimson * 0.9, vec3(1.0, 0.75, 0.82), Inner * Progress);
    float Beat = 0.8 + 0.2 * sin(Time * (6.0 + 20.0 * Progress));
    return vec4(Colour, max(Light * Beat, max(Fill, Stain)));
}

vec4 EclipseDark(vec2 P, float R, float A, float Progress)
{
    // a front of shadow rolling out, smoke at its edge
    float Smoke = Fbm(Around(A, 2.5) + vec2(0.0, R * 3.0 - Time * 0.7));
    float Front = Progress + 0.08 * (Smoke - 0.5);
    float Behind = 1.0 - smoothstep(Front - 0.12, Front, R);
    float Edge = Line(R - Front, 0.05);
    vec3 Colour = mix(vec3(0.03, 0.0, 0.06), vec3(0.45, 0.25, 0.8), Edge);
    return vec4(Colour, max(Behind * 0.45, Edge * 0.6));
}

vec4 WaveBand(vec2 P, float R, float A, float Front, float Fire)
{
    // the front, torn by noise, with light trailing behind it
    float Torn = 0.025 * (Noise(Around(A, 9.0) + vec2(Time * 2.0, 0.0)) - 0.5);
    float Gap = R - Front + Torn;
    float Band = 1.0 - smoothstep(0.0, 0.025, abs(Gap));
    float Trail = step(Gap, 0.0) * exp(Gap * 14.0) * 0.45;
    // frost: facets of ice catching the light; fire: tongues licking up
    float Grain = Fire > 0.5 ?
        Noise(Around(A, 14.0) + vec2(R * 30.0 - Time * 6.0, 0.0)) :
        step(0.5, Noise(Around(A, 26.0) + vec2(R * 40.0, 0.0)));
    vec3 Cold = mix(vec3(0.45, 0.75, 1.0), vec3(0.92, 0.98, 1.0), Grain);
    vec3 Hot = mix(vec3(1.0, 0.38, 0.1), vec3(1.0, 0.86, 0.45), Grain);
    vec3 Colour = Fire > 0.5 ? Hot : Cold;
    float Reach = Line(R - 0.99, 0.012) * 0.35;
    float Light = max(Band * (0.7 + 0.3 * Grain), max(Trail * (0.6 + 0.4 * Grain), Reach));
    return vec4(Colour * (1.0 + Band * 0.4), Light);
}

vec4 BrandMark(vec2 P, float R, float A, float Progress)
{
    vec3 Violet = vec3(0.72, 0.35, 1.0);
    vec3 Gold = vec3(1.0, 0.82, 0.4);
    float Beat = 0.7 + 0.3 * sin(Time * (5.0 + 14.0 * Progress));
    float Ring = Line(R - 0.96, 0.03) * Beat;
    // the countdown: a gold arc from the top, closing as the burst nears
    float Turn = fract(-(A / TAU) + 0.25);
    float Arc = Line(R - 0.9, 0.025) * step(Turn, 1.0 - Progress);
    // a haze of the void inside, thickening, with ticks turning round it
    float Haze = smoothstep(0.95, 0.2, R) * (0.08 + 0.22 * Progress) *
        (0.7 + 0.3 * Noise(P * 4.0 + Time * 0.6));
    float Ticks = step(0.85, fract(A / TAU * 16.0 - Time * 0.4)) * step(0.82, R) * step(R, 0.86);
    vec3 Colour = mix(Violet, Gold, clamp(Arc * 1.2, 0.0, 1.0));
    return vec4(Colour * (1.0 + 0.4 * Ring), max(max(Ring, Arc), max(Haze, Ticks * 0.7)));
}

vec4 MirrorShell(vec2 P, float R, float A, float Progress, float Left)
{
    // a hexagon of glass, its distance to the edge measured flat side on
    vec2 Q = abs(P);
    float Hex = max(Q.x * 0.866 + Q.y * 0.5, Q.y);
    float Edge = Line(Hex - 0.86, 0.035);
    float Inside = 1.0 - smoothstep(0.84, 0.87, Hex);
    // facets: lines from the middle to the corners, faint
    float Facets = Line(abs(sin(A * 3.0 + 1.5708)) * R, 0.015) * Inside * 0.35;
    // a glint sweeping across the glass
    float Sweep = fract(Time * 0.45) * 3.0 - 1.5;
    float Glint = Line(P.x * 0.6 + P.y * 0.8 - Sweep, 0.09) * Inside;
    float Glass = Inside * (0.1 + 0.1 * (P.y * -0.5 + 0.5));
    float Arc = Line(R - 0.97, 0.02) * step(fract(-(A / TAU) + 0.25), Left);
    vec3 Silver = vec3(0.86, 0.9, 1.0);
    vec3 Colour = mix(Silver, vec3(1.0), Glint);
    Colour = mix(Colour, vec3(1.0, 0.82, 0.4), Arc);
    float Alpha = max(max(Edge, Glint * 0.85), max(Glass, max(Facets, Arc)));
    return vec4(Colour, Alpha * (0.4 + 0.6 * Progress));
}

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    if (R > 1.0)
    {
        discard;
    }
    float A = atan(P.y, P.x);
    float Progress = fragmentColor.r;
    float Look = floor(fragmentColor.g * 15.9375 + 0.5);
    float Param = fragmentColor.b;
    vec4 Result;
    if (Look < 0.5)
    {
        Result = GravityWell(P, R, A, Progress, Param);
    }
    else if (Look < 1.5)
    {
        Result = EclipseLight(P, R, A, Progress, Param);
    }
    else if (Look < 2.5)
    {
        Result = VoidMaw(P, R, A, Progress);
    }
    else if (Look < 3.5)
    {
        Result = StarMark(P, R, A, Progress);
    }
    else if (Look < 4.5)
    {
        Result = SunkenDark(P, R, A);
    }
    else if (Look < 5.5)
    {
        Result = BlackSun(P, R, A);
    }
    else if (Look < 6.5)
    {
        Result = SmiteMark(P, R, A, Progress);
    }
    else if (Look < 7.5)
    {
        Result = EclipseDark(P, R, A, Progress);
    }
    else if (Look < 8.5)
    {
        Result = WaveBand(P, R, A, Progress, Param);
    }
    else if (Look < 9.5)
    {
        Result = BrandMark(P, R, A, Progress);
    }
    else
    {
        Result = MirrorShell(P, R, A, Progress, Param);
    }
    FragColor = vec4(Result.rgb, clamp(Result.a, 0.0, 1.0) * fragmentColor.a);
}
