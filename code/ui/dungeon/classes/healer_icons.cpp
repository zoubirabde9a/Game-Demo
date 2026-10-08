/* Mender (healer) talent icons (ui/dungeon/role_icons.cpp) for the tree's
   later slots, painted on the icon canvas (ui/ability_icons/
   icon_canvas.cpp); ui/dungeon/role_talent_icons.cpp lists them in the
   Mender's row. Steadfast Ward shows the Ward spell with a badge, Guardian
   Angel its own picture. */

// NOTE(zoubir): Steadfast Ward: the Ward spell with a badge holding an
// arrow up over a clock hand, "more, and sooner"
internal void
PaintSteadfastWardIcon(icon_canvas *Canvas)
{
    PaintRoleWardIcon(Canvas);
    v4 Ink = IconColor(16, 24, 40);
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.17f, Solid(IconColor(16, 18, 26)));
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.14f, Solid(IconColor(150, 220, 255)));
    IconTriangle(Canvas, V2(0.78f, 0.66f), V2(0.85f, 0.76f), V2(0.71f, 0.76f), Solid(Ink));
    IconCapsule(Canvas, V2(0.78f, 0.76f), V2(0.78f, 0.88f), 0.022f, Solid(Ink));
    IconCapsule(Canvas, V2(0.78f, 0.84f), V2(0.85f, 0.87f), 0.02f, Solid(Ink));
}

// NOTE(zoubir): Guardian Angel: a haloed figure in gold light, two wings
// spread round it
internal void
PaintGuardianAngelIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(255, 225, 130, 150));
    for(u32 Feather = 0; Feather < 4; Feather++)
    {
        float Drop = 0.1f * (float)Feather;
        float Reach = 0.42f - 0.05f * (float)Feather;
        v4 Tip = IconColor(255, 250, 235);
        v4 Root = IconColor(230, 210, 160);
        IconCapsule(Canvas, V2(0.46f, 0.5f), V2(0.5f - Reach, 0.28f + Drop), 0.05f,
                    Gradient(Root, Tip, V2(0.46f, 0.5f), V2(0.5f - Reach, 0.28f + Drop)));
        IconCapsule(Canvas, V2(0.54f, 0.5f), V2(0.5f + Reach, 0.28f + Drop), 0.05f,
                    Gradient(Root, Tip, V2(0.54f, 0.5f), V2(0.5f + Reach, 0.28f + Drop)));
    }
    IconCircle(Canvas, V2(0.5f, 0.36f), 0.075f, Solid(IconColor(255, 250, 235)));
    v2 Robe[4] = {V2(0.44f, 0.46f), V2(0.56f, 0.46f), V2(0.64f, 0.88f), V2(0.36f, 0.88f)};
    IconPolygon(Canvas, Robe, 4, Gradient(IconColor(255, 250, 235), IconColor(240, 200, 110),
                                          V2(0.5f, 0.46f), V2(0.5f, 0.88f)));
    IconArc(Canvas, V2(0.5f, 0.22f), 0.09f, 0.025f, Solid(IconColor(255, 210, 90)));
}
