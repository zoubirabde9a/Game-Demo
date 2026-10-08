/* Shadowblade icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Shadowblade's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty. Violet shadow and steel, with
   acid green for poison and the critical strike. */

inline v4 ShadowbladeIconViolet(u32 A = 255) { return IconColor(170, 110, 255, A); }
inline v4 ShadowbladeIconDeep(u32 A = 255) { return IconColor(52, 22, 80, A); }
inline v4 ShadowbladeIconAcid(u32 A = 255) { return IconColor(150, 255, 70, A); }

// NOTE(zoubir): a dagger from its grip at Grip along Dir (a unit vector),
// Length long: steel blade lit on one edge, violet guard, dark grip and an
// acid-green pommel stone
internal void
IconShadowbladeDagger(icon_canvas *Canvas, v2 Grip, v2 Dir, float Length)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    float W = 0.13f * Length;
    v2 Base = Grip + (0.22f * Length) * Dir;
    v2 Tip = Grip + Length * Dir;
    v2 Outline[3] = {Base - (W + 0.015f) * Side, Tip + 0.02f * Dir, Base + (W + 0.015f) * Side};
    IconPolygon(Canvas, Outline, 3, Solid(IconColor(16, 10, 20)));
    IconTriangle(Canvas, Base - W * Side, Tip, Base,
                 Gradient(IconColor(255, 255, 255), IconColor(200, 205, 225), Base, Tip));
    IconTriangle(Canvas, Base, Tip, Base + W * Side,
                 Gradient(IconColor(150, 140, 170), IconColor(100, 90, 120), Base, Tip));
    IconCapsule(Canvas, Base + 0.05f * Length * Dir, Tip - 0.25f * Length * Dir, 0.008f,
                Solid(ShadowbladeIconViolet(170)));
    v2 Guard = Grip + (0.2f * Length) * Dir;
    IconCapsule(Canvas, Guard - (2.2f * W) * Side, Guard + (2.2f * W) * Side, 0.03f,
                Solid(IconColor(16, 10, 20)));
    IconCapsule(Canvas, Guard - (2.f * W) * Side, Guard + (2.f * W) * Side, 0.018f,
                Gradient(IconColor(230, 180, 255), IconColor(130, 70, 200), Guard - W * Side, Guard + W * Side));
    IconCapsule(Canvas, Grip - (0.06f * Length) * Dir, Guard, 0.024f, Solid(IconColor(40, 22, 40)));
    IconCircle(Canvas, Grip - (0.1f * Length) * Dir, 0.034f, Solid(IconColor(16, 10, 20)));
    IconCircle(Canvas, Grip - (0.1f * Length) * Dir, 0.022f, Solid(ShadowbladeIconAcid()));
}

// NOTE(zoubir): a hooded figure standing at Feet, Height tall, in Paint
internal void
IconShadowbladeFigure(icon_canvas *Canvas, v2 Feet, float Height, icon_paint Paint)
{
    float H = Height;
    v2 Body[5] = {Feet + V2(-0.2f * H, 0.f), Feet + V2(-0.16f * H, -0.55f * H), Feet + V2(0.f, -0.68f * H),
                  Feet + V2(0.16f * H, -0.55f * H), Feet + V2(0.2f * H, 0.f)};
    IconPolygon(Canvas, Body, 5, Paint);
    IconCircle(Canvas, Feet + V2(0.f, -0.8f * H), 0.14f * H, Paint);
}

