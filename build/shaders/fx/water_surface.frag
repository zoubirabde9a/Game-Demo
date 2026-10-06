// Water surface (client/ground/water_surface.cpp): the moving light on
// water, drawn additive over the water tiles. Two layers of noise drift
// different ways; where a ridge of one meets a ridge of the other the
// light gathers into thin wavering lines, as sun through ripples does.
// Now and then a cell of the surface glints.
//   uv:        place on the map in tiles, so tiles join without a seam
//   colour a:  how much of this point is water (0 at the shore)
//   colour r:  how much of it is deep water: dimmer, slower light

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

float Hash(vec2 P)
{
    return fract(sin(dot(P, vec2(12.9898, 78.233))) * 43758.5453);
}

// NOTE(zoubir): value noise; cells wrap at 289 so the hash keeps its
// precision far out on an endless map
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

// NOTE(zoubir): 1 on the line where the noise crosses its middle, falling
// away to either side
float Ridge(vec2 P)
{
    float N = 0.65 * Noise(P) + 0.35 * Noise(P * 2.1 + 9.0);
    return 1.0 - abs(N * 2.0 - 1.0);
}

void main()
{
    float Deep = fragmentColor.r;
    float Water = fragmentColor.a;
    float Speed = mix(1.0, 0.55, Deep);
    float T = mod(Time, 1000.0) * Speed;
    vec2 P = fragmentUV * 2.2;

    float A = Ridge(P + vec2(0.21, 0.13) * T);
    float B = Ridge(P * 1.3 + vec2(-0.17, 0.19) * T + 31.0);
    float Lines = pow(A, 14.0) + pow(B, 14.0) + 1.2 * pow(A * B, 6.0);

    // NOTE(zoubir): glints: a few cells in a hundred flash for a fifth of
    // a second each
    vec2 GlintGrid = fragmentUV * 5.0;
    vec2 Cell = floor(GlintGrid);
    float Beat = floor(Time * 5.0 + Hash(mod(Cell, 289.0)) * 5.0);
    float On = step(0.975, Hash(mod(Cell + Beat * 0.37, 289.0)));
    vec2 Off = GlintGrid - Cell - 0.5;
    float Glint = On * pow(max(1.0 - length(Off) * 4.0, 0.0), 2.0);

    float Strength = mix(0.16, 0.12, Deep) * Lines + mix(0.7, 0.5, Deep) * Glint;
    vec3 Color = vec3(0.62, 0.86, 1.0);
    FragColor = vec4(Color, Water * Strength);
}
