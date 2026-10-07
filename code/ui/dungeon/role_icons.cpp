/* Role icons: the ability bar's pictures for the dungeon roles' spells
   (sim/dungeon/role_abilities.cpp), painted with the ability icons'
   canvas (ui/ability_icons/icon_canvas.cpp) into the same atlas, after
   the bar's own slots. The bar shows them on a key a tank's or healer's
   role owns (RoleIconCell). Included by ui/ability_icons/ability_icons.cpp
   after the canvas. */

// NOTE(zoubir): Taunt: a horned helm roaring red rings outward
internal void
PaintRoleTauntIcon(icon_canvas *Canvas)
{
    v4 Steel = IconColor(200, 205, 215);
    v4 Dark = IconColor(70, 75, 90);
    v4 Red = IconColor(255, 90, 70);
    IconGlow(Canvas, V2(0.45f, 0.5f), 0.45f, IconColor(255, 70, 50, 90));
    for(u32 Ring = 0; Ring < 3; Ring++)
    {
        float Radius = 0.22f + 0.1f * (float)Ring;
        IconArc(Canvas, V2(0.42f, 0.52f), Radius, 0.03f,
                Solid(IconColor(255, 110 - 25 * Ring, 80, 230 - 50 * Ring)), -0.7f, 0.7f);
    }
    IconCircle(Canvas, V2(0.36f, 0.48f), 0.2f,
               Gradient(Steel, Dark, V2(0.3f, 0.3f), V2(0.4f, 0.7f)));
    v2 Visor[4] = {V2(0.16f, 0.5f), V2(0.56f, 0.5f), V2(0.52f, 0.72f), V2(0.2f, 0.72f)};
    IconPolygon(Canvas, Visor, 4, Gradient(Steel, Dark, V2(0.3f, 0.5f), V2(0.3f, 0.72f)));
    IconCapsule(Canvas, V2(0.24f, 0.56f), V2(0.48f, 0.56f), 0.03f, Solid(IconColor(20, 20, 26)));
    IconTriangle(Canvas, V2(0.2f, 0.36f), V2(0.27f, 0.32f), V2(0.12f, 0.16f),
                 Gradient(IconColor(250, 240, 220), Steel, V2(0.12f, 0.16f), V2(0.24f, 0.34f)));
    IconTriangle(Canvas, V2(0.44f, 0.32f), V2(0.51f, 0.36f), V2(0.6f, 0.16f),
                 Gradient(IconColor(250, 240, 220), Steel, V2(0.6f, 0.16f), V2(0.48f, 0.34f)));
    IconSparkle(Canvas, V2(0.78f, 0.3f), 0.06f, Solid(Red));
}

// NOTE(zoubir): Shield Wall: three tower shields side by side
internal void
PaintRoleShieldWallIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.52f), 0.46f, IconColor(140, 180, 255, 100));
    for(u32 Shield = 0; Shield < 3; Shield++)
    {
        float X = 0.24f + 0.26f * (float)Shield;
        float Lift = (Shield == 1) ? -0.05f : 0.f;
        v2 Points[5] = {V2(X - 0.13f, 0.24f + Lift), V2(X + 0.13f, 0.24f + Lift),
                        V2(X + 0.13f, 0.66f + Lift), V2(X, 0.82f + Lift),
                        V2(X - 0.13f, 0.66f + Lift)};
        IconPolygon(Canvas, Points, 5, Gradient(IconColor(225, 230, 240), IconColor(110, 120, 140),
                                                V2(X, 0.24f), V2(X, 0.82f)));
        v2 Inner[5] = {V2(X - 0.09f, 0.29f + Lift), V2(X + 0.09f, 0.29f + Lift),
                       V2(X + 0.09f, 0.63f + Lift), V2(X, 0.75f + Lift),
                       V2(X - 0.09f, 0.63f + Lift)};
        IconPolygon(Canvas, Inner, 5, Gradient(IconColor(80, 110, 190), IconColor(30, 45, 100),
                                               V2(X, 0.29f), V2(X, 0.75f)));
        IconCircle(Canvas, V2(X, 0.47f + Lift), 0.035f, Solid(IconColor(235, 200, 90)));
    }
}

// NOTE(zoubir): Intercept: a leap arcing over to a shield raised before an
// ally
internal void
PaintRoleInterceptIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 210, 100);
    IconGlow(Canvas, V2(0.6f, 0.45f), 0.42f, IconColor(255, 200, 90, 90));
    IconArc(Canvas, V2(0.48f, 0.72f), 0.34f, 0.045f,
            Gradient(IconColor(255, 230, 160, 60), Gold, V2(0.14f, 0.7f), V2(0.8f, 0.4f)),
            3.4f, 5.9f);
    IconTriangle(Canvas, V2(0.74f, 0.3f), V2(0.88f, 0.42f), V2(0.7f, 0.46f), Solid(Gold));
    // NOTE(zoubir): the ally, small, behind the shield
    IconCircle(Canvas, V2(0.78f, 0.6f), 0.06f, Solid(IconColor(150, 210, 150)));
    IconCapsule(Canvas, V2(0.78f, 0.68f), V2(0.78f, 0.84f), 0.05f, Solid(IconColor(110, 170, 110)));
    v2 Shield[5] = {V2(0.52f, 0.52f), V2(0.7f, 0.52f), V2(0.7f, 0.72f), V2(0.61f, 0.86f),
                    V2(0.52f, 0.72f)};
    IconPolygon(Canvas, Shield, 5, Gradient(IconColor(210, 220, 235), IconColor(90, 110, 150),
                                            V2(0.61f, 0.52f), V2(0.61f, 0.86f)));
}

