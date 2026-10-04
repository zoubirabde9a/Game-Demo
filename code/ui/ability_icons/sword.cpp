/* Sword icon (right click): a blade on the diagonal, gold guard and pommel. */

internal void
PaintSwordIcon(icon_canvas *Canvas)
{
    v4 SteelLight = IconColor(236, 242, 250);
    v4 SteelDark = IconColor(120, 132, 156);
    v4 Gold = IconColor(240, 190, 70);
    v4 GoldDark = IconColor(160, 104, 30);
    // NOTE(zoubir): the blade runs from the guard to the tip, its two
    // halves lit differently so it reads as an edged blade
    v2 Guard = V2(0.32f, 0.68f);
    v2 Near = V2(0.76f, 0.24f);
    v2 Tip = V2(0.86f, 0.14f);
    v2 N = V2(0.0495f, 0.0495f);
    v2 LitHalf[4] = {Guard - N, Near - N, Tip, Guard};
    v2 DarkHalf[4] = {Guard, Tip, Near + N, Guard + N};
    IconGlow(Canvas, V2(0.56f, 0.44f), 0.45f, IconColor(170, 200, 255, 70));
    IconPolygon(Canvas, LitHalf, 4, Gradient(SteelLight, IconColor(200, 210, 228),
                                            Guard, Tip));
    IconPolygon(Canvas, DarkHalf, 4, Gradient(SteelDark, IconColor(170, 180, 200),
                                             Guard, Tip));
    IconCapsule(Canvas, V2(0.36f, 0.64f), V2(0.74f, 0.26f), 0.008f,
                Solid(IconColor(255, 255, 255, 200)));
    // NOTE(zoubir): guard across the blade, then grip and pommel
    IconCapsule(Canvas, V2(0.20f, 0.56f), V2(0.44f, 0.80f), 0.05f,
                Gradient(Gold, GoldDark, V2(0.2f, 0.56f), V2(0.44f, 0.8f)));
    IconCapsule(Canvas, V2(0.30f, 0.70f), V2(0.17f, 0.83f), 0.036f,
                Solid(IconColor(110, 64, 40)));
    IconCircle(Canvas, V2(0.15f, 0.85f), 0.055f,
               Gradient(Gold, GoldDark, V2(0.1f, 0.8f), V2(0.2f, 0.9f)));
}
