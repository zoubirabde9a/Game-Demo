/* Fireball icon (X): a burning orb with a tail of flame behind it. */

internal void
PaintFireballIcon(icon_canvas *Canvas)
{
    v4 Deep = IconColor(200, 40, 20);
    v4 Orange = IconColor(255, 128, 32);
    v4 Yellow = IconColor(255, 220, 90);
    v4 White = IconColor(255, 250, 225);
    IconGlow(Canvas, V2(0.58f, 0.42f), 0.46f, IconColor(255, 110, 30, 150));
    // NOTE(zoubir): the tail, widest at the orb and burning out behind it
    v2 Tail[4] = {V2(0.10f, 0.92f), V2(0.40f, 0.30f), V2(0.62f, 0.36f), V2(0.68f, 0.62f)};
    IconPolygon(Canvas, Tail, 4, Gradient(IconColor(200, 40, 20, 0), Orange,
                                         V2(0.12f, 0.88f), V2(0.55f, 0.48f)));
    v2 InnerTail[3] = {V2(0.24f, 0.80f), V2(0.48f, 0.40f), V2(0.62f, 0.56f)};
    IconPolygon(Canvas, InnerTail, 3, Gradient(IconColor(255, 200, 60, 0), Yellow,
                                              V2(0.26f, 0.78f), V2(0.56f, 0.48f)));
    IconCircle(Canvas, V2(0.58f, 0.42f), 0.25f,
               Gradient(Yellow, Deep, V2(0.48f, 0.30f), V2(0.72f, 0.64f)));
    IconCircle(Canvas, V2(0.56f, 0.40f), 0.15f,
               Gradient(White, Orange, V2(0.50f, 0.32f), V2(0.66f, 0.52f)));
    IconCircle(Canvas, V2(0.52f, 0.35f), 0.05f, Solid(White));
    IconCircle(Canvas, V2(0.20f, 0.56f), 0.03f, Solid(Yellow));
    IconCircle(Canvas, V2(0.34f, 0.84f), 0.025f, Solid(Orange));
}