// NOTE(zoubir): Shadowstep: ghosts fading back along an arc that lands
// behind a foe, the Shadowblade dark against a violet flash
internal void
PaintShadowstepIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.66f, 0.56f), 0.44f, IconColor(170, 110, 255, 150));
    IconArc(Canvas, V2(0.5f, 0.78f), 0.36f, 0.035f,
            Gradient(ShadowbladeIconViolet(20), ShadowbladeIconViolet(230), V2(0.14f, 0.7f), V2(0.84f, 0.6f)),
            3.5f, 5.9f);
    IconShadowbladeFigure(Canvas, V2(0.2f, 0.9f), 0.42f, Solid(ShadowbladeIconViolet(60)));
    IconShadowbladeFigure(Canvas, V2(0.38f, 0.9f), 0.46f, Solid(ShadowbladeIconViolet(110)));
    IconShadowbladeFigure(Canvas, V2(0.7f, 0.92f), 0.6f,
                          Gradient(IconColor(90, 50, 140), ShadowbladeIconDeep(), V2(0.6f, 0.4f), V2(0.7f, 0.9f)));
    IconCircle(Canvas, V2(0.67f, 0.43f), 0.018f, Solid(ShadowbladeIconAcid()));
    IconCircle(Canvas, V2(0.73f, 0.43f), 0.018f, Solid(ShadowbladeIconAcid()));
    IconShadowbladeDagger(Canvas, V2(0.84f, 0.62f), V2(0.45f, 0.89f), 0.26f);
    IconSparkle(Canvas, V2(0.86f, 0.22f), 0.07f, Solid(IconColor(240, 220, 255)));
}

// NOTE(zoubir): Fan of Knives: knives flung out in a ring from a violet
// burst
internal void
PaintFanOfKnivesIcon(icon_canvas *Canvas)
{
    v2 Centre = V2(0.5f, 0.5f);
    IconGlow(Canvas, Centre, 0.5f, IconColor(170, 110, 255, 160));
    IconArc(Canvas, Centre, 0.27f, 0.03f, Solid(ShadowbladeIconViolet(150)));
    IconCircle(Canvas, Centre, 0.11f, Gradient(IconColor(250, 230, 255), ShadowbladeIconViolet(),
                                               V2(0.45f, 0.44f), V2(0.56f, 0.58f)));
    for(u32 Knife = 0; Knife < 8; Knife++)
    {
        float A = 2.f * Pi32 * (float)Knife / 8.f + 0.2f;
        v2 Dir = V2(Cos(A), Sin(A));
        IconCapsule(Canvas, Centre + 0.12f * Dir, Centre + 0.24f * Dir, 0.012f,
                    Gradient(ShadowbladeIconViolet(0), ShadowbladeIconViolet(200), Centre, Centre + 0.24f * Dir));
        IconShadowbladeDagger(Canvas, Centre + 0.22f * Dir, Dir, 0.24f);
    }
}

// NOTE(zoubir): Smoke Bomb: a round black bomb, its fuse sparking, smoke
// rolling out round it
internal void
PaintSmokeBombIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.48f, IconColor(140, 90, 200, 120));
    v4 Smoke = IconColor(110, 95, 130, 230);
    v4 SmokeDark = IconColor(60, 50, 75, 230);
    v2 Puffs[6] = {V2(0.2f, 0.74f), V2(0.36f, 0.82f), V2(0.56f, 0.84f), V2(0.76f, 0.76f),
                   V2(0.84f, 0.58f), V2(0.16f, 0.56f)};
    for(u32 Puff = 0; Puff < 6; Puff++)
    {
        IconCircle(Canvas, Puffs[Puff], 0.13f, Gradient(Smoke, SmokeDark, Puffs[Puff] - V2(0.f, 0.1f),
                                                         Puffs[Puff] + V2(0.f, 0.1f)));
    }
    IconCircle(Canvas, V2(0.5f, 0.56f), 0.22f, Solid(IconColor(16, 10, 20)));
    IconCircle(Canvas, V2(0.5f, 0.56f), 0.2f, Gradient(IconColor(90, 70, 120), IconColor(20, 14, 30),
                                                     V2(0.42f, 0.44f), V2(0.6f, 0.72f)));
    IconCircle(Canvas, V2(0.43f, 0.48f), 0.05f, Solid(IconColor(220, 200, 255, 160)));
    IconCapsule(Canvas, V2(0.6f, 0.38f), V2(0.66f, 0.3f), 0.04f, Solid(IconColor(60, 50, 70)));
    IconArc(Canvas, V2(0.76f, 0.3f), 0.1f, 0.02f, Solid(IconColor(200, 170, 120)), 3.2f, 4.6f);
    IconGlow(Canvas, V2(0.78f, 0.2f), 0.12f, IconColor(255, 220, 120, 220));
    IconSparkle(Canvas, V2(0.78f, 0.2f), 0.08f, Solid(IconColor(255, 245, 210)));
}

