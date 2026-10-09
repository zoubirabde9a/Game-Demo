// Boss shock: the shockwave of a boss's blow landing
// (client/dungeon/boss_fx/boss_impacts.cpp). Drawn additive on the
// ground, once a frame, its quad the size of the blow. A flash fills the
// middle at once and fades, a ragged front rolls out to the rim and thins
// as it goes, and streaks of debris fly out along it.
//   colour rgb: the blow's colour
//   colour a:   age, 0 when it lands to 1 when it is gone

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

void main()
{
    vec2 P = fragmentUV * 2.0 - 1.0;
    float R = length(P);
    if (R > 1.0)
    {
        discard;
    }
    float A = atan(P.y, P.x);
    float Age = fragmentColor.a;
    float Left = 1.0 - Age;
    // the front races out and slows, the way a blast does
    float Front = 1.0 - Left * Left * Left;
    float Rag = 0.06 * (Noise(vec2(cos(A), sin(A)) * 5.0 + vec2(Age * 4.0, 0.0)) - 0.5);
    float Width = 0.04 + 0.16 * Left;
    float Ring = 1.0 - smoothstep(0.0, Width, abs(R - Front * 0.95 + Rag));
    // behind the front, the air still glows a little
    float Wake = step(R, Front) * smoothstep(0.0, Front + 0.001, R) * 0.15 * Left;
    // the flash in the middle, gone in the first fifth
    float Flash = (1.0 - smoothstep(0.0, 0.2, Age)) * (1.0 - smoothstep(0.0, 0.7, R));
    // streaks of debris thrown out along the front
    float Streak = step(0.72, Noise(vec2(cos(A), sin(A)) * 18.0 + 7.0)) *
        (1.0 - smoothstep(0.0, 0.18, abs(R - Front * 0.8))) * Left;
    float Light = (Ring * (0.6 + 0.4 * Left) + Wake + Flash + Streak * 0.7) * Left;
    vec3 Colour = mix(fragmentColor.rgb, vec3(1.0, 0.97, 0.9), clamp(Flash + 0.5 * Ring * Left, 0.0, 1.0));
    FragColor = vec4(Colour, clamp(Light, 0.0, 1.0));
}
