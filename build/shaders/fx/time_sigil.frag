// Time sigil: a clock face of light, drawn under a rewind's caster and over
// a bubble (client/rewind_fx/rewind_draw.cpp). A double rim, twelve ticks
// (every third long), a ring of runes turning against the hands, and two
// hands whose angle the caller turns backwards. Drawn additive.
//   colour r: brightness 0..1
//   colour g: the minute hand's angle, a whole turn in 0..1 (the hour hand
//             follows at a twelfth)
//   colour b: 0 cyan and gold (cast, hold), 1 magenta (playback)
//   colour a: strength

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

void main()
{
    float Clock = mod(Time, 64.0);
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    float A = atan(P.y, P.x);
    float Bright = fragmentColor.r;
    float Hand = fragmentColor.g * 6.28318;
    float Mode = fragmentColor.b;

    vec3 Cyan = vec3(0.35, 0.9, 1.0);
    vec3 Gold = vec3(1.0, 0.82, 0.45);
    vec3 Magenta = vec3(1.0, 0.4, 0.95);
    vec3 Tint = mix(Cyan, Magenta, smoothstep(0.5, 1.0, Mode));

    float Ink = smoothstep(0.035, 0.0, abs(R - 0.92));
    Ink += 0.6 * smoothstep(0.02, 0.0, abs(R - 0.84));

    float Sector = A / 6.28318 * 12.0;
    float Index = floor(Sector + 0.5);
    float Arc = abs(Sector - Index) / 12.0 * 6.28318 * R;
    float Major = 1.0 - step(0.5, mod(Index + 12.0, 3.0));
    float TickInner = mix(0.74, 0.64, Major);
    Ink += smoothstep(mix(0.018, 0.028, Major), 0.0, Arc) * step(TickInner, R) * step(R, 0.84);

    float Spin = Clock * 0.8 * (1.0 + 3.0 * Mode);
    float Runes = fract((A + Spin) / 6.28318 * 36.0);
    Ink += 0.55 * step(abs(R - 0.56), 0.022) * step(0.3, Runes) * step(Runes, 0.8);
    float Inner = fract((A - Spin * 0.5) / 6.28318 * 8.0);
    Ink += 0.35 * step(abs(R - 0.38), 0.012) * step(0.15, Inner) * step(Inner, 0.65);

    vec2 Minute = vec2(cos(Hand), sin(Hand)) * 0.72;
    vec2 Hour = vec2(cos(Hand / 12.0), sin(Hand / 12.0)) * 0.45;
    Ink += smoothstep(0.03, 0.0, Segment(P, vec2(0.0), Minute));
    Ink += smoothstep(0.045, 0.0, Segment(P, vec2(0.0), Hour));
    Ink += 1.5 * smoothstep(0.09, 0.0, R);

    float Glow = exp(-R * 2.5) * 0.25 + exp(-abs(R - 0.92) * 14.0) * 0.35;
    vec3 Rgb = Tint * (Ink + Glow) * (0.6 + 0.6 * Bright) +
        Gold * Ink * 0.3 * (1.0 - Mode);
    float Alpha = clamp(Ink + Glow, 0.0, 1.0) * fragmentColor.a * step(R, 1.0);
    FragColor = vec4(Rgb, Alpha);
}
