/* Shield icon (E): a kite shield with a bright rim and a glint. */

internal void
PaintShieldIcon(icon_canvas *Canvas)
{
    v4 Rim = IconColor(200, 235, 255);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.42f, IconColor(90, 170, 255, 110));
    v2 Outer[5] = {V2(0.20f, 0.18f), V2(0.80f, 0.18f), V2(0.80f, 0.48f),
                   V2(0.50f, 0.88f), V2(0.20f, 0.48f)};
    IconPolygon(Canvas, Outer, 5, Gradient(Rim, IconColor(120, 180, 240),
                                           V2(0.5f, 0.18f), V2(0.5f, 0.88f)));
    v2 Inner[5] = {V2(0.27f, 0.25f), V2(0.73f, 0.25f), V2(0.73f, 0.46f),
                   V2(0.50f, 0.78f), V2(0.27f, 0.46f)};
    IconPolygon(Canvas, Inner, 5, Gradient(IconColor(60, 130, 230), IconColor(25, 60, 150),
                                           V2(0.3f, 0.25f), V2(0.7f, 0.75f)));
    IconCapsule(Canvas, V2(0.50f, 0.30f), V2(0.50f, 0.66f), 0.035f,
                Gradient(Rim, Rim, V2(0.f, 0.f), V2(1.f, 1.f)));
    IconSparkle(Canvas, V2(0.36f, 0.34f), 0.07f,
                Gradient(IconColor(255, 255, 255), IconColor(255, 255, 255),
                         V2(0.f, 0.f), V2(1.f, 1.f)));
}
