/* Push icon (R): a wide cone of force rolling outward to the right. */

internal void
PaintPushIcon(icon_canvas *Canvas)
{
    v2 Origin = V2(0.20f, 0.5f);
    v4 Teal = IconColor(90, 230, 170);
    v4 Pale = IconColor(210, 255, 235);
    float Start = -0.85f;
    float End = 0.85f;
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.45f, IconColor(60, 220, 160, 90));
    IconArc(Canvas, Origin, 0.62f, 0.05f, Solid(IconColor(80, 210, 160, 140)), Start, End);
    IconArc(Canvas, Origin, 0.44f, 0.065f, Gradient(Teal, Pale, V2(0.5f, 0.8f), V2(0.6f, 0.2f)),
            Start, End);
    IconArc(Canvas, Origin, 0.26f, 0.08f, Gradient(Pale, Teal, V2(0.4f, 0.3f), V2(0.4f, 0.7f)),
            Start, End);
    IconCircle(Canvas, Origin, 0.07f, Solid(Pale));
}
