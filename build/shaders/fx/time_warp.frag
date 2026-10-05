// Time warp: the time rewinds' post-process (client/rewind_fx/time_warp.cpp).
// Reads the world, drawn into a texture this frame, and bends and recolours
// it inside each rewind's bubble (everywhere for a world rewind):
//   cast:     time thickens: a swirl tightening on the caster, a rim of
//             light drawn in around the bubble
//   hold:     the colour drains into a cold silver-blue, frost glints, a
//             shock ring sweeps out and bends the picture as it goes
//   playback: tape rewinding: tearing bands, a rolling tracking band,
//             magenta and cyan split, scanlines, a zoom pull to the centre
//   landing:  a ring of light goes out and the colour floods back
// Uniforms (see time_warp.cpp): Rewinds[i] = centre x, y in window pixels
// from the bottom left, radius in pixels, kind (0 self, 1 bubble, 2 world);
// Looks[i] = phase (1..4), progress 0..1, strength 0..1, seed; Screen =
// width, height, rewind count, world zoom.

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform sampler2D WarpTexture;
uniform vec4 Rewinds[4];
uniform vec4 Looks[4];
uniform vec4 Screen;
uniform float Time;

float Hash(vec2 P)
{
    return fract(sin(dot(P, vec2(127.1, 311.7))) * 43758.5453);
}

float Noise(vec2 P)
{
    vec2 I = floor(P);
    vec2 F = fract(P);
    F = F * F * (3.0 - 2.0 * F);
    float A = Hash(I);
    float B = Hash(I + vec2(1.0, 0.0));
    float C = Hash(I + vec2(0.0, 1.0));
    float D = Hash(I + vec2(1.0, 1.0));
    return mix(mix(A, B, F.x), mix(C, D, F.x), F.y);
}

vec3 ReadWorld(vec2 Pixel)
{
    vec2 UV = clamp(Pixel / Screen.xy, vec2(0.001), vec2(0.999));
    return TEXTURE(WarpTexture, UV).rgb;
}

vec3 ReadSplit(vec2 Pixel, vec2 Dir, float Split)
{
    vec3 Result;
    Result.r = ReadWorld(Pixel + Dir * Split).r;
    Result.g = ReadWorld(Pixel).g;
    Result.b = ReadWorld(Pixel - Dir * Split).b;
    return Result;
}

