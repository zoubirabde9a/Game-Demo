/* Role talent icons (sim/dungeon/role_talents.cpp): one per slot of each
   role's branch, in the talent panel's atlas after the game's talents
   (ui/talent_panel/talent_icons.cpp, TalentIconCell). A talent that
   changes a role spell shows that spell's icon (role_icons.cpp) with a
   badge saying how: an arrow for more, a clock for sooner, rings for
   wider. The rest have their own pictures here. */

// NOTE(zoubir): a small badge in the bottom-right corner
internal void
IconBadge(icon_canvas *Canvas, v4 Color)
{
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.17f, Solid(IconColor(16, 18, 26)));
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.14f, Solid(Color));
}

// NOTE(zoubir): "more": an arrow pointing up in the badge
internal void
IconBadgeMore(icon_canvas *Canvas)
{
    IconBadge(Canvas, IconColor(255, 205, 90));
    IconTriangle(Canvas, V2(0.78f, 0.67f), V2(0.86f, 0.79f), V2(0.7f, 0.79f), Solid(IconColor(30, 24, 10)));
    IconCapsule(Canvas, V2(0.78f, 0.78f), V2(0.78f, 0.88f), 0.025f, Solid(IconColor(30, 24, 10)));
}

// NOTE(zoubir): "sooner": a clock face in the badge
internal void
IconBadgeSooner(icon_canvas *Canvas)
{
    IconBadge(Canvas, IconColor(150, 220, 255));
    IconCapsule(Canvas, V2(0.78f, 0.78f), V2(0.78f, 0.69f), 0.022f, Solid(IconColor(16, 24, 40)));
    IconCapsule(Canvas, V2(0.78f, 0.78f), V2(0.85f, 0.81f), 0.022f, Solid(IconColor(16, 24, 40)));
}

// NOTE(zoubir): "wider": two rings in the badge
internal void
IconBadgeWider(icon_canvas *Canvas)
{
    IconBadge(Canvas, IconColor(255, 160, 80));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.05f, 0.02f, Solid(IconColor(30, 16, 8)));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.1f, 0.02f, Solid(IconColor(30, 16, 8)));
}

// NOTE(zoubir): Iron Skin: a breastplate with rivets
internal void
PaintIronSkinIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(140, 170, 220, 110));
    v2 Plate[6] = {V2(0.24f, 0.22f), V2(0.76f, 0.22f), V2(0.8f, 0.5f), V2(0.66f, 0.82f),
                   V2(0.34f, 0.82f), V2(0.2f, 0.5f)};
    IconPolygon(Canvas, Plate, 6, Gradient(IconColor(230, 235, 245), IconColor(90, 100, 125),
                                           V2(0.4f, 0.2f), V2(0.6f, 0.85f)));
    IconCapsule(Canvas, V2(0.5f, 0.28f), V2(0.5f, 0.76f), 0.02f, Solid(IconColor(70, 80, 100)));
    for(u32 Rivet = 0; Rivet < 4; Rivet++)
    {
        float Y = 0.34f + 0.12f * (float)Rivet;
        IconCircle(Canvas, V2(0.32f + 0.02f * (float)Rivet, Y), 0.025f, Solid(IconColor(240, 205, 90)));
        IconCircle(Canvas, V2(0.68f - 0.02f * (float)Rivet, Y), 0.025f, Solid(IconColor(240, 205, 90)));
    }
}

// NOTE(zoubir): Renewal: a green leaf with light dripping off it
internal void
PaintRenewalIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(110, 240, 140, 120));
    v2 Leaf[6] = {V2(0.5f, 0.14f), V2(0.7f, 0.32f), V2(0.72f, 0.56f), V2(0.5f, 0.78f),
                  V2(0.28f, 0.56f), V2(0.3f, 0.32f)};
    IconPolygon(Canvas, Leaf, 6, Gradient(IconColor(190, 255, 190), IconColor(40, 150, 70),
                                          V2(0.4f, 0.15f), V2(0.6f, 0.8f)));
    IconCapsule(Canvas, V2(0.5f, 0.22f), V2(0.5f, 0.86f), 0.022f, Solid(IconColor(30, 110, 50)));
    IconCircle(Canvas, V2(0.68f, 0.84f), 0.04f, Solid(IconColor(200, 255, 210)));
    IconCircle(Canvas, V2(0.32f, 0.88f), 0.03f, Solid(IconColor(200, 255, 210)));
}

