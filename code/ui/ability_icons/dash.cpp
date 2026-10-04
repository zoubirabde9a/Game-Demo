/* Dash icon (Alt): an arrow head with speed streaks trailing behind it. */

internal void
PaintDashIcon(icon_canvas *Canvas)
{
    v4 Cyan = IconColor(110, 240, 250);
    v4 Clear = IconColor(110, 240, 250, 0);
    IconGlow(Canvas, V2(0.66f, 0.5f), 0.36f, IconColor(80, 220, 240, 100));
    IconCapsule(Canvas, V2(0.10f, 0.34f), V2(0.56f, 0.34f), 0.035f,
                Gradient(Clear, Cyan, V2(0.1f, 0.f), V2(0.56f, 0.f)));
    IconCapsule(Canvas, V2(0.04f, 0.50f), V2(0.60f, 0.50f), 0.045f,
                Gradient(Clear, Cyan, V2(0.04f, 0.f), V2(0.6f, 0.f)));
    IconCapsule(Canvas, V2(0.16f, 0.66f), V2(0.56f, 0.66f), 0.035f,
                Gradient(Clear, Cyan, V2(0.16f, 0.f), V2(0.56f, 0.f)));
    v2 Head[3] = {V2(0.52f, 0.22f), V2(0.90f, 0.50f), V2(0.52f, 0.78f)};
    IconPolygon(Canvas, Head, 3, Gradient(IconColor(40, 170, 210), IconColor(220, 255, 255),
                                         V2(0.52f, 0.7f), V2(0.8f, 0.4f)));
}
