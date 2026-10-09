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

// NOTE(zoubir): Prayer of Healing: three motes of green light rising over
// joined hands of light
internal void
PaintPrayerOfHealingIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(120, 230, 150, 150));
    IconArc(Canvas, V2(0.5f, 0.86f), 0.3f, 0.05f, Solid(IconColor(255, 240, 190)), 3.6f, 5.8f);
    for(u32 Mote = 0; Mote < 3; Mote++)
    {
        v2 C = V2(0.3f + 0.2f * (float)Mote, 0.38f - 0.08f * (float)(Mote % 2));
        IconCircle(Canvas, C, 0.07f, Gradient(IconColor(240, 255, 230), IconColor(90, 200, 120),
                                              C - V2(0.03f, 0.03f), C + V2(0.05f, 0.05f)));
        IconCapsule(Canvas, C, C + V2(0.f, 0.2f), 0.014f, Solid(IconColor(160, 240, 170, 140)));
    }
}

// NOTE(zoubir): Dawnbreak: a beam of gold light crossing the icon, a sun
// rising behind it
internal void
PaintDawnbreakIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.5f, IconColor(255, 210, 110, 160));
    IconCircle(Canvas, V2(0.5f, 0.78f), 0.24f, Gradient(IconColor(255, 245, 200), IconColor(255, 170, 60),
                                                       V2(0.5f, 0.6f), V2(0.5f, 0.95f)));
    IconCapsule(Canvas, V2(0.06f, 0.52f), V2(0.94f, 0.36f), 0.05f,
                Gradient(IconColor(255, 250, 220), IconColor(255, 200, 90, 120), V2(0.06f, 0.f), V2(0.94f, 0.f)));
    IconSparkle(Canvas, V2(0.82f, 0.2f), 0.08f, Solid(IconColor(255, 255, 235)));
}

// NOTE(zoubir): Purify: a white drop washing a dark stain away, a gold
// ring of ward round it
internal void
PaintPurifyIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(200, 240, 255, 150));
    IconArc(Canvas, V2(0.5f, 0.55f), 0.3f, 0.035f, Solid(IconColor(255, 215, 120)), 0.f, 2.f * Pi32);
    v2 Drop[3] = {V2(0.5f, 0.24f), V2(0.66f, 0.56f), V2(0.34f, 0.56f)};
    IconPolygon(Canvas, Drop, 3, Solid(IconColor(235, 250, 255)));
    IconCircle(Canvas, V2(0.5f, 0.6f), 0.16f, Gradient(IconColor(255, 255, 255), IconColor(150, 210, 240),
                                                      V2(0.46f, 0.52f), V2(0.56f, 0.72f)));
    IconCircle(Canvas, V2(0.74f, 0.8f), 0.05f, Solid(IconColor(90, 60, 80, 140)));
}