// NOTE(zoubir): Shadow Dance: the Shadowblade and its shadow double, both
// blades raised, under a violet moon
internal void
PaintShadowDanceIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.45f), 0.5f, IconColor(170, 110, 255, 170));
    IconCircle(Canvas, V2(0.5f, 0.32f), 0.2f, Gradient(IconColor(240, 220, 255), ShadowbladeIconViolet(),
                                                     V2(0.4f, 0.2f), V2(0.6f, 0.46f)));
    IconCircle(Canvas, V2(0.58f, 0.28f), 0.17f, Solid(IconColor(60, 30, 90, 235)));
    IconShadowbladeFigure(Canvas, V2(0.64f, 0.94f), 0.56f, Solid(ShadowbladeIconViolet(150)));
    IconShadowbladeFigure(Canvas, V2(0.4f, 0.94f), 0.6f,
                          Gradient(IconColor(80, 46, 120), IconColor(20, 10, 30), V2(0.4f, 0.4f), V2(0.4f, 0.94f)));
    IconCircle(Canvas, V2(0.37f, 0.45f), 0.018f, Solid(ShadowbladeIconAcid()));
    IconCircle(Canvas, V2(0.43f, 0.45f), 0.018f, Solid(ShadowbladeIconAcid()));
    IconShadowbladeDagger(Canvas, V2(0.2f, 0.64f), V2(-0.3f, -0.95f), 0.26f);
    IconShadowbladeDagger(Canvas, V2(0.84f, 0.62f), V2(0.3f, -0.95f), 0.24f);
}

// NOTE(zoubir): Eviscerate: three deep cuts raked across, five combo gems
// under them
internal void
PaintEviscerateIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.42f), 0.48f, IconColor(200, 60, 160, 150));
    for(u32 Cut = 0; Cut < 3; Cut++)
    {
        float O = 0.15f * ((float)Cut - 1.f);
        v2 A = V2(0.22f + O, 0.14f);
        v2 B = V2(0.62f + O, 0.66f);
        IconCapsule(Canvas, A, B, 0.045f, Gradient(IconColor(120, 30, 110, 0), IconColor(170, 50, 150, 230), A, B));
        IconCapsule(Canvas, A + V2(0.03f, 0.04f), B - V2(0.04f, 0.05f), 0.018f,
                    Gradient(IconColor(255, 230, 255, 40), IconColor(255, 240, 255), A, B));
    }
    for(u32 Gem = 0; Gem < 5; Gem++)
    {
        v2 P = V2(0.18f + 0.16f * (float)Gem, 0.86f);
        v2 Outer[4] = {P + V2(0.f, -0.075f), P + V2(0.075f, 0.f), P + V2(0.f, 0.075f), P + V2(-0.075f, 0.f)};
        v2 Inner[4] = {P + V2(0.f, -0.055f), P + V2(0.055f, 0.f), P + V2(0.f, 0.055f), P + V2(-0.055f, 0.f)};
        IconPolygon(Canvas, Outer, 4, Solid(IconColor(16, 10, 20)));
        IconPolygon(Canvas, Inner, 4, Gradient(IconColor(250, 225, 255), ShadowbladeIconViolet(),
                                               P - V2(0.f, 0.05f), P + V2(0.f, 0.05f)));
    }
}

