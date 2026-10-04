/* Launch icon (A): a burst on the ground throwing an arrow skyward. */

internal void
PaintLaunchIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 210, 70);
    v4 Amber = IconColor(220, 130, 30);
    v4 Pale = IconColor(255, 245, 200);
    IconGlow(Canvas, V2(0.5f, 0.78f), 0.32f, IconColor(255, 180, 60, 150));
    IconCapsule(Canvas, V2(0.16f, 0.86f), V2(0.84f, 0.86f), 0.03f,
                Solid(IconColor(200, 160, 100, 180)));
    // NOTE(zoubir): debris kicked up either side of the arrow
    IconCapsule(Canvas, V2(0.30f, 0.80f), V2(0.20f, 0.64f), 0.022f, Solid(Amber));
    IconCapsule(Canvas, V2(0.70f, 0.80f), V2(0.80f, 0.64f), 0.022f, Solid(Amber));
    IconCapsule(Canvas, V2(0.50f, 0.82f), V2(0.50f, 0.40f), 0.075f,
                Gradient(Amber, Gold, V2(0.5f, 0.82f), V2(0.5f, 0.4f)));
    v2 Head[3] = {V2(0.24f, 0.44f), V2(0.5f, 0.12f), V2(0.76f, 0.44f)};
    IconPolygon(Canvas, Head, 3, Gradient(Gold, Pale, V2(0.5f, 0.44f), V2(0.5f, 0.14f)));
}
