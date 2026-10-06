// World grade: the post-process every frame's world goes through when no
// time rewind is bending it (client/world_grade.cpp). Reads the world,
// drawn into a texture this frame, and puts it on the window with:
//   glow:  bright coloured pixels (fire, lava, magic, flashes) bleed a soft
//          light over their neighbours, sampled on two rings around the
//          pixel; pale ground (snow, sand) does not
//   shoulder: highlights past 0.8 roll off instead of clipping
//   grade: a gentle S-curve for contrast, a little more saturation, cool
//          shadows and warm highlights, so the ground stops looking flat
//   dither: under one step of 8-bit noise, so the vignette and the glow
//          fade without bands
// Uniforms: Screen = texture width, height in pixels, glow strength, grade
// strength (both 0..1).

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform sampler2D WorldTexture;
uniform vec4 Screen;
uniform float Time;

vec3 World(vec2 Pixel)
{
    vec2 UV = clamp(Pixel / Screen.xy, vec2(0.0005), vec2(0.9995));
    return TEXTURE(WorldTexture, UV).rgb;
}

// NOTE(zoubir): how much of a pixel is bright enough to glow: the
// brightest channel past a soft knee, so a saturated orange glows as
// much as a white of the same peak. Pale pixels (snow, sand, stone) need
// a far higher peak, so only coloured light (fire, lava, magic) and true
// flashes glow and a snowfield keeps its detail
vec3 Bright(vec3 C)
{
    float Peak = max(C.r, max(C.g, C.b));
    float Low = min(C.r, min(C.g, C.b));
    float Saturation = (Peak - Low) / max(Peak, 0.001);
    float Knee = mix(0.97, 0.70, smoothstep(0.12, 0.45, Saturation));
    float Over = smoothstep(Knee, Knee + 0.28, Peak);
    return C * Over;
}

float Hash(vec2 P)
{
    return fract(sin(dot(P, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    vec2 Pixel = gl_FragCoord.xy;
    vec3 C = World(Pixel);

    // NOTE(zoubir): the rings scale with the screen's height, so the glow
    // covers the same part of the world at any resolution
    float Radius = Screen.y / 200.0;
    vec3 Glow = vec3(0.0);
    for (int Index = 0; Index < 8; Index++)
    {
        float Angle = float(Index) * 0.7854 + 0.39;
        vec2 Ray = vec2(cos(Angle), sin(Angle)) * Radius;
        Glow += Bright(World(Pixel + Ray));
        Glow += Bright(World(Pixel + Ray * 2.6)) * 0.55;
    }
    Glow *= 1.0 / 12.4;
    C += Glow * Screen.z * vec3(1.0, 0.93, 0.82);

    float Grade = Screen.w;
    float Luma = dot(C, vec3(0.2126, 0.7152, 0.0722));
    vec3 Graded = mix(vec3(Luma), C, 1.14);
    vec3 Curve = clamp(Graded, 0.0, 1.0);
    Curve = Curve * Curve * (3.0 - 2.0 * Curve);
    Graded = mix(Graded, Curve, 0.30);
    Graded += vec3(-0.012, 0.000, 0.022) * (1.0 - Luma) * (1.0 - Luma);
    Graded += vec3(0.022, 0.010, -0.014) * Luma * Luma;
    C = mix(C, Graded, Grade);

    // NOTE(zoubir): a soft shoulder past 0.8, so snow and lit stone roll
    // off toward white instead of clipping flat
    vec3 Over = max(C - 0.8, 0.0);
    C = min(C, 0.8) + Over / (1.0 + Over * 2.5);

    C += (Hash(Pixel + fract(Time) * 61.0) - 0.5) / 255.0;
    FragColor = vec4(C, 1.0);
}