// NOTE(zoubir): Miracle: a figure rising in a column of gold
internal void
PaintMiracleIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(255, 220, 110, 140));
    v2 Beam[4] = {V2(0.34f, 0.9f), V2(0.66f, 0.9f), V2(0.58f, 0.08f), V2(0.42f, 0.08f)};
    IconPolygon(Canvas, Beam, 4, Gradient(IconColor(255, 230, 140, 220), IconColor(255, 230, 140, 0),
                                          V2(0.5f, 0.9f), V2(0.5f, 0.08f)));
    IconCircle(Canvas, V2(0.5f, 0.36f), 0.08f, Solid(IconColor(255, 250, 230)));
    IconCapsule(Canvas, V2(0.5f, 0.46f), V2(0.5f, 0.68f), 0.07f, Solid(IconColor(255, 245, 215)));
    IconCapsule(Canvas, V2(0.36f, 0.48f), V2(0.64f, 0.48f), 0.04f, Solid(IconColor(255, 245, 215)));
    IconArc(Canvas, V2(0.5f, 0.24f), 0.1f, 0.025f, Solid(IconColor(255, 210, 90)));
}

// NOTE(zoubir): Pyromancer: a hand wreathed in flame
internal void
PaintPyromancerIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.45f), 0.48f, IconColor(255, 110, 30, 150));
    IconCapsule(Canvas, V2(0.5f, 0.9f), V2(0.5f, 0.62f), 0.11f, Solid(IconColor(200, 150, 120)));
    for(u32 Finger = 0; Finger < 4; Finger++)
    {
        float X = 0.38f + 0.08f * (float)Finger;
        IconCapsule(Canvas, V2(X, 0.6f), V2(X, 0.48f - 0.03f * (float)(Finger % 3)), 0.03f,
                    Solid(IconColor(215, 165, 135)));
    }
    v2 Flame[5] = {V2(0.5f, 0.06f), V2(0.66f, 0.3f), V2(0.62f, 0.5f), V2(0.38f, 0.5f), V2(0.34f, 0.3f)};
    IconPolygon(Canvas, Flame, 5, Gradient(IconColor(255, 240, 150), IconColor(230, 70, 20),
                                           V2(0.5f, 0.5f), V2(0.5f, 0.06f)));
}

// NOTE(zoubir): Molten Ground: Meteor's fire with a pale blue badge, an
// arrow pointing down: the monsters in it walk slower
internal void
PaintMoltenGroundIcon(icon_canvas *Canvas)
{
    PaintRoleInfernoIcon(Canvas);
    IconBadge(Canvas, IconColor(150, 210, 255));
    IconTriangle(Canvas, V2(0.78f, 0.89f), V2(0.7f, 0.77f), V2(0.86f, 0.77f), Solid(IconColor(16, 24, 40)));
    IconCapsule(Canvas, V2(0.78f, 0.78f), V2(0.78f, 0.68f), 0.025f, Solid(IconColor(16, 24, 40)));
}

// NOTE(zoubir): Cataclysm: the Giant Fireball with stun stars spinning
// under it
internal void
PaintCataclysmIcon(icon_canvas *Canvas)
{
    PaintRoleGiantFireballIcon(Canvas);
    for(u32 Star = 0; Star < 3; Star++)
    {
        float Angle = 2.f * Pi32 * (float)Star / 3.f + 0.5f;
        IconSparkle(Canvas, V2(0.5f + 0.2f * Cos(Angle), 0.84f + 0.07f * Sin(Angle)), 0.07f,
                    Solid(IconColor(255, 245, 150)));
    }
}

