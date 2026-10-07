// Danger zone: where a monster's attack will land, painted on the ground
// under units while the attack winds up (client/danger_zones.cpp). The
// whole area is stained from the start, so a player sees at once how far
// to run; a brighter fill spreads over it as the windup runs, and the
// attack lands when the fill reaches the rim. The rim beats faster as the
// hit gets close.
//   A disc fills the quad's circle from the middle out. A lane is a
//   strip from the caster (U = 0) to its far end (U = 1), filled along
//   its length, with chevrons running the way the attack goes.
//   colour r: windup progress, 0 to 1
//   colour g: palette, 0 danger (red), 0.5 spirit (violet), 1 grave (green)
//   colour b: shape, below 0.5 a disc, above a lane
//   colour a: strength

VARYING vec4 fragmentColor;
VARYING vec2 fragmentUV;

uniform float Time;

vec3 Palette(float Pick)
{
    vec3 Result = vec3(1.0, 0.22, 0.12);
    if (Pick > 0.75)
    {
        Result = vec3(0.35, 1.0, 0.45);
    }
    else if (Pick > 0.25)
    {
        Result = vec3(0.72, 0.4, 1.0);
    }
    return Result;
}

void main()
{
    float Progress = fragmentColor.r;
    vec3 Hue = Palette(fragmentColor.g);
    bool Lane = fragmentColor.b > 0.5;

    // Edge is 0 in the middle and 1 on the rim; Along is how far the fill
    // has to travel to reach this point
    float Edge;
    float Along;
    float Stripes;
    if (Lane)
    {
        float Across = abs(fragmentUV.y * 2.0 - 1.0);
        Edge = max(Across, smoothstep(0.96, 1.0, fragmentUV.x));
        Along = fragmentUV.x;
        // chevrons pointing down the lane, running toward its end
        Stripes = fract(fragmentUV.x * 7.0 - Across * 0.6 - Time * 1.6);
    }
    else
    {
        vec2 P = fragmentUV * 2.0 - 1.0;
        Edge = length(P);
        Along = Edge;
        // rings drifting inward, the attack gathering
        Stripes = fract(Edge * 5.0 + Time * 1.2);
    }
    if (Edge > 1.0)
    {
        discard;
    }

    // the stain over the whole area, darker toward the rim
    float Stain = 0.3 + 0.12 * Edge + 0.08 * step(0.5, Stripes);
    // the fill, with a bright front where it is spreading
    float Filled = 1.0 - smoothstep(Progress - 0.02, Progress, Along);
    float Front = (1.0 - smoothstep(0.0, 0.06, abs(Along - Progress))) * step(0.02, Progress);
    float Fill = 0.3 * Filled + 0.6 * Front;
    // the rim beats, faster as the hit gets close
    float Beat = 0.75 + 0.25 * sin(Time * (5.0 + 22.0 * Progress));
    float Rim = (1.0 - smoothstep(0.03, 0.09, 1.0 - Edge)) * Beat;
    // the last moment, the whole area flares
    float Flare = smoothstep(0.85, 1.0, Progress) * 0.25;

    float Alpha = clamp(Stain + Fill + 0.85 * Rim + Flare, 0.0, 1.0);
    vec3 Colour = mix(Hue * 0.8, mix(Hue, vec3(1.0, 0.95, 0.8), 0.35), clamp(Fill + Rim + Flare, 0.0, 1.0));
    FragColor = vec4(Colour, Alpha * fragmentColor.a);
}
