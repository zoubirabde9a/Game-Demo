/* Blink icon (F): a faded shape where the player was, a dotted jump and a
   sparkle at the landing spot. */

internal void
PaintBlinkIcon(icon_canvas *Canvas)
{
    v4 Magenta = IconColor(240, 110, 230);
    v4 Pale = IconColor(255, 220, 250);
    IconGlow(Canvas, V2(0.72f, 0.30f), 0.34f, IconColor(230, 90, 220, 130));
    IconCircle(Canvas, V2(0.24f, 0.74f), 0.12f, Solid(IconColor(200, 120, 220, 70)));
    IconArc(Canvas, V2(0.24f, 0.74f), 0.12f, 0.025f, Solid(IconColor(220, 150, 235, 140)));
    // NOTE(zoubir): dots along the jump, growing toward the landing
    for(u32 Index = 0; Index < 4; Index++)
    {
        float t = (float)(Index + 1) / 5.f;
        v2 P = V2(Lerp(0.30f, t, 0.70f), Lerp(0.64f, t, 0.32f) - 0.12f * Sin(t * Pi32));
        IconCircle(Canvas, P, 0.02f + 0.012f * (float)Index,
                   Solid(IconColor(240, 140, 235, 120 + 30 * Index)));
    }
    IconSparkle(Canvas, V2(0.72f, 0.30f), 0.22f, Gradient(Pale, Magenta, V2(0.6f, 0.2f),
                                                          V2(0.84f, 0.42f)));
    IconCircle(Canvas, V2(0.72f, 0.30f), 0.05f, Solid(IconColor(255, 255, 255)));
}
