/* Jump icon (Space): two chevrons rising off a ground line. */

internal void
PaintJumpIcon(icon_canvas *Canvas)
{
    v4 Sky = IconColor(150, 220, 255);
    v4 Deep = IconColor(60, 140, 230);
    IconGlow(Canvas, V2(0.5f, 0.4f), 0.42f, IconColor(110, 190, 255, 90));
    IconCapsule(Canvas, V2(0.18f, 0.86f), V2(0.82f, 0.86f), 0.028f,
                Solid(IconColor(170, 190, 210, 170)));
    icon_paint Lower = Gradient(IconColor(80, 160, 240, 150), IconColor(130, 200, 255, 190),
                                V2(0.5f, 0.76f), V2(0.5f, 0.52f));
    IconCapsule(Canvas, V2(0.28f, 0.74f), V2(0.5f, 0.54f), 0.055f, Lower);
    IconCapsule(Canvas, V2(0.72f, 0.74f), V2(0.5f, 0.54f), 0.055f, Lower);
    icon_paint Upper = Gradient(Deep, Sky, V2(0.5f, 0.54f), V2(0.5f, 0.22f));
    IconCapsule(Canvas, V2(0.24f, 0.52f), V2(0.5f, 0.26f), 0.07f, Upper);
    IconCapsule(Canvas, V2(0.76f, 0.52f), V2(0.5f, 0.26f), 0.07f, Upper);
}