// NOTE(zoubir): Poisoned Shiv: a thrown dagger, green venom dripping off
// its point
internal void
PaintPoisonedShivIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.6f, 0.6f), 0.46f, IconColor(140, 255, 80, 130));
    IconCapsule(Canvas, V2(0.08f, 0.14f), V2(0.32f, 0.34f), 0.03f,
                Gradient(ShadowbladeIconViolet(0), ShadowbladeIconViolet(200), V2(0.08f, 0.14f), V2(0.32f, 0.34f)));
    IconShadowbladeDagger(Canvas, V2(0.26f, 0.26f), V2(0.7071f, 0.7071f), 0.56f);
    IconCircle(Canvas, V2(0.72f, 0.8f), 0.055f, Solid(ShadowbladeIconAcid()));
    IconTriangle(Canvas, V2(0.67f, 0.79f), V2(0.77f, 0.79f), V2(0.72f, 0.68f), Solid(ShadowbladeIconAcid()));
    IconCircle(Canvas, V2(0.6f, 0.9f), 0.035f, Solid(ShadowbladeIconAcid(220)));
    IconCircle(Canvas, V2(0.84f, 0.9f), 0.028f, Solid(IconColor(210, 130, 255, 220)));
    IconCircle(Canvas, V2(0.7f, 0.77f), 0.018f, Solid(IconColor(240, 255, 220)));
}

// NOTE(zoubir): Twin Strike: two daggers crossed under two crossing cuts
internal void
PaintTwinStrikeIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(170, 110, 255, 140));
    IconArc(Canvas, V2(0.2f, 0.8f), 0.62f, 0.05f,
            Gradient(ShadowbladeIconViolet(0), IconColor(235, 210, 255, 240), V2(0.1f, 0.2f), V2(0.8f, 0.6f)),
            4.9f, 5.95f);
    IconArc(Canvas, V2(0.8f, 0.8f), 0.62f, 0.05f,
            Gradient(IconColor(200, 255, 140, 240), ShadowbladeIconAcid(0), V2(0.2f, 0.6f), V2(0.9f, 0.2f)),
            3.47f, 4.52f);
    IconShadowbladeDagger(Canvas, V2(0.24f, 0.82f), V2(0.6f, -0.8f), 0.6f);
    IconShadowbladeDagger(Canvas, V2(0.76f, 0.82f), V2(-0.6f, -0.8f), 0.6f);
}

// NOTE(zoubir): a small "more" badge in the bottom-right corner
internal void
IconShadowbladeBadgeMore(icon_canvas *Canvas)
{
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.16f, Solid(IconColor(16, 10, 20)));
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.13f, Solid(IconColor(255, 205, 90)));
    IconTriangle(Canvas, V2(0.8f, 0.7f), V2(0.87f, 0.8f), V2(0.73f, 0.8f), Solid(IconColor(30, 24, 10)));
    IconCapsule(Canvas, V2(0.8f, 0.79f), V2(0.8f, 0.88f), 0.022f, Solid(IconColor(30, 24, 10)));
}

// NOTE(zoubir): Lethality: a blade's point and a bead of blood, sharpened
internal void
PaintLethalityIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.46f, 0.5f), 0.46f, IconColor(200, 70, 150, 140));
    IconShadowbladeDagger(Canvas, V2(0.78f, 0.14f), V2(-0.6f, 0.8f), 0.74f);
    IconCircle(Canvas, V2(0.32f, 0.82f), 0.06f, Solid(IconColor(210, 40, 80)));
    IconTriangle(Canvas, V2(0.27f, 0.8f), V2(0.37f, 0.8f), V2(0.32f, 0.69f), Solid(IconColor(210, 40, 80)));
    IconSparkle(Canvas, V2(0.26f, 0.6f), 0.07f, Solid(IconColor(255, 255, 255)));
    IconShadowbladeBadgeMore(Canvas);
}

