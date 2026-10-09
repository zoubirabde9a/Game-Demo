/* Duelist icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Duelist's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty. Rose and steel, with gold for
   the hilt and the parry. */

inline v4 DuelistIconRose(u32 A = 255) { return IconColor(240, 110, 170, A); }
inline v4 DuelistIconPale(u32 A = 255) { return IconColor(255, 215, 235, A); }
inline v4 DuelistIconGold(u32 A = 255) { return IconColor(255, 205, 90, A); }
inline v4 DuelistIconDark(u32 A = 255) { return IconColor(20, 10, 20, A); }

inline v2
DuelistNormalIcon(v2 V)
{
    v2 Result = DirectionTo(V);
    return Result;
}

// NOTE(zoubir): a rapier from its grip at Grip along Dir (a unit vector),
// Length long: a long thin steel blade, a gold cup hilt with its knuckle
// bow, a rose grip
internal void
IconDuelistRapier(icon_canvas *Canvas, v2 Grip, v2 Dir, float Length)
{
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Base = Grip + (0.14f * Length) * Dir;
    v2 Tip = Grip + Length * Dir;
    IconTriangle(Canvas, Base - 0.03f * Side, Tip + 0.02f * Dir, Base + 0.03f * Side, Solid(DuelistIconDark()));
    IconTriangle(Canvas, Base - 0.016f * Side, Tip, Base,
                 Gradient(IconColor(255, 255, 255), IconColor(215, 215, 230), Base, Tip));
    IconTriangle(Canvas, Base, Tip, Base + 0.016f * Side,
                 Gradient(IconColor(160, 155, 175), IconColor(120, 115, 135), Base, Tip));
    v2 Cup = Grip + (0.11f * Length) * Dir;
    v2 Pommel = Grip - (0.07f * Length) * Dir;
    v2 Bow = Grip + (0.03f * Length) * Dir + (0.09f * Length) * Side;
    IconCapsule(Canvas, Cup + 0.03f * Side, Bow, 0.022f, Solid(DuelistIconDark()));
    IconCapsule(Canvas, Bow, Pommel, 0.022f, Solid(DuelistIconDark()));
    IconCapsule(Canvas, Cup + 0.03f * Side, Bow, 0.011f, Solid(DuelistIconGold()));
    IconCapsule(Canvas, Bow, Pommel, 0.011f, Solid(DuelistIconGold()));
    IconCapsule(Canvas, Pommel, Cup, 0.026f, Solid(DuelistIconDark()));
    IconCapsule(Canvas, Pommel, Cup, 0.015f, Gradient(IconColor(120, 30, 80), DuelistIconRose(), Pommel, Cup));
    IconCircle(Canvas, Cup, 0.05f, Solid(DuelistIconDark()));
    IconCircle(Canvas, Cup, 0.036f, Gradient(IconColor(255, 235, 160), IconColor(200, 140, 40),
                                             Cup - V2(0.02f, 0.02f), Cup + V2(0.02f, 0.02f)));
    IconCircle(Canvas, Pommel, 0.026f, Solid(DuelistIconGold()));
}

// NOTE(zoubir): a heart centred on C, Size across
internal void
IconDuelistHeart(icon_canvas *Canvas, v2 C, float Size, icon_paint Paint)
{
    IconCircle(Canvas, C + V2(-0.24f * Size, -0.1f * Size), 0.28f * Size, Paint);
    IconCircle(Canvas, C + V2(0.24f * Size, -0.1f * Size), 0.28f * Size, Paint);
    IconTriangle(Canvas, C + V2(-0.5f * Size, -0.02f * Size), C + V2(0.5f * Size, -0.02f * Size),
                 C + V2(0.f, 0.52f * Size), Paint);
}

