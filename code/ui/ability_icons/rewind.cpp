/* Time rewind icons (T, G, V; sim/time_rewind/): a clock with an arrow
   running back round it for the caster's own rewind, the same clock
   inside a bubble for the bubble, and a globe under the tape's rewind
   mark for the whole world. */

// NOTE(zoubir): a clock face: rim, four ticks and two hands at ten to two
internal void
PaintRewindClock(icon_canvas *Canvas, v2 Centre, float Radius, v4 Rim, v4 Hands)
{
    IconCircle(Canvas, Centre, Radius, Solid(IconColor(20, 40, 70, 150)));
    IconArc(Canvas, Centre, Radius, 0.045f, Solid(Rim));
    for(u32 Tick = 0; Tick < 4; Tick++)
    {
        float Angle = 0.5f * Pi32 * (float)Tick;
        v2 Dir = V2(Cos(Angle), Sin(Angle));
        IconCapsule(Canvas, Centre + 0.70f * Radius * Dir, Centre + 0.86f * Radius * Dir,
                    0.016f, Solid(Rim));
    }
    float Minute = -0.5f * Pi32 - 0.35f * Pi32;
    float Hour = -0.5f * Pi32 + 0.33f * Pi32;
    IconCapsule(Canvas, Centre, Centre + 0.72f * Radius * V2(Cos(Minute), Sin(Minute)),
                0.022f, Solid(Hands));
    IconCapsule(Canvas, Centre, Centre + 0.48f * Radius * V2(Cos(Hour), Sin(Hour)),
                0.03f, Solid(Hands));
    IconCircle(Canvas, Centre, 0.04f, Solid(Hands));
}

// NOTE(zoubir): an arc running anticlockwise from Start to End with an
// arrowhead at End
internal void
PaintRewindArrow(icon_canvas *Canvas, v2 Centre, float Radius, float Start, float End,
                 v4 Color)
{
    IconArc(Canvas, Centre, Radius, 0.05f, Solid(Color), Start, End);
    v2 Tip = Centre + Radius * V2(Cos(Start), Sin(Start));
    v2 Along = V2(Sin(Start), -Cos(Start));
    v2 Out = V2(Cos(Start), Sin(Start));
    IconTriangle(Canvas, Tip + 0.11f * Along, Tip - 0.06f * Along + 0.09f * Out,
                 Tip - 0.06f * Along - 0.09f * Out, Solid(Color));
}

internal void
PaintRewindSelfIcon(icon_canvas *Canvas)
{
    v4 Cyan = IconColor(90, 220, 255);
    v4 Pale = IconColor(225, 250, 255);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.42f, IconColor(60, 190, 255, 110));
    PaintRewindClock(Canvas, V2(0.5f, 0.52f), 0.26f, Cyan, Pale);
    PaintRewindArrow(Canvas, V2(0.5f, 0.52f), 0.37f, 0.15f * Pi32, 1.75f * Pi32, Cyan);
}

internal void
PaintRewindBubbleIcon(icon_canvas *Canvas)
{
    v4 Teal = IconColor(110, 240, 230);
    v4 Pale = IconColor(220, 255, 250);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(60, 220, 210, 90));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.40f, Solid(IconColor(80, 200, 220, 50)));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.40f, 0.035f, Solid(Teal));
    // NOTE(zoubir): the bubble's shine
    IconArc(Canvas, V2(0.5f, 0.5f), 0.31f, 0.03f, Solid(IconColor(240, 255, 255, 170)),
            1.1f * Pi32, 1.4f * Pi32);
    PaintRewindClock(Canvas, V2(0.5f, 0.54f), 0.2f, Teal, Pale);
    IconSparkle(Canvas, V2(0.78f, 0.24f), 0.09f, Solid(Pale));
    IconSparkle(Canvas, V2(0.22f, 0.76f), 0.06f, Solid(Pale));
}

internal void
PaintRewindWorldIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 200, 90);
    v4 Magenta = IconColor(250, 110, 230);
    v4 Pale = IconColor(255, 235, 255);
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.46f, IconColor(230, 90, 220, 110));
    // NOTE(zoubir): the globe: a disc with an equator and two meridians
    v2 Centre = V2(0.5f, 0.6f);
    IconCircle(Canvas, Centre, 0.3f, Gradient(IconColor(90, 60, 160, 230),
                                              IconColor(40, 20, 90, 230),
                                              V2(0.3f, 0.35f), V2(0.7f, 0.85f)));
    IconArc(Canvas, Centre, 0.3f, 0.03f, Solid(Magenta));
    IconCapsule(Canvas, Centre - V2(0.29f, 0.f), Centre + V2(0.29f, 0.f), 0.012f,
                Solid(IconColor(250, 150, 240, 200)));
    IconArc(Canvas, Centre, 0.15f, 0.02f, Solid(IconColor(250, 150, 240, 200)),
            0.5f * Pi32, 1.5f * Pi32);
    IconArc(Canvas, Centre, 0.15f, 0.02f, Solid(IconColor(250, 150, 240, 200)),
            -0.5f * Pi32, 0.5f * Pi32);
    // NOTE(zoubir): the tape's rewind mark over it
    for(u32 Arrow = 0; Arrow < 2; Arrow++)
    {
        float X = 0.24f + 0.22f * (float)Arrow;
        IconTriangle(Canvas, V2(X, 0.24f), V2(X + 0.22f, 0.10f), V2(X + 0.22f, 0.38f),
                     Gradient(Pale, Gold, V2(X, 0.24f), V2(X + 0.22f, 0.24f)));
    }
}
