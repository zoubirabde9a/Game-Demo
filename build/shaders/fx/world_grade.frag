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
//   sky:   two kinds of slow light change pinned to the map, so a wide
//          plain stops reading as one tile repeated: broad patches a
//          little lighter or darker, and soft cloud shadows drifting over
//          it
//   lights: fireballs, shots and lava (client/world_lights.cpp) brighten
//          and tint what is near them, the albedo times the light, so
//          dark grass turns warm instead of grey
// Uniforms: Screen = texture width, height in pixels, glow strength, grade
// strength (both 0..1). LightSpot[i] = x, y, radius (window pixels),
// strength; LightColor[i] = r, g, b, flicker; LightCount.x = how many.
// WorldView = camera x, y (world units), window pixels per world unit,
// window height.

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform sampler2D WorldTexture;
uniform vec4 Screen;
uniform float Time;

#define MAX_LIGHTS 32
uniform vec4 LightSpot[MAX_LIGHTS];
uniform vec4 LightColor[MAX_LIGHTS];
uniform vec4 LightCount;
uniform vec4 WorldView;

// NOTE(zoubir): the light every source throws on this pixel; a smooth
// (1 - d^2)^2 falloff that reaches zero at the radius, and fire wavers
vec3 Lights(vec2 Pixel)
{
    vec3 Sum = vec3(0.0);
    for (int Index = 0; Index < MAX_LIGHTS; Index++)
    {
        if (float(Index) >= LightCount.x) break;
        vec4 Spot = LightSpot[Index];
        vec4 Color = LightColor[Index];
        vec2 Away = (Pixel - Spot.xy) / Spot.z;
        float Near = max(1.0 - dot(Away, Away), 0.0);
        float Waver = 1.0 + Color.w * 0.12 *
            sin(Time * 13.0 + float(Index) * 2.3) * sin(Time * 7.1 + float(Index));
        Sum += Color.rgb * (Spot.w * Waver * Near * Near);
    }
    return Sum;
}

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

// NOTE(zoubir): smooth value noise and three octaves of it; the cell
// coordinates wrap at 289 so the hash keeps its precision far from the
// origin of an endless map
float Noise(vec2 P)
{
    vec2 Cell = floor(P);
    vec2 F = P - Cell;
    F = F * F * (3.0 - 2.0 * F);
    Cell = mod(Cell, 289.0);
    float A = Hash(Cell);
    float B = Hash(mod(Cell + vec2(1.0, 0.0), 289.0));
    float C = Hash(mod(Cell + vec2(0.0, 1.0), 289.0));
    float D = Hash(mod(Cell + vec2(1.0, 1.0), 289.0));
    return mix(mix(A, B, F.x), mix(C, D, F.x), F.y);
}

float Fbm(vec2 P)
{
    return 0.55 * Noise(P) + 0.30 * Noise(P * 2.03 + 17.0) +
        0.15 * Noise(P * 4.11 + 41.0);
}

// NOTE(zoubir): how much of the open sky's light reaches a world point:
// 1 is full light. Patches 400 units across vary it by about 6%, and
// clouds 1000 units across drift by at 10 units a second, each taking up
// to 16% off where it is thickest
vec3 Sky(vec2 Pixel)
{
    vec2 Point = vec2(Pixel.x, WorldView.w - Pixel.y) / max(WorldView.z, 0.001) +
        WorldView.xy;
    float Patch = (Fbm(Point / 400.0) - 0.5) * 0.12;
    vec2 Drift = vec2(9.0, 4.0) * mod(Time, 4000.0);
    float Cloud = smoothstep(0.52, 0.78, Fbm((Point + Drift) / 1000.0 + 5.3));
    vec3 Shade = vec3(1.0 + Patch) - Cloud * vec3(0.16, 0.15, 0.12);
    return Shade;
}

void main()
{
    vec2 Pixel = gl_FragCoord.xy;
    vec3 C = World(Pixel);
    if (WorldView.z > 0.0)
    {
        C *= Sky(Pixel);
    }
    vec3 Light = Lights(Pixel);
    // NOTE(zoubir): bright pixels take less of it, so the lava and the
    // fireball themselves keep their detail instead of clipping to yellow
    float Lit = 1.1 * (1.0 - 0.85 * max(C.r, max(C.g, C.b)));
    C = C * (1.0 + Lit * Light) + 0.06 * Light;

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