// NOTE(zoubir): a thin Tempo diamond at P, Size tall, lit or not
internal void
IconDuelistDiamond(icon_canvas *Canvas, v2 P, float Size, bool32 Lit)
{
    float H = 0.5f * Size;
    float W = 0.32f * Size;
    v2 Outer[4] = {P + V2(0.f, -H - 0.02f), P + V2(W + 0.02f, 0.f), P + V2(0.f, H + 0.02f), P + V2(-W - 0.02f, 0.f)};
    v2 Inner[4] = {P + V2(0.f, -H), P + V2(W, 0.f), P + V2(0.f, H), P + V2(-W, 0.f)};
    v2 Core[4] = {P + V2(0.f, -0.65f * H), P + V2(0.65f * W, 0.f), P + V2(0.f, 0.65f * H), P + V2(-0.65f * W, 0.f)};
    IconPolygon(Canvas, Outer, 4, Solid(DuelistIconDark()));
    IconPolygon(Canvas, Inner, 4, Solid(DuelistIconPale()));
    IconPolygon(Canvas, Core, 4, Lit ? Gradient(DuelistIconPale(), DuelistIconRose(), P - V2(0.f, H), P + V2(0.f, H)) :
                Solid(IconColor(60, 30, 50)));
}

// NOTE(zoubir): a fencer side-on at Feet, Height tall, facing right
internal void
IconDuelistFencer(icon_canvas *Canvas, v2 Feet, float Height, icon_paint Paint)
{
    float H = Height;
    v2 Hip = Feet + V2(0.f, -0.45f * H);
    v2 Neck = Feet + V2(0.03f * H, -0.75f * H);
    IconCapsule(Canvas, Hip, Neck, 0.09f * H, Paint);
    IconCapsule(Canvas, Hip, Feet + V2(0.26f * H, 0.f), 0.05f * H, Paint);
    IconCapsule(Canvas, Hip, Feet + V2(-0.2f * H, 0.f), 0.05f * H, Paint);
    IconCapsule(Canvas, Neck, Neck + V2(-0.2f * H, -0.12f * H), 0.04f * H, Paint);
    IconCapsule(Canvas, Neck, Neck + V2(0.24f * H, 0.08f * H), 0.04f * H, Paint);
    IconCircle(Canvas, Feet + V2(0.04f * H, -0.88f * H), 0.1f * H, Paint);
}

// NOTE(zoubir): Thrust: a rapier driven straight out, a speed line along it
internal void
PaintThrustIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.6f, 0.42f), 0.46f, IconColor(240, 110, 170, 130));
    IconCapsule(Canvas, V2(0.1f, 0.36f), V2(0.62f, 0.36f), 0.012f, Gradient(DuelistIconPale(0), DuelistIconPale(220),
                                                                           V2(0.1f, 0.f), V2(0.62f, 0.f)));
    IconCapsule(Canvas, V2(0.16f, 0.66f), V2(0.56f, 0.66f), 0.012f, Gradient(DuelistIconPale(0), DuelistIconPale(180),
                                                                            V2(0.16f, 0.f), V2(0.56f, 0.f)));
    IconDuelistRapier(Canvas, V2(0.16f, 0.6f), DuelistNormalIcon(V2(1.f, -0.25f)), 0.86f);
    IconSparkle(Canvas, V2(0.92f, 0.38f), 0.08f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Lunge: a fencer stretched out low, a rose streak behind
internal void
PaintLungeIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.48f, IconColor(240, 110, 170, 140));
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Y = 0.52f + 0.1f * (float)Line;
        IconCapsule(Canvas, V2(0.04f, Y), V2(0.42f, Y - 0.04f), 0.02f,
                    Gradient(DuelistIconRose(0), DuelistIconRose(230), V2(0.04f, 0.f), V2(0.42f, 0.f)));
    }
    v2 Hip = V2(0.4f, 0.62f);
    icon_paint Body = Gradient(IconColor(90, 40, 70), DuelistIconDark(), V2(0.4f, 0.4f), V2(0.4f, 0.9f));
    IconCapsule(Canvas, Hip, V2(0.5f, 0.42f), 0.06f, Body);
    IconCapsule(Canvas, Hip, V2(0.66f, 0.88f), 0.04f, Body);
    IconCapsule(Canvas, Hip, V2(0.16f, 0.86f), 0.04f, Body);
    IconCapsule(Canvas, V2(0.5f, 0.44f), V2(0.32f, 0.3f), 0.03f, Body);
    IconCircle(Canvas, V2(0.55f, 0.32f), 0.07f, Body);
    IconDuelistRapier(Canvas, V2(0.58f, 0.46f), DuelistNormalIcon(V2(1.f, -0.15f)), 0.42f);
}

