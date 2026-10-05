/* Frost Nova icon (G): a six-armed snowflake over a burst of cold light,
   with a frosted ring round it. */

internal void
PaintFrostNovaIcon(icon_canvas *Canvas)
{
    v4 Ice = IconColor(150, 225, 255);
    v4 Pale = IconColor(235, 250, 255);
    v4 Deep = IconColor(60, 140, 230);
    v2 Centre = V2(0.5f, 0.5f);
    IconGlow(Canvas, Centre, 0.48f, IconColor(90, 190, 255, 140));
    IconArc(Canvas, Centre, 0.40f, 0.035f,
            Gradient(Pale, IconColor(90, 170, 240, 120), V2(0.5f, 0.1f), V2(0.5f, 0.9f)));
    for(u32 Arm = 0; Arm < 6; Arm++)
    {
        float Angle = Pi32 / 6.f + (float)Arm * Pi32 / 3.f;
        v2 Dir = V2(Cos(Angle), Sin(Angle));
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 Tip = Centre + 0.33f * Dir;
        IconCapsule(Canvas, Centre, Tip, 0.03f,
                    Gradient(Pale, Ice, Centre, Tip));
        // NOTE(zoubir): two small branches on each arm, angled out
        v2 Fork = Centre + 0.2f * Dir;
        IconCapsule(Canvas, Fork, Fork + 0.09f * Dir + 0.08f * Side, 0.018f, Solid(Ice));
        IconCapsule(Canvas, Fork, Fork + 0.09f * Dir - 0.08f * Side, 0.018f, Solid(Ice));
    }
    IconCircle(Canvas, Centre, 0.085f, Gradient(Pale, Deep, V2(0.45f, 0.42f), V2(0.56f, 0.6f)));
    IconSparkle(Canvas, V2(0.72f, 0.26f), 0.07f, Solid(Pale));
}
