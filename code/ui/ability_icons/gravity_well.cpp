/* Gravity Well icon (T): violet arms spiralling into a black core, with
   specks being drawn in. */

internal void
PaintGravityWellIcon(icon_canvas *Canvas)
{
    v4 Violet = IconColor(180, 100, 255);
    v4 Pink = IconColor(240, 170, 255);
    v2 Centre = V2(0.5f, 0.5f);
    IconGlow(Canvas, Centre, 0.48f, IconColor(140, 60, 255, 150));
    // NOTE(zoubir): each arm is arcs that tighten toward the core, so
    // together they read as a spiral
    for(u32 Arm = 0; Arm < 3; Arm++)
    {
        float Turn = (float)Arm * 2.f * Pi32 / 3.f;
        for(u32 Step = 0; Step < 4; Step++)
        {
            float Radius = 0.38f - 0.075f * (float)Step;
            float Start = Turn + 0.55f * (float)Step;
            float Width = 0.05f - 0.008f * (float)Step;
            IconArc(Canvas, Centre, Radius, Width,
                    Gradient(Pink, Violet, V2(0.2f, 0.2f), V2(0.8f, 0.8f)),
                    Start, Start + 1.1f);
        }
    }
    IconCircle(Canvas, Centre, 0.11f, Solid(IconColor(120, 70, 220)));
    IconCircle(Canvas, Centre, 0.075f, Solid(IconColor(10, 6, 24)));
    IconCircle(Canvas, V2(0.78f, 0.3f), 0.03f, Solid(Pink));
    IconCircle(Canvas, V2(0.2f, 0.66f), 0.025f, Solid(Pink));
    IconCircle(Canvas, V2(0.66f, 0.82f), 0.02f, Solid(Violet));
}
