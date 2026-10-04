/* Slam icon (C): an arrow driving down into the ground and cracking it. */

internal void
PaintSlamIcon(icon_canvas *Canvas)
{
    v4 Red = IconColor(240, 70, 50);
    v4 Ember = IconColor(255, 170, 80);
    v4 Pale = IconColor(255, 230, 200);
    IconGlow(Canvas, V2(0.5f, 0.82f), 0.36f, IconColor(255, 90, 40, 160));
    IconCapsule(Canvas, V2(0.12f, 0.84f), V2(0.88f, 0.84f), 0.032f,
                Solid(IconColor(150, 110, 100, 200)));
    // NOTE(zoubir): cracks spreading from the point of impact
    IconCapsule(Canvas, V2(0.5f, 0.84f), V2(0.30f, 0.95f), 0.016f, Solid(Ember));
    IconCapsule(Canvas, V2(0.5f, 0.84f), V2(0.70f, 0.95f), 0.016f, Solid(Ember));
    IconCapsule(Canvas, V2(0.5f, 0.84f), V2(0.16f, 0.78f), 0.012f, Solid(Ember));
    IconCapsule(Canvas, V2(0.5f, 0.84f), V2(0.84f, 0.78f), 0.012f, Solid(Ember));
    IconCapsule(Canvas, V2(0.5f, 0.10f), V2(0.5f, 0.50f), 0.075f,
                Gradient(Pale, Red, V2(0.5f, 0.1f), V2(0.5f, 0.5f)));
    v2 Head[3] = {V2(0.24f, 0.46f), V2(0.76f, 0.46f), V2(0.5f, 0.78f)};
    IconPolygon(Canvas, Head, 3, Gradient(Ember, Red, V2(0.5f, 0.46f), V2(0.5f, 0.78f)));
}
