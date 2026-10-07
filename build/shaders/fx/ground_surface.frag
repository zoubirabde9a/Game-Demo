// Ground surface (client/ground/ground_surface.cpp): light moving on the
// ground, drawn additive over the tiles of one surface.
//   water: two layers of noise drift different ways; where a ridge of one
//          meets a ridge of the other the light gathers into thin wavering
//          lines, as sun through ripples does. Now and then a cell glints,
//          and foam laps along the shore
//   ice:   a broad sheen sliding slowly across, and rarer, sharper glints
//   snow:  blended, not added: cold grey drifts combed by the wind, and
//          a fine glitter of white points twinkling in turn over them
//   uv:        place on the map in tiles, so tiles join without a seam
//   colour a:  how much of this point has the surface (0 at its edge)
//   colour g:  which surface, 1 water, 2 ice, 3 snow (out of 255)
//   colour r:  how much of the water is deep: dimmer, slower light

// Hash and Noise come from fx/noise.glsl (the shader library puts it
// first).

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

// NOTE(zoubir): 1 on the line where the noise crosses its middle, falling
// away to either side
float Ridge(vec2 P)
{
    float N = 0.65 * Noise(P) + 0.35 * Noise(P * 2.1 + 9.0);
    return 1.0 - abs(N * 2.0 - 1.0);
}

// NOTE(zoubir): Count cells per tile, a few in Rarity of them flash at a
// time, each for a 1 / Rate of a second; a soft point of light Size of
// its cell
float Glints(vec2 UV, float Count, float Rarity, float Rate, float Size)
{
    vec2 Grid = UV * Count;
    vec2 Cell = floor(Grid);
    float Beat = floor(Time * Rate + Hash(mod(Cell, 289.0)) * Rate);
    float On = step(1.0 - Rarity, Hash(mod(Cell + Beat * 0.37, 289.0)));
    vec2 Off = Grid - Cell - 0.5;
    return On * pow(max(1.0 - length(Off) / Size, 0.0), 2.0);
}

void main()
{
    float Surface = floor(fragmentColor.g * 255.0 + 0.5);
    float Here = fragmentColor.a;
    float T = mod(Time, 1000.0);
    vec3 Color = vec3(0.62, 0.86, 1.0);
    float Strength = 0.0;
    if (Surface < 1.5)
    {
        float Deep = fragmentColor.r;
        float Flow = T * mix(1.0, 0.55, Deep);
        vec2 P = fragmentUV * 2.2;
        float A = Ridge(P + vec2(0.21, 0.13) * Flow);
        float B = Ridge(P * 1.3 + vec2(-0.17, 0.19) * Flow + 31.0);
        float Lines = pow(A, 14.0) + pow(B, 14.0) + 1.2 * pow(A * B, 6.0);
        float Glint = Glints(fragmentUV, 5.0, 0.025, 5.0, 0.25);
        Strength = mix(0.10, 0.08, Deep) * Lines + mix(0.6, 0.4, Deep) * Glint;
        // NOTE(zoubir): foam along the shore. Here falls from 1 to 0 across
        // the last tile before land, so the band where it is about half is
        // the water's edge; broken up by noise, lapping in and out
        float Shore = smoothstep(0.15, 0.35, Here) * (1.0 - smoothstep(0.45, 0.70, Here));
        float Lap = 0.5 + 0.5 * sin(T * 1.6 - (fragmentUV.x + fragmentUV.y) * 2.0);
        float Froth = smoothstep(0.54, 0.68, Noise(fragmentUV * 11.0 + vec2(0.3, -0.2) * T) +
                                 0.18 * Lap);
        Strength += 0.55 * Shore * Froth;
        Color = mix(Color, vec3(0.95, 1.0, 1.0), Shore * Froth);
    }
    else if (Surface < 2.5)
    {
        // NOTE(zoubir): a band along the diagonal, three tiles apart,
        // sliding a tile every four seconds, wavering with the noise
        float Along = (fragmentUV.x + fragmentUV.y) / 3.0 - T * 0.08 +
            0.35 * Noise(fragmentUV * 0.7);
        float Sheen = pow(0.5 + 0.5 * sin(Along * 6.2832), 10.0);
        float Glint = Glints(fragmentUV, 4.0, 0.04, 2.0, 0.26);
        Color = vec3(0.80, 0.92, 1.0);
        Strength = 0.13 * Sheen + 0.8 * Glint;
    }
    else
    {
        // NOTE(zoubir): drawn blended: the drifts shade the snow toward a
        // cold grey, stretched along the wind; the glints are white on top
        vec2 Wind = vec2(fragmentUV.x * 0.45 + fragmentUV.y * 0.2, fragmentUV.y * 1.1);
        float Drift = smoothstep(0.35, 0.75, 0.6 * Noise(Wind) + 0.4 * Noise(Wind * 2.3 + 7.0));
        float Glint = Glints(fragmentUV, 6.0, 0.05, 3.0, 0.32);
        float Shade = 0.38 * Drift * (1.0 - Glint);
        Color = mix(vec3(0.62, 0.70, 0.84), vec3(1.0), Glint / max(Shade + Glint, 0.001));
        Strength = Shade + Glint;
    }
    FragColor = vec4(Color, Here * Strength);
}
