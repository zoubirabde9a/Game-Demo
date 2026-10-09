/* Bulwark icons (ui/dungeon/role_icons.cpp, through class_icons.cpp): the
   pictures for the Bulwark's spells on G and T, painted in code on the
   icon canvas. Its other spells' icons are role_icons.cpp's. */

// NOTE(zoubir): Rallying Cry: a raised banner of the Bulwark's blue, a
// gold ring of light round its foot
internal void
PaintRallyingCryIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.5f, IconColor(110, 170, 255, 150));
    IconArc(Canvas, V2(0.5f, 0.8f), 0.3f, 0.035f, Solid(IconColor(255, 215, 120)), 0.f, 2.f * Pi32);
    IconCapsule(Canvas, V2(0.4f, 0.86f), V2(0.4f, 0.12f), 0.025f, Solid(IconColor(200, 170, 120)));
    v2 Flag[4] = {V2(0.42f, 0.14f), V2(0.82f, 0.2f), V2(0.74f, 0.36f), V2(0.42f, 0.44f)};
    IconPolygon(Canvas, Flag, 4, Gradient(IconColor(140, 190, 255), IconColor(50, 90, 200),
                                          V2(0.42f, 0.14f), V2(0.82f, 0.4f)));
    IconSparkle(Canvas, V2(0.72f, 0.64f), 0.08f, Solid(IconColor(255, 240, 200)));
}

// NOTE(zoubir): Demoralizing Roar: a shield with red rings going out from
// it, and a foe's blade bent down
internal void
PaintDemoralizingRoarIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.4f, 0.55f), 0.5f, IconColor(255, 90, 70, 120));
    for(u32 Ring = 0; Ring < 3; Ring++)
    {
        IconArc(Canvas, V2(0.36f, 0.55f), 0.24f + 0.14f * (float)Ring, 0.035f,
                Solid(IconColor(255, 110 + 30 * Ring, 90, 230 - 60 * Ring)), -0.8f, 0.8f);
    }
    v2 Shield[5] = {V2(0.18f, 0.36f), V2(0.44f, 0.36f), V2(0.44f, 0.62f), V2(0.31f, 0.78f), V2(0.18f, 0.62f)};
    IconPolygon(Canvas, Shield, 5, Gradient(IconColor(140, 190, 255), IconColor(50, 90, 200),
                                            V2(0.2f, 0.36f), V2(0.44f, 0.76f)));
    IconCapsule(Canvas, V2(0.78f, 0.22f), V2(0.92f, 0.62f), 0.022f, Solid(IconColor(120, 120, 135)));
}