// NOTE(zoubir): Feint: the fencer stepping aside, a pale afterimage where
// it stood and a blow's streak passing through it
internal void
PaintFeintIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.55f, 0.55f), 0.48f, IconColor(240, 110, 170, 120));
    IconDuelistFencer(Canvas, V2(0.32f, 0.9f), 0.7f, Solid(IconColor(240, 180, 210, 90)));
    IconDuelistFencer(Canvas, V2(0.66f, 0.9f), 0.7f,
                      Gradient(IconColor(90, 40, 70), DuelistIconDark(), V2(0.6f, 0.3f), V2(0.6f, 0.9f)));
    IconCapsule(Canvas, V2(0.04f, 0.5f), V2(0.5f, 0.42f), 0.022f,
                Gradient(DuelistIconRose(0), IconColor(255, 230, 240), V2(0.04f, 0.f), V2(0.5f, 0.f)));
}

// NOTE(zoubir): Riposte: the rapier held across, a gold clang where a blow
// meets it, and the counter's point turning back
internal void
PaintRiposteIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.55f, 0.4f), 0.48f, IconColor(255, 205, 90, 140));
    IconArc(Canvas, V2(0.42f, 0.62f), 0.42f, 0.04f, Gradient(DuelistIconPale(30), IconColor(255, 255, 255, 240),
                                                            V2(0.2f, 0.9f), V2(0.8f, 0.2f)), 4.3f, 5.9f);
    IconDuelistRapier(Canvas, V2(0.2f, 0.84f), DuelistNormalIcon(V2(0.62f, -1.f)), 0.84f);
    IconSparkle(Canvas, V2(0.58f, 0.38f), 0.2f, Solid(DuelistIconGold()));
    IconSparkle(Canvas, V2(0.58f, 0.38f), 0.1f, Solid(IconColor(255, 255, 255)));
    IconCapsule(Canvas, V2(0.92f, 0.12f), V2(0.72f, 0.28f), 0.025f, Solid(IconColor(160, 160, 180)));
    IconCircle(Canvas, V2(0.76f, 0.5f), 0.025f, Solid(DuelistIconGold()));
    IconCircle(Canvas, V2(0.42f, 0.22f), 0.02f, Solid(DuelistIconGold()));
}

// NOTE(zoubir): Heartseeker: a rapier piercing a rose heart
internal void
PaintHeartseekerIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.56f, 0.46f), 0.48f, IconColor(240, 110, 170, 170));
    IconDuelistHeart(Canvas, V2(0.58f, 0.44f), 0.56f, Solid(DuelistIconDark()));
    IconDuelistHeart(Canvas, V2(0.58f, 0.44f), 0.48f,
                     Gradient(IconColor(255, 170, 210), IconColor(200, 40, 110), V2(0.4f, 0.2f), V2(0.6f, 0.7f)));
    IconCircle(Canvas, V2(0.48f, 0.34f), 0.05f, Solid(IconColor(255, 255, 255, 170)));
    IconDuelistRapier(Canvas, V2(0.08f, 0.86f), DuelistNormalIcon(V2(1.f, -0.85f)), 0.98f);
}

// NOTE(zoubir): Perfect Form: a fencer and two rose afterimages behind
// it, petals about
internal void
PaintPerfectFormIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(240, 110, 170, 160));
    IconDuelistFencer(Canvas, V2(0.3f, 0.92f), 0.66f, Solid(DuelistIconRose(70)));
    IconDuelistFencer(Canvas, V2(0.42f, 0.92f), 0.68f, Solid(DuelistIconRose(130)));
    IconDuelistFencer(Canvas, V2(0.56f, 0.92f), 0.7f,
                      Gradient(IconColor(110, 50, 90), DuelistIconDark(), V2(0.5f, 0.3f), V2(0.5f, 0.9f)));
    IconDuelistRapier(Canvas, V2(0.74f, 0.42f), DuelistNormalIcon(V2(1.f, -0.6f)), 0.3f);
    IconSparkle(Canvas, V2(0.84f, 0.16f), 0.08f, Solid(DuelistIconPale()));
    IconCircle(Canvas, V2(0.16f, 0.24f), 0.03f, Solid(DuelistIconRose()));
    IconCircle(Canvas, V2(0.86f, 0.62f), 0.025f, Solid(DuelistIconRose()));
}

