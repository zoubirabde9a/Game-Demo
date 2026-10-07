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

// NOTE(zoubir): Rally: three small shields under one big ring of light
internal void
PaintRallyIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.46f, IconColor(255, 220, 120, 110));
    IconArc(Canvas, V2(0.5f, 0.58f), 0.36f, 0.035f, Solid(IconColor(255, 225, 130)));
    for(u32 Shield = 0; Shield < 3; Shield++)
    {
        float X = 0.3f + 0.2f * (float)Shield;
        float Y = Shield == 1 ? 0.42f : 0.56f;
        v2 Points[5] = {V2(X - 0.08f, Y - 0.1f), V2(X + 0.08f, Y - 0.1f), V2(X + 0.08f, Y + 0.06f),
                        V2(X, Y + 0.15f), V2(X - 0.08f, Y + 0.06f)};
        IconPolygon(Canvas, Points, 5, Gradient(IconColor(220, 230, 245), IconColor(80, 110, 180),
                                                V2(X, Y - 0.1f), V2(X, Y + 0.15f)));
    }
}

// NOTE(zoubir): Unyielding: a cracked shield still standing, red behind it
internal void
PaintUnyieldingIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(255, 60, 40, 130));
    v2 Shield[5] = {V2(0.28f, 0.16f), V2(0.72f, 0.16f), V2(0.72f, 0.6f), V2(0.5f, 0.86f),
                    V2(0.28f, 0.6f)};
    IconPolygon(Canvas, Shield, 5, Gradient(IconColor(200, 205, 215), IconColor(80, 80, 95),
                                            V2(0.5f, 0.16f), V2(0.5f, 0.86f)));
    IconCapsule(Canvas, V2(0.46f, 0.18f), V2(0.54f, 0.38f), 0.02f, Solid(IconColor(30, 20, 20)));
    IconCapsule(Canvas, V2(0.54f, 0.38f), V2(0.44f, 0.56f), 0.02f, Solid(IconColor(30, 20, 20)));
    IconCapsule(Canvas, V2(0.44f, 0.56f), V2(0.52f, 0.78f), 0.02f, Solid(IconColor(30, 20, 20)));
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

// NOTE(zoubir): Beacon: one bolt of light splitting into two
internal void
PaintBeaconIcon(icon_canvas *Canvas)
{
    v4 Green = IconColor(120, 240, 150);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(110, 240, 150, 110));
    IconCapsule(Canvas, V2(0.14f, 0.5f), V2(0.46f, 0.5f), 0.05f, Solid(Green));
    IconCapsule(Canvas, V2(0.46f, 0.5f), V2(0.78f, 0.26f), 0.04f, Solid(Green));
    IconCapsule(Canvas, V2(0.46f, 0.5f), V2(0.78f, 0.74f), 0.03f, Solid(IconColor(180, 255, 200)));
    IconCircle(Canvas, V2(0.8f, 0.26f), 0.09f, Solid(IconColor(235, 255, 240)));
    IconCircle(Canvas, V2(0.8f, 0.74f), 0.07f, Solid(IconColor(200, 255, 215)));
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

// NOTE(zoubir): Bloodlust: a red drop feeding a flame
internal void
PaintBloodlustIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 50, 50, 140));
    v2 Drop[5] = {V2(0.5f, 0.12f), V2(0.66f, 0.4f), V2(0.62f, 0.56f), V2(0.38f, 0.56f), V2(0.34f, 0.4f)};
    IconPolygon(Canvas, Drop, 5, Gradient(IconColor(255, 120, 110), IconColor(150, 10, 20),
                                          V2(0.45f, 0.15f), V2(0.55f, 0.56f)));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.12f, Solid(IconColor(180, 20, 30)));
    IconSparkle(Canvas, V2(0.72f, 0.74f), 0.1f, Solid(IconColor(255, 170, 60)));
    IconSparkle(Canvas, V2(0.3f, 0.78f), 0.07f, Solid(IconColor(255, 120, 40)));
}

internal void PaintProvokeIcon(icon_canvas *C) { PaintRoleTauntIcon(C); IconBadgeSooner(C); }
internal void PaintBastionIcon(icon_canvas *C) { PaintRoleShieldSlamIcon(C); IconBadgeMore(C); }
internal void PaintGuardianIcon(icon_canvas *C) { PaintRoleInterceptIcon(C); IconBadgeMore(C); }
internal void PaintSwiftMendingIcon(icon_canvas *C) { PaintRoleMendingBoltIcon(C); IconBadgeMore(C); }
internal void PaintDeepWardIcon(icon_canvas *C) { PaintRoleWardIcon(C); IconBadgeMore(C); }
internal void PaintHallowedIcon(icon_canvas *C) { PaintRoleSanctuaryIcon(C); IconBadgeWider(C); }
internal void PaintKindlingIcon(icon_canvas *C) { PaintRoleInfernoIcon(C); IconBadgeSooner(C); }
internal void PaintWildfireIcon(icon_canvas *C) { PaintRoleInfernoIcon(C); IconBadgeMore(C); }
internal void PaintCataclysmIcon(icon_canvas *C) { PaintRoleInfernoIcon(C); IconBadgeWider(C); }

// NOTE(zoubir): by player_role (Damage, Tank, Healer), then slot, as
// RoleTalentDefs lists them
global_variable talent_icon_painter *RoleTalentIconPainters[PlayerRole_Count][ROLE_TALENTS] =
{
    {PaintPyromancerIcon, PaintKindlingIcon, PaintWildfireIcon, PaintExecutionerIcon,
     PaintCataclysmIcon, PaintBloodlustIcon},
    {PaintIronSkinIcon, PaintProvokeIcon, PaintBastionIcon, PaintGuardianIcon,
     PaintRallyIcon, PaintUnyieldingIcon},
    {PaintSwiftMendingIcon, PaintDeepWardIcon, PaintRenewalIcon, PaintHallowedIcon,
     PaintBeaconIcon, PaintMiracleIcon},
};
