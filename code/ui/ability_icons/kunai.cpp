/* Kunai icon (V): a throwing blade on the diagonal, point up, with a
   wrapped red grip, a ring at the end and a pale streak behind it. */

internal void
PaintKunaiIcon(icon_canvas *Canvas)
{
    v4 SteelLight = IconColor(236, 242, 250);
    v4 SteelDark = IconColor(112, 124, 150);
    v2 Tip = V2(0.84f, 0.16f);
    v2 Base = V2(0.52f, 0.48f);
    v2 N = V2(0.075f, 0.075f);
    IconGlow(Canvas, V2(0.56f, 0.44f), 0.45f, IconColor(140, 210, 255, 70));
    // NOTE(zoubir): the streak it leaves, behind and below the grip
    IconCapsule(Canvas, V2(0.10f, 0.90f), V2(0.30f, 0.70f), 0.02f,
                Solid(IconColor(200, 235, 255, 120)));
    IconCapsule(Canvas, V2(0.16f, 0.94f), V2(0.30f, 0.80f), 0.012f,
                Solid(IconColor(200, 235, 255, 80)));
    // NOTE(zoubir): a leaf-shaped blade, its two halves lit differently
    v2 LitHalf[3] = {Base - N, Tip, Base};
    v2 DarkHalf[3] = {Base, Tip, Base + N};
    IconPolygon(Canvas, LitHalf, 3, Gradient(SteelLight, IconColor(196, 208, 226), Base, Tip));
    IconPolygon(Canvas, DarkHalf, 3, Gradient(SteelDark, IconColor(170, 182, 204), Base, Tip));
    IconCapsule(Canvas, V2(0.56f, 0.44f), V2(0.80f, 0.20f), 0.007f,
                Solid(IconColor(255, 255, 255, 210)));
    // NOTE(zoubir): the wrapped grip, then the ring
    IconCapsule(Canvas, V2(0.50f, 0.50f), V2(0.32f, 0.68f), 0.04f,
                Solid(IconColor(150, 40, 44)));
    for(u32 Band = 0; Band < 3; Band++)
    {
        float T = 0.25f + 0.25f * (float)Band;
        v2 P = V2(0.50f - 0.18f * T, 0.50f + 0.18f * T);
        IconCapsule(Canvas, P + V2(-0.03f, -0.03f), P + V2(0.03f, 0.03f), 0.012f,
                    Solid(IconColor(90, 20, 26)));
    }
    IconArc(Canvas, V2(0.26f, 0.74f), 0.07f, 0.03f,
            Gradient(SteelLight, SteelDark, V2(0.2f, 0.68f), V2(0.32f, 0.8f)));
}