// NOTE(zoubir): a small "more" badge in the bottom-right corner
internal void
IconDuelistBadgeMore(icon_canvas *Canvas)
{
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.16f, Solid(DuelistIconDark()));
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.13f, Solid(DuelistIconGold()));
    IconTriangle(Canvas, V2(0.8f, 0.7f), V2(0.87f, 0.8f), V2(0.73f, 0.8f), Solid(IconColor(30, 24, 10)));
    IconCapsule(Canvas, V2(0.8f, 0.79f), V2(0.8f, 0.88f), 0.022f, Solid(IconColor(30, 24, 10)));
}

// NOTE(zoubir): Finesse: the rapier's point, a glint on it
internal void
PaintFinesseIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(240, 110, 170, 140));
    IconDuelistRapier(Canvas, V2(0.14f, 0.86f), DuelistNormalIcon(V2(0.75f, -1.f)), 0.9f);
    IconSparkle(Canvas, V2(0.62f, 0.2f), 0.14f, Solid(IconColor(255, 255, 255)));
    IconDuelistBadgeMore(Canvas);
}

// NOTE(zoubir): Footwork: two footprints and speed lines
internal void
PaintFootworkIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.46f, IconColor(240, 110, 170, 130));
    for(u32 Foot = 0; Foot < 2; Foot++)
    {
        v2 P = Foot ? V2(0.62f, 0.34f) : V2(0.38f, 0.66f);
        IconCapsule(Canvas, P - V2(0.06f, 0.f), P + V2(0.08f, -0.03f), 0.07f, Solid(DuelistIconDark()));
        IconCapsule(Canvas, P - V2(0.06f, 0.f), P + V2(0.08f, -0.03f), 0.05f,
                    Gradient(DuelistIconPale(), DuelistIconRose(), P - V2(0.06f, 0.f), P + V2(0.08f, 0.f)));
    }
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Y = 0.2f + 0.24f * (float)Line;
        IconCapsule(Canvas, V2(0.04f, Y + 0.12f), V2(0.24f, Y + 0.08f), 0.015f,
                    Gradient(DuelistIconPale(0), DuelistIconPale(220), V2(0.04f, 0.f), V2(0.24f, 0.f)));
    }
}

// NOTE(zoubir): Precision: a cracked heart under a sight, half its health
internal void
PaintPrecisionIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(240, 110, 170, 140));
    IconDuelistHeart(Canvas, V2(0.5f, 0.48f), 0.5f, Solid(DuelistIconDark()));
    IconDuelistHeart(Canvas, V2(0.5f, 0.48f), 0.42f, Gradient(IconColor(255, 170, 210), IconColor(200, 40, 110),
                                                              V2(0.4f, 0.3f), V2(0.6f, 0.7f)));
    IconCapsule(Canvas, V2(0.5f, 0.3f), V2(0.44f, 0.46f), 0.015f, Solid(DuelistIconDark()));
    IconCapsule(Canvas, V2(0.44f, 0.46f), V2(0.54f, 0.6f), 0.015f, Solid(DuelistIconDark()));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.38f, 0.035f, Solid(IconColor(255, 255, 255, 220)));
    IconCapsule(Canvas, V2(0.5f, 0.04f), V2(0.5f, 0.18f), 0.02f, Solid(IconColor(255, 255, 255, 220)));
    IconCapsule(Canvas, V2(0.5f, 0.82f), V2(0.5f, 0.96f), 0.02f, Solid(IconColor(255, 255, 255, 220)));
    IconCapsule(Canvas, V2(0.04f, 0.5f), V2(0.18f, 0.5f), 0.02f, Solid(IconColor(255, 255, 255, 220)));
    IconCapsule(Canvas, V2(0.82f, 0.5f), V2(0.96f, 0.5f), 0.02f, Solid(IconColor(255, 255, 255, 220)));
}

