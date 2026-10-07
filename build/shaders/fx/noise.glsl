// Noise: a hash and smooth value noise over the plane, shared by the
// effects that draw patterns pinned to the map (world_grade.frag,
// ground_surface.frag). Not a shader on its own: the shader library puts
// it between the prelude and a shader whose ShaderDefs row names it.

// NOTE(zoubir): these hash world coordinates; at the browser's default
// medium precision (16 bits on phones) they turn to static, so ask for
// full precision where the device has it. Desktop GLSL skips this, and
// it covers the shader that follows too
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#endif

float Hash(vec2 P)
{
    return fract(sin(dot(P, vec2(12.9898, 78.233))) * 43758.5453);
}

// NOTE(zoubir): smooth value noise; the cells wrap at 289 so the hash
// keeps its precision far out on an endless map
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

// NOTE(zoubir): three octaves of it
float Fbm(vec2 P)
{
    return 0.55 * Noise(P) + 0.30 * Noise(P * 2.03 + 17.0) +
        0.15 * Noise(P * 4.11 + 41.0);
}