// NOTE(zoubir): Sanctuary: a golden ring on the ground with light rising
// out of it
internal void
PaintRoleSanctuaryIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 225, 120);
    v4 Green = IconColor(140, 240, 180);
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.46f, IconColor(130, 240, 190, 110));
    v2 Beam[4] = {V2(0.36f, 0.72f), V2(0.64f, 0.72f), V2(0.58f, 0.16f), V2(0.42f, 0.16f)};
    IconPolygon(Canvas, Beam, 4,
                Gradient(IconColor(200, 255, 220, 200), IconColor(200, 255, 220, 0),
                         V2(0.5f, 0.72f), V2(0.5f, 0.16f)));
    for(u32 Dot = 0; Dot < 12; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / 12.f;
        IconCircle(Canvas, V2(0.5f + 0.3f * Cos(Angle), 0.74f + 0.1f * Sin(Angle)), 0.03f,
                   Solid(Gold));
    }
    IconCapsule(Canvas, V2(0.5f, 0.3f), V2(0.5f, 0.56f), 0.035f, Solid(Green));
    IconCapsule(Canvas, V2(0.4f, 0.4f), V2(0.6f, 0.4f), 0.035f, Solid(Green));
    IconSparkle(Canvas, V2(0.68f, 0.26f), 0.06f, Solid(IconColor(255, 255, 240)));
}

// NOTE(zoubir): Ward: a blue hexagon of light closed round a small figure
internal void
PaintRoleWardIcon(icon_canvas *Canvas)
{
    v4 Blue = IconColor(130, 200, 255);
    v2 Centre = V2(0.5f, 0.52f);
    IconGlow(Canvas, Centre, 0.46f, IconColor(90, 170, 255, 120));
    v2 Hex[6];
    for(u32 Side = 0; Side < 6; Side++)
    {
        float Angle = Pi32 / 6.f + 2.f * Pi32 * (float)Side / 6.f;
        Hex[Side] = Centre + 0.34f * V2(Cos(Angle), Sin(Angle));
    }
    IconPolygon(Canvas, Hex, 6, Gradient(IconColor(140, 200, 255, 90), IconColor(60, 120, 230, 60),
                                         V2(0.5f, 0.2f), V2(0.5f, 0.85f)));
    for(u32 Side = 0; Side < 6; Side++)
    {
        IconCapsule(Canvas, Hex[Side], Hex[(Side + 1) % 6], 0.025f, Solid(Blue));
    }
    IconCircle(Canvas, V2(0.5f, 0.42f), 0.07f, Solid(IconColor(240, 245, 255)));
    IconCapsule(Canvas, V2(0.5f, 0.52f), V2(0.5f, 0.68f), 0.06f, Solid(IconColor(220, 230, 250)));
}

// NOTE(zoubir): Mending Bolt: a green orb with a white cross flying in
internal void
PaintRoleMendingBoltIcon(icon_canvas *Canvas)
{
    v4 Green = IconColor(110, 235, 140);
    v4 Pale = IconColor(230, 255, 235);
    IconGlow(Canvas, V2(0.58f, 0.44f), 0.44f, IconColor(100, 240, 140, 120));
    IconCapsule(Canvas, V2(0.16f, 0.82f), V2(0.5f, 0.5f), 0.07f,
                Gradient(IconColor(110, 235, 140, 0), IconColor(110, 235, 140, 200),
                         V2(0.16f, 0.82f), V2(0.5f, 0.5f)));
    IconCircle(Canvas, V2(0.6f, 0.42f), 0.2f, Gradient(Pale, Green, V2(0.52f, 0.32f), V2(0.7f, 0.56f)));
    IconCapsule(Canvas, V2(0.6f, 0.31f), V2(0.6f, 0.53f), 0.04f, Solid(IconColor(255, 255, 255)));
    IconCapsule(Canvas, V2(0.49f, 0.42f), V2(0.71f, 0.42f), 0.04f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): the painters in RoleSpells' order (tank then healer, each
// key A, E, V); the damage role has none
#define ROLE_ICON_COUNT 6
typedef void role_icon_painter(icon_canvas *Canvas);
global_variable role_icon_painter *RoleIconPainters[ROLE_ICON_COUNT] =
{
    PaintRoleTauntIcon, PaintRoleShieldWallIcon, PaintRoleInterceptIcon,
    PaintRoleSanctuaryIcon, PaintRoleWardIcon, PaintRoleMendingBoltIcon,
};

// NOTE(zoubir): the icon for Role's Key, as an index into RoleIconPainters,
// or ROLE_ICON_COUNT for none
inline u32
RoleIconIndex(u32 Role, u32 Key)
{
    u32 Result = ROLE_ICON_COUNT;
    if (Role == PlayerRole_Tank && Key < ROLE_KEYS)
    {
        Result = Key;
    }
    else if (Role == PlayerRole_Healer && Key < ROLE_KEYS)
    {
        Result = ROLE_KEYS + Key;
    }
    return Result;
}