// NOTE(zoubir): Bait: the guard's arc held open, a green heal rising
internal void
PaintBaitIcon(icon_canvas *Canvas)
{
    PaintRiposteIcon(Canvas);
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.16f, Solid(DuelistIconDark()));
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.13f, Solid(IconColor(120, 220, 140)));
    IconCapsule(Canvas, V2(0.8f, 0.72f), V2(0.8f, 0.88f), 0.025f, Solid(IconColor(20, 60, 30)));
    IconCapsule(Canvas, V2(0.72f, 0.8f), V2(0.88f, 0.8f), 0.025f, Solid(IconColor(20, 60, 30)));
}

// NOTE(zoubir): Crescendo: five lit diamonds over a wide rose crescent
internal void
PaintCrescendoIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.48f, IconColor(240, 110, 170, 150));
    IconArc(Canvas, V2(0.5f, 1.05f), 0.6f, 0.08f, Gradient(DuelistIconRose(60), DuelistIconPale(),
                                                          V2(0.1f, 0.7f), V2(0.9f, 0.5f)), 3.7f, 5.7f);
    IconDuelistRapier(Canvas, V2(0.5f, 0.98f), V2(0.f, -1.f), 0.6f);
    for(u32 Stack = 0; Stack < 5; Stack++)
    {
        IconDuelistDiamond(Canvas, V2(0.18f + 0.16f * (float)Stack, 0.16f), 0.18f, true);
    }
}

// NOTE(zoubir): Flurry: three rapier points side by side
internal void
PaintFlurryIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.6f, 0.5f), 0.46f, IconColor(240, 110, 170, 140));
    for(u32 Blade = 0; Blade < 3; Blade++)
    {
        float Y = 0.26f + 0.24f * (float)Blade;
        IconDuelistRapier(Canvas, V2(0.12f + 0.06f * (float)(Blade % 2), Y), V2(1.f, 0.f), 0.78f);
    }
    IconDuelistBadgeMore(Canvas);
}

// NOTE(zoubir): Masterstroke: two hearts pierced on one blade
internal void
PaintMasterstrokeIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(240, 110, 170, 170));
    v2 Hearts[2] = {V2(0.36f, 0.62f), V2(0.66f, 0.34f)};
    for(u32 Heart = 0; Heart < 2; Heart++)
    {
        float S = Heart ? 0.34f : 0.3f;
        IconDuelistHeart(Canvas, Hearts[Heart], S + 0.06f, Solid(DuelistIconDark()));
        IconDuelistHeart(Canvas, Hearts[Heart], S, Gradient(IconColor(255, 170, 210), IconColor(200, 40, 110),
                                                            Hearts[Heart] - V2(0.f, 0.1f), Hearts[Heart] + V2(0.f, 0.15f)));
    }
    IconDuelistRapier(Canvas, V2(0.08f, 0.9f), DuelistNormalIcon(V2(1.f, -1.f)), 1.f);
    IconSparkle(Canvas, V2(0.86f, 0.14f), 0.09f, Solid(DuelistIconGold()));
}

global_variable role_icon_painter *DuelistIconPainters[ROLE_KEYS] =
{
    PaintLungeIcon, PaintRiposteIcon, PaintFeintIcon, PaintPerfectFormIcon,
    PaintHeartseekerIcon, 0, PaintThrustIcon,
};
global_variable talent_icon_painter *DuelistTalentIconPainters[ROLE_TALENTS] =
{
    PaintFinesseIcon, PaintFootworkIcon, PaintPrecisionIcon, PaintBaitIcon,
    PaintPerfectFormIcon, PaintCrescendoIcon,
    PaintStatDamageIcon, PaintStatArmorIcon, PaintFlurryIcon, PaintStatHasteIcon,
    PaintStatVitalityIcon, PaintMasterstrokeIcon,
};
