/* Shockwave icon (E): a bright core with two rings breaking outward. */

internal void
PaintShockwaveIcon(icon_canvas *Canvas)
{
    v2 Centre = V2(0.5f, 0.5f);
    v4 Violet = IconColor(170, 130, 255);
    v4 Pale = IconColor(230, 215, 255);
    IconGlow(Canvas, Centre, 0.5f, IconColor(140, 100, 255, 120));
    IconArc(Canvas, Centre, 0.40f, 0.045f, Solid(IconColor(150, 110, 250, 170)));
    IconArc(Canvas, Centre, 0.27f, 0.06f, Gradient(Pale, Violet, V2(0.3f, 0.3f), V2(0.7f, 0.7f)));
    IconCircle(Canvas, Centre, 0.13f, Gradient(IconColor(255, 255, 255), Violet,
                                              V2(0.44f, 0.44f), V2(0.58f, 0.6f)));
    // NOTE(zoubir): short spikes between the rings, every 45 degrees
    for(u32 Index = 0; Index < 8; Index++)
    {
        float Angle = (float)Index * (Pi32 / 4.f) + Pi32 / 8.f;
        v2 Dir = V2(Cos(Angle), Sin(Angle));
        IconCapsule(Canvas, Centre + 0.31f * Dir, Centre + 0.36f * Dir, 0.014f, Solid(Pale));
    }
}