// NOTE(zoubir): Executioner: an axe over a skull
internal void
PaintExecutionerIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 70, 50, 130));
    IconCircle(Canvas, V2(0.5f, 0.66f), 0.16f, Solid(IconColor(230, 225, 210)));
    IconCircle(Canvas, V2(0.44f, 0.64f), 0.04f, Solid(IconColor(30, 20, 20)));
    IconCircle(Canvas, V2(0.56f, 0.64f), 0.04f, Solid(IconColor(30, 20, 20)));
    IconCapsule(Canvas, V2(0.22f, 0.86f), V2(0.74f, 0.16f), 0.03f, Solid(IconColor(120, 80, 50)));
    v2 Blade[4] = {V2(0.6f, 0.12f), V2(0.86f, 0.22f), V2(0.8f, 0.42f), V2(0.66f, 0.3f)};
    IconPolygon(Canvas, Blade, 4, Gradient(IconColor(240, 240, 250), IconColor(130, 135, 150),
                                           V2(0.6f, 0.12f), V2(0.8f, 0.42f)));
}

internal void PaintProvokeIcon(icon_canvas *C) { PaintRoleTauntIcon(C); IconBadgeSooner(C); }
internal void PaintBastionIcon(icon_canvas *C) { PaintRoleShieldSlamIcon(C); IconBadgeMore(C); }
internal void PaintSwiftMendingIcon(icon_canvas *C) { PaintRoleMendingBoltIcon(C); IconBadgeMore(C); }
internal void PaintDeepWardIcon(icon_canvas *C) { PaintRoleWardIcon(C); IconBadgeMore(C); }
internal void PaintOverloadIcon(icon_canvas *C) { PaintRoleDetonateIcon(C); IconBadgeSooner(C); }
internal void PaintShatterArmorIcon(icon_canvas *C) { PaintRoleShieldSlamIcon(C); IconBadgeMore(C); }
internal void PaintWildfireIcon(icon_canvas *C) { PaintRoleInfernoIcon(C); IconBadgeMore(C); }

// NOTE(zoubir): by player_role, then slot, as RoleTalentDefs lists them;
// a slot that unlocks a spell shows the spell. The later classes' rows
// are their ui/dungeon/classes/<class>_icons.cpp
global_variable talent_icon_painter *StrikerTalentIconPainters[ROLE_TALENTS] =
{
    PaintPyromancerIcon, PaintRoleDetonateIcon, PaintWildfireIcon, PaintExecutionerIcon,
    PaintRoleCombustionIcon, PaintOverloadIcon,
    PaintStatDamageIcon, PaintStatVitalityIcon, PaintMoltenGroundIcon, PaintStatHasteIcon,
    PaintStatArmorIcon, PaintCataclysmIcon,
};
#include "tank_talent_icons.cpp"
global_variable talent_icon_painter *TankTalentIconPainters[ROLE_TALENTS] =
{
    PaintIronSkinIcon, PaintRoleInterceptIcon, PaintProvokeIcon, PaintBastionIcon,
    PaintRoleLastStandIcon, PaintShatterArmorIcon,
    PaintStatVitalityIcon, PaintStatArmorIcon, PaintJuggernautIcon, PaintStatDamageIcon,
    PaintStatHasteIcon, PaintUnbrokenIcon,
};
global_variable talent_icon_painter *HealerTalentIconPainters[ROLE_TALENTS] =
{
    PaintSwiftMendingIcon, PaintRoleSanctuaryIcon, PaintDeepWardIcon, PaintRenewalIcon,
    PaintRoleRadianceIcon, PaintMiracleIcon,
    PaintStatHealingIcon, PaintStatVitalityIcon, PaintSteadfastWardIcon, PaintStatHasteIcon,
    PaintStatSwiftnessIcon, PaintGuardianAngelIcon,
};
global_variable talent_icon_painter **RoleTalentIconPainters[PlayerRole_Count] =
{
    StrikerTalentIconPainters, TankTalentIconPainters, HealerTalentIconPainters,
    RangerTalentIconPainters, BerserkerTalentIconPainters, ShadowbladeTalentIconPainters,
};