void main()
{
    // NOTE: wrapped, so mediump floats keep their precision after hours
    float Clock = mod(Time, 64.0);
    vec2 Pixel = gl_FragCoord.xy;
    vec2 Offset = vec2(0.0);
    float Frozen = 0.0;
    float Split = 0.0;
    vec2 SplitDir = vec2(1.0, 0.0);
    float Rewinding = 0.0;
    float Pull = 0.0;
    vec2 PullCentre = Pixel;
    vec3 Light = vec3(0.0);
    float Landing = 0.0;

    int Count = int(Screen.z + 0.5);
    for (int I = 0; I < 4; I++)
    {
        if (I >= Count)
        {
            break;
        }
        vec4 R = Rewinds[I];
        vec4 L = Looks[I];
        float Radius = max(R.z, 1.0);
        float Phase = L.x;
        float T = L.y;
        float Strength = L.z;
        vec2 D = Pixel - R.xy;
        float Dist = length(D);
        vec2 Dir = D / max(Dist, 0.001);
        float Angle = atan(D.y, D.x);
        bool Everywhere = R.w > 1.5;
        float Ragged = 0.035 * sin(Angle * 7.0 + Clock * 3.0 + L.w) +
            0.04 * (Noise(vec2(Angle * 3.0 + L.w, Clock * 2.0)) - 0.5);
        float Edge = Radius * (1.0 + Ragged);
        float Inside = Everywhere ? 1.0 : 1.0 - smoothstep(Edge - 10.0, Edge + 4.0, Dist);
        float Rim = Everywhere ? 0.0 : exp(-abs(Dist - Edge) / 5.0);

        if (Phase < 1.5)
        {
            float Reach = Everywhere ? Radius * 0.35 * T : Radius * 1.25;
            float Fall = clamp(1.0 - Dist / max(Reach, 1.0), 0.0, 1.0);
            float Twist = (Everywhere ? 0.25 : 0.9) * T * T * Fall * Fall;
            float S = sin(-Twist);
            float C = cos(-Twist);
            vec2 Turned = vec2(C * D.x - S * D.y, S * D.x + C * D.y);
            Offset += (Turned - D) * Strength;
            Frozen = max(Frozen, 0.25 * T * Inside * (Everywhere ? 0.6 : 1.0));
            Light += vec3(0.35, 0.8, 1.0) * Rim * T * 0.8 * Strength;
            if (!Everywhere)
            {
                float Gather = Radius * (1.0 - T) + 6.0;
                Light += vec3(0.5, 0.9, 1.0) * exp(-abs(Dist - Gather) / 3.0) * T * 0.6;
            }
        }
        else if (Phase < 2.5)
        {
            float In = smoothstep(0.0, 0.12, T);
            Frozen = max(Frozen, Inside * In * Strength);
            float Swept = 1.0 - pow(1.0 - min(T * 1.6, 1.0), 3.0);
            float Ring = (Everywhere ? Radius * 0.5 : Radius * 1.15) * Swept;
            float Band = exp(-abs(Dist - Ring) / (Everywhere ? 26.0 : 12.0));
            float Fade = 1.0 - smoothstep(0.45, 0.65, T);
            Offset -= Dir * Band * (Everywhere ? 34.0 : 16.0) * Fade * Strength;
            Light += vec3(0.6, 0.9, 1.0) * Band * Fade * 0.45;
            Split = max(Split, (2.0 + 4.0 * Band) * Inside * In);
            SplitDir = Dir;
            Light += vec3(0.4, 0.85, 1.0) * Rim * 0.9;
            float Frost = Noise(Pixel * 0.09 + vec2(L.w, 0.0)) *
                Noise(Pixel * 0.023 - vec2(Clock * 0.2));
            Light += vec3(0.25, 0.45, 0.7) * smoothstep(0.55, 0.9, Frost) * Inside * In * 0.35;
        }
        else if (Phase < 3.5)
        {
            float Env = smoothstep(0.0, 0.08, T) * (1.0 - smoothstep(0.88, 1.0, T));
            float Amount = Inside * Strength;
            Frozen = max(Frozen, Amount * (0.45 + 0.2 * Env));
            Rewinding = max(Rewinding, Amount * Env);
            Pull = max(Pull, Amount * Env * (Everywhere ? 0.012 : 0.06));
            PullCentre = R.xy;
            float Row = floor((Pixel.y + fract(Clock * 0.25) * 3040.0) / 18.0);
            float Tick = floor(Clock * 24.0);
            float Tear = (Hash(vec2(Row, Tick)) - 0.5) *
                step(0.62, Hash(vec2(Row * 1.7, Tick + 3.0)));
            Offset.x += Tear * (Everywhere ? 20.0 : 32.0) * Amount * Env;
            float Track = fract(Clock * 0.9 + L.w);
            float TrackBand = exp(-abs(Pixel.y / Screen.y - (1.0 - Track)) * 60.0);
            Offset.x += TrackBand * 16.0 * Amount * Env * sin(Pixel.y * 0.5 + Clock * 40.0);
            Split = max(Split, (3.0 + (Everywhere ? 2.0 : 4.0) * Env) * Amount);
            SplitDir = Everywhere ? vec2(1.0, 0.0) : normalize(vec2(1.0, 0.0) + Dir * 0.6);
            Light += vec3(1.0, 0.35, 0.95) * Rim * 0.8 +
                vec3(0.3, 1.0, 1.0) * Rim * 0.5 * sin(Clock * 30.0);
        }
        else
        {
            float Ring = (Everywhere ? Radius * 0.55 : Radius * 1.3) * (1.0 - pow(1.0 - T, 2.5));
            float Band = exp(-abs(Dist - Ring) / (Everywhere ? 30.0 : 10.0)) * (1.0 - T);
            Offset += Dir * Band * (Everywhere ? 24.0 : 12.0);
            Light += vec3(1.0, 0.92, 0.7) * Band * 0.7;
            Landing = max(Landing, Inside * (1.0 - T));
            Frozen = max(Frozen, Inside * pow(1.0 - T, 3.0) * 0.8);
            Split = max(Split, 4.0 * Band);
            SplitDir = Dir;
        }
    }

    vec2 From = Pixel + Offset;
    vec3 Color = vec3(0.0);
    if (Pull > 0.001)
    {
        for (int Tap = 0; Tap < 6; Tap++)
        {
            float K = float(Tap) / 5.0;
            Color += ReadSplit(mix(From, PullCentre, Pull * K), SplitDir, Split);
        }
        Color /= 6.0;
    }
    else
    {
        Color = ReadSplit(From, SplitDir, Split);
    }

    float Luma = dot(Color, vec3(0.299, 0.587, 0.114));
    vec3 Cold = mix(vec3(0.03, 0.06, 0.14), vec3(0.78, 0.93, 1.08),
                    smoothstep(0.02, 0.95, Luma));
    vec3 Tape = mix(vec3(0.10, 0.02, 0.16), vec3(1.05, 0.62, 1.10), Luma);
    Tape = mix(Tape, vec3(0.45, 1.05, 1.10) * Luma,
               0.35 + 0.35 * sin(Pixel.y * 0.12 + Clock * 9.0));
    vec3 Graded = mix(Color, Cold, Frozen);
    Graded = mix(Graded, Tape, Rewinding * 0.4);
    float Scan = 0.5 + 0.5 * sin(Pixel.y * 3.14159);
    Graded *= 1.0 - Rewinding * 0.16 * Scan;
    Graded += (Hash(Pixel + vec2(fract(Clock) * 100.0)) - 0.5) * 0.10 *
        max(Rewinding, Frozen * 0.5);
    Graded = mix(Graded, Color * 1.15 + vec3(0.04, 0.03, 0.0), Landing * 0.6);
    Graded += Light;
    vec2 Q = Pixel / Screen.xy - 0.5;
    Graded *= 1.0 - max(Frozen, Rewinding) * 0.45 * smoothstep(0.25, 0.75, length(Q));
    FragColor = vec4(Graded, 1.0);
}