// NOTE(zoubir): Venom: a round flask of green poison, bubbling
internal void
PaintVenomIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.46f, IconColor(140, 255, 80, 150));
    IconCapsule(Canvas, V2(0.5f, 0.12f), V2(0.5f, 0.36f), 0.08f, Solid(IconColor(16, 10, 20)));
    IconCapsule(Canvas, V2(0.5f, 0.14f), V2(0.5f, 0.36f), 0.06f, Solid(IconColor(180, 190, 210, 200)));
    IconCapsule(Canvas, V2(0.42f, 0.12f), V2(0.58f, 0.12f), 0.035f, Solid(IconColor(120, 70, 50)));
    IconCircle(Canvas, V2(0.5f, 0.62f), 0.28f, Solid(IconColor(16, 10, 20)));
    IconCircle(Canvas, V2(0.5f, 0.62f), 0.255f, Solid(IconColor(190, 200, 220, 120)));
    IconCircle(Canvas, V2(0.5f, 0.66f), 0.22f, Gradient(IconColor(200, 255, 120), IconColor(60, 150, 30),
                                                      V2(0.5f, 0.5f), V2(0.5f, 0.86f)));
    IconCircle(Canvas, V2(0.44f, 0.64f), 0.04f, Solid(IconColor(230, 255, 200, 200)));
    IconCircle(Canvas, V2(0.58f, 0.72f), 0.03f, Solid(IconColor(230, 255, 200, 200)));
    IconCircle(Canvas, V2(0.52f, 0.54f), 0.022f, Solid(IconColor(230, 255, 200, 200)));
    IconCircle(Canvas, V2(0.4f, 0.52f), 0.05f, Solid(IconColor(255, 255, 255, 120)));
    IconShadowbladeBadgeMore(Canvas);
}

// NOTE(zoubir): Opportunist: a foe facing away, a dagger at its back
internal void
PaintOpportunistIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.4f, 0.5f), 0.46f, IconColor(170, 110, 255, 140));
    IconShadowbladeFigure(Canvas, V2(0.62f, 0.94f), 0.74f,
                          Gradient(IconColor(170, 90, 80), IconColor(70, 30, 30), V2(0.6f, 0.3f), V2(0.6f, 0.9f)));
    IconCircle(Canvas, V2(0.72f, 0.34f), 0.02f, Solid(IconColor(255, 240, 160)));
    IconArc(Canvas, V2(0.62f, 0.6f), 0.34f, 0.03f, Solid(ShadowbladeIconViolet(200)), 2.4f, 3.9f);
    IconShadowbladeDagger(Canvas, V2(0.12f, 0.36f), V2(0.92f, 0.38f), 0.5f);
    IconSparkle(Canvas, V2(0.56f, 0.54f), 0.08f, Solid(ShadowbladeIconAcid()));
}

// NOTE(zoubir): Relentless: a dagger inside a ring that turns back on
// itself, the points it gives back beside it
internal void
PaintRelentlessIcon(icon_canvas *Canvas)
{
    v2 Centre = V2(0.5f, 0.5f);
    IconGlow(Canvas, Centre, 0.48f, IconColor(170, 110, 255, 150));
    IconArc(Canvas, Centre, 0.34f, 0.06f, Gradient(ShadowbladeIconViolet(60), IconColor(235, 210, 255),
                                                   V2(0.2f, 0.8f), V2(0.8f, 0.2f)), -1.2f, 3.9f);
    IconTriangle(Canvas, V2(0.56f, 0.06f), V2(0.72f, 0.22f), V2(0.5f, 0.28f), Solid(IconColor(235, 210, 255)));
    IconShadowbladeDagger(Canvas, V2(0.36f, 0.72f), V2(0.6f, -0.8f), 0.44f);
    for(u32 Gem = 0; Gem < 3; Gem++)
    {
        v2 P = V2(0.66f + 0.0f * (float)Gem, 0.56f + 0.13f * (float)Gem);
        v2 Diamond[4] = {P + V2(0.f, -0.05f), P + V2(0.05f, 0.f), P + V2(0.f, 0.05f), P + V2(-0.05f, 0.f)};
        IconPolygon(Canvas, Diamond, 4, Solid(ShadowbladeIconAcid()));
    }
}

global_variable role_icon_painter *ShadowbladeIconPainters[ROLE_KEYS] =
{
    PaintShadowstepIcon, PaintFanOfKnivesIcon, PaintSmokeBombIcon, PaintShadowDanceIcon,
    PaintEviscerateIcon, PaintPoisonedShivIcon, PaintTwinStrikeIcon,
};
global_variable talent_icon_painter *ShadowbladeTalentIconPainters[ROLE_TALENTS] =
{
    PaintLethalityIcon, PaintSmokeBombIcon, PaintVenomIcon, PaintOpportunistIcon,
    PaintShadowDanceIcon, PaintRelentlessIcon,
};
