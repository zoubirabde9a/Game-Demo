// Boss beam: a beam of light along a quad, from its caster (U = 0) to
// where it stops (U = 1), V across it (client/dungeon/boss_fx/). Drawn
// additive. A live beam has a white-hot core, a coloured glow round it
// that wavers, light streaming along it, a flare where it leaves the
// caster and a splash where it hits. A warning is a thin line with dashes
// running along it, so the party sees where the beam will start. A
// pillar is a steady column of light standing up from the floor (U = 0),
// motes of light rising through it, fading toward its top.
//   colour r: strength
//   colour g: 0 a warning, a half a pillar, 1 a live beam
//   colour b: palette, 0 aurora green, a third aurora violet, two thirds
//             smite crimson, 1 star gold
//   colour a: overall alpha

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

vec3 BeamHue(float Pick)
{
    vec3 Result = vec3(0.35, 1.0, 0.68);
    if (Pick > 0.83)
    {
        Result = vec3(1.0, 0.78, 0.35);
    }
    else if (Pick > 0.5)
    {
        Result = vec3(1.0, 0.2, 0.45);
    }
    else if (Pick > 0.17)
    {
        Result = vec3(0.7, 0.45, 1.0);
    }
    return Result;
}

void main()
{
    float U = fragmentUV.x;
    float Across = fragmentUV.y * 2.0 - 1.0;
    float Strength = fragmentColor.r;
    vec3 Hue = BeamHue(fragmentColor.b);
    float Light;
    vec3 Colour;
    if (fragmentColor.g > 0.25 && fragmentColor.g < 0.75)
    {
        float Core = exp(-Across * Across * 16.0) * 0.55;
        float Glow = exp(-Across * Across * 2.5) * 0.35;
        float Motes = smoothstep(0.55, 0.9, Noise(vec2(U * 10.0 - Time * 1.8, Across * 4.0)));
        float Fade = pow(1.0 - U, 1.5) * smoothstep(0.0, 0.03, U);
        float Foot = exp(-U * 18.0) * exp(-Across * Across * 1.5) * 0.5;
        Light = ((Core + Glow) * (0.8 + 0.2 * sin(Time * 2.0 + U * 6.0)) + Motes * 0.35) * Fade + Foot;
        Light *= Strength;
        Colour = mix(Hue, vec3(1.0, 0.98, 0.9), clamp(Core * 1.2 + Motes * 0.5, 0.0, 1.0));
    }
    else if (fragmentColor.g < 0.5)
    {
        float Dash = step(0.45, fract(U * 18.0 - Time * 2.5));
        float Thin = exp(-Across * Across * 18.0);
        Light = Thin * (0.5 + 0.5 * Dash) * Strength;
        Colour = mix(Hue, vec3(1.0), 0.3 * Dash);
    }
    else
    {
        // the edges waver, the light streams away from the caster
        float Waver = 0.18 * (Noise(vec2(U * 9.0 - Time * 7.0, Time * 3.0)) - 0.5);
        float X = Across + Waver;
        float Core = exp(-X * X * 70.0);
        float Glow = exp(-X * X * 5.0) * 0.55;
        float Stream = Fbm(vec2(U * 14.0 - Time * 9.0, X * 2.0));
        Glow *= 0.6 + 0.8 * Stream;
        // the flare at the caster and the splash where it hits
        float Ends = exp(-U * 30.0) + exp(-(1.0 - U) * 22.0) * (0.8 + 0.4 * sin(Time * 30.0));
        float Fade = smoothstep(0.0, 0.02, U) * smoothstep(1.0, 0.985, U);
        Light = (Core + Glow) * Fade + Ends * exp(-X * X * 3.0) * 0.6;
        Light *= Strength;
        Colour = mix(Hue, vec3(1.0, 1.0, 0.96), clamp(Core * 0.9 + Ends * 0.5, 0.0, 1.0));
    }
    FragColor = vec4(Colour, clamp(Light, 0.0, 1.0) * fragmentColor.a);
}
