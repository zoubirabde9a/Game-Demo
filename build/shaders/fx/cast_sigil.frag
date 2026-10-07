// Cast sigil: the circle of light on the ground under a player winding up
// a spell (client/cast_fx.cpp). A double rim whose outer band fills
// clockwise from the top as the cast runs, a ring of glyphs turning one
// way, a figure in the middle turning the other, and a light at the
// centre. Everything brightens and turns faster as the cast nears its end.
// Drawn additive.
//   colour rgb: the spell's colour, already dimmed by the fade (dark adds
//               nothing)
//   colour a:   how far the cast is, 0..1
//   uv:         0..1 across the quad, plus 2 * the figure on x:
//               0 arcane star, 1 frost spokes, 2 fire star, 3 gravity spiral

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

float Segment(vec2 P, vec2 A, vec2 B)
{
    vec2 PA = P - A;
    vec2 BA = B - A;
    float H = clamp(dot(PA, BA) / dot(BA, BA), 0.0, 1.0);
    return length(PA - BA * H);
}

// distance to the edge of a regular polygon of Sides, corners at Radius,
// turned by Turn
float Polygon(vec2 P, float Sides, float Radius, float Turn)
{
    float Wedge = 6.28318 / Sides;
    float A = atan(P.y, P.x) - Turn;
    float Local = mod(A, Wedge) - 0.5 * Wedge;
    return abs(length(P) * cos(Local) - Radius * cos(0.5 * Wedge));
}

float Hash(float N)
{
    return fract(sin(N * 91.345) * 47453.5453);
}

// one glyph in a cell, Q in -1..1 across it: strokes picked by Seed
float Glyph(vec2 Q, float Seed)
{
    float Ink = 0.0;
    float W = 0.16;
    Ink += step(0.5, Hash(Seed + 1.0)) * smoothstep(W, 0.0, abs(Q.x)) * step(abs(Q.y), 0.8);
    Ink += step(0.4, Hash(Seed + 2.0)) * smoothstep(W, 0.0, abs(Q.y - 0.6)) * step(abs(Q.x), 0.6);
    Ink += step(0.6, Hash(Seed + 3.0)) * smoothstep(W, 0.0, abs(Q.y + 0.6)) * step(abs(Q.x), 0.6);
    Ink += step(0.5, Hash(Seed + 4.0)) * smoothstep(W, 0.0, Segment(Q, vec2(-0.6, -0.8), vec2(0.6, 0.8)));
    Ink += step(0.7, Hash(Seed + 5.0)) * smoothstep(0.3, 0.1, length(Q - vec2(0.0, 0.05)));
    return min(Ink, 1.0);
}

void main()
{
    float Figure = floor(fragmentUV.x * 0.5);
    vec2 P = vec2(fragmentUV.x - 2.0 * Figure, fragmentUV.y) * 2.0 - 1.0;
    float R = length(P);
    float A = atan(P.y, P.x);
    float Progress = fragmentColor.a;
    float Clock = mod(Time, 256.0);
    float Spin = Clock * (0.5 + 1.6 * Progress * Progress);

    // the rims, and the outer band filling clockwise from the top
    float Ink = smoothstep(0.03, 0.0, abs(R - 0.95));
    Ink += 0.7 * smoothstep(0.02, 0.0, abs(R - 0.84));
    float Around = fract(atan(P.x, -P.y) / 6.28318 + 1.0);
    float Band = step(0.85, R) * step(R, 0.94);
    Ink += Band * step(Around, Progress) * 0.55;
    Ink += Band * smoothstep(0.04, 0.0, abs(Around - Progress)) * 1.4;

    // the ring of glyphs between the rims
    float Cells = 20.0;
    float Turned = (A + Spin * 0.6) / 6.28318 * Cells;
    float Cell = floor(Turned);
    vec2 Q = vec2((fract(Turned) - 0.5) * 2.4, (R - 0.73) / 0.075);
    float InRing = step(abs(R - 0.73), 0.085);
    Ink += 0.8 * InRing * Glyph(Q, mod(Cell, Cells));
    Ink += 0.5 * smoothstep(0.015, 0.0, abs(R - 0.62));

    // the figure in the middle, turning against the glyphs
    float Turn = -Spin * 0.8;
    float Shape = 0.0;
    if (Figure < 0.5)
    {
        Shape = smoothstep(0.025, 0.0, Polygon(P, 3.0, 0.58, Turn));
        Shape += smoothstep(0.025, 0.0, Polygon(P, 3.0, 0.58, Turn + 1.0472));
        Shape += 0.6 * smoothstep(0.02, 0.0, abs(R - 0.3));
    }
    else if (Figure < 1.5)
    {
        float Wedge = 6.28318 / 6.0;
        float Local = mod(A - Turn, Wedge) - 0.5 * Wedge;
        vec2 S = R * vec2(cos(Local), sin(Local));
        Shape = smoothstep(0.025, 0.0, abs(S.y)) * step(R, 0.58);
        Shape += smoothstep(0.02, 0.0, Segment(S, vec2(0.36, 0.0), vec2(0.48, 0.1)));
        Shape += smoothstep(0.02, 0.0, Segment(S, vec2(0.36, 0.0), vec2(0.48, -0.1)));
        Shape += smoothstep(0.025, 0.0, Polygon(P, 6.0, 0.3, Turn));
    }
    else if (Figure < 2.5)
    {
        for (int I = 0; I < 5; I++)
        {
            float From = Turn + float(I) * 2.51327;
            float To = From + 2.51327;
            Shape += smoothstep(0.025, 0.0, Segment(P, 0.58 * vec2(cos(From), sin(From)),
                                                    0.58 * vec2(cos(To), sin(To))));
        }
        // tongues of flame licking up off the inner rim
        float Lick = sin(A * 9.0 + Clock * 7.0) * 0.5 + 0.5;
        Shape += 0.6 * smoothstep(0.08, 0.0, abs(R - 0.62 + 0.06 * Lick)) * Lick;
    }
    else
    {
        float Arms = fract((A - Turn * 2.0) / 6.28318 * 3.0 + R * 1.4);
        Shape = smoothstep(0.12, 0.0, abs(Arms - 0.5) - 0.02) * step(R, 0.6) *
            smoothstep(0.05, 0.2, R);
    }
    Ink += min(Shape, 1.2);

    // the light at the centre swells with the cast
    float Core = exp(-R * 5.0) * (0.3 + 1.2 * Progress);
    float Halo = exp(-abs(R - 0.95) * 12.0) * 0.25 + exp(-R * 1.8) * 0.12;

    // the last stretch flickers hot
    float Hot = smoothstep(0.8, 1.0, Progress);
    float Bright = 0.55 + 0.6 * Progress + Hot * (0.3 + 0.3 * sin(Clock * 40.0));
    vec3 Rgb = fragmentColor.rgb * (Ink + Halo + Core) * Bright +
        vec3(1.0) * (Core * 0.6 + Ink * 0.25 * Hot);
    float Alpha = clamp(Ink + Halo + Core, 0.0, 1.0) * step(R, 1.0);
    FragColor = vec4(Rgb, Alpha);
}
