/* Role icons: the ability bar's pictures for the dungeon roles' spells
   (sim/dungeon/role_abilities.cpp), painted with the ability icons'
   canvas (ui/ability_icons/icon_canvas.cpp) into the same atlas, after
   the bar's own slots. The bar shows them on a key a role owns
   (RoleIconIndex). The role talents reuse them (role_talent_icons.cpp). Included by ui/ability_icons/ability_icons.cpp
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

// NOTE(zoubir): Shield Slam: a tower shield driven into the ground, the
// ground cracking and rings of force running out from it
internal void
PaintRoleShieldSlamIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.46f, IconColor(140, 180, 255, 110));
    for(u32 Ring = 0; Ring < 2; Ring++)
    {
        float Radius = 0.3f + 0.12f * (float)Ring;
        IconArc(Canvas, V2(0.5f, 0.8f), Radius, 0.03f,
                Solid(IconColor(220, 230, 255, 220 - 80 * Ring)), Pi32 + 0.35f, 2.f * Pi32 - 0.35f);
    }
    IconCapsule(Canvas, V2(0.14f, 0.84f), V2(0.86f, 0.84f), 0.025f, Solid(IconColor(120, 100, 80)));
    IconCapsule(Canvas, V2(0.5f, 0.84f), V2(0.3f, 0.94f), 0.018f, Solid(IconColor(40, 30, 24)));
    IconCapsule(Canvas, V2(0.5f, 0.84f), V2(0.72f, 0.95f), 0.018f, Solid(IconColor(40, 30, 24)));
    v2 Shield[5] = {V2(0.32f, 0.16f), V2(0.68f, 0.16f), V2(0.68f, 0.62f), V2(0.5f, 0.84f),
                    V2(0.32f, 0.62f)};
    IconPolygon(Canvas, Shield, 5, Gradient(IconColor(235, 238, 245), IconColor(110, 120, 140),
                                            V2(0.5f, 0.16f), V2(0.5f, 0.84f)));
    v2 Field[5] = {V2(0.37f, 0.22f), V2(0.63f, 0.22f), V2(0.63f, 0.6f), V2(0.5f, 0.76f),
                   V2(0.37f, 0.6f)};
    IconPolygon(Canvas, Field, 5, Gradient(IconColor(80, 120, 210), IconColor(30, 50, 110),
                                           V2(0.5f, 0.22f), V2(0.5f, 0.76f)));
    IconCircle(Canvas, V2(0.5f, 0.42f), 0.06f, Solid(IconColor(240, 205, 90)));
    IconSparkle(Canvas, V2(0.78f, 0.26f), 0.07f, Solid(IconColor(255, 255, 255)));
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

// NOTE(zoubir): Inferno: a meteor plunging onto a ring of fire
internal void
PaintRoleInfernoIcon(icon_canvas *Canvas)
{
    v4 Yellow = IconColor(255, 225, 110);
    v4 Orange = IconColor(255, 130, 40);
    v4 Red = IconColor(200, 40, 20);
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.48f, IconColor(255, 110, 30, 140));
    for(u32 Dot = 0; Dot < 14; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / 14.f;
        IconCircle(Canvas, V2(0.5f + 0.32f * Cos(Angle), 0.78f + 0.1f * Sin(Angle)), 0.04f,
                   Solid(Dot % 2 ? Orange : Yellow));
    }
    IconCapsule(Canvas, V2(0.14f, 0.08f), V2(0.5f, 0.62f), 0.1f,
                Gradient(IconColor(255, 120, 30, 0), IconColor(255, 150, 50, 230),
                         V2(0.14f, 0.08f), V2(0.5f, 0.62f)));
    IconCircle(Canvas, V2(0.54f, 0.64f), 0.15f, Gradient(Yellow, Red, V2(0.46f, 0.54f), V2(0.64f, 0.76f)));
    IconCircle(Canvas, V2(0.5f, 0.6f), 0.06f, Solid(IconColor(255, 250, 225)));
}

// NOTE(zoubir): Detonate: a starburst of fire over three Searing flames
internal void
PaintRoleDetonateIcon(icon_canvas *Canvas)
{
    v4 Yellow = IconColor(255, 230, 120);
    v4 Orange = IconColor(255, 120, 30);
    v4 Red = IconColor(190, 30, 20);
    IconGlow(Canvas, V2(0.5f, 0.42f), 0.5f, IconColor(255, 90, 30, 150));
    for(u32 Ray = 0; Ray < 8; Ray++)
    {
        float Angle = 2.f * Pi32 * (float)Ray / 8.f + 0.2f;
        v2 Tip = V2(0.5f + 0.4f * Cos(Angle), 0.42f + 0.34f * Sin(Angle));
        IconCapsule(Canvas, V2(0.5f, 0.42f), Tip, Ray % 2 ? 0.035f : 0.055f,
                    Gradient(Yellow, IconColor(255, 110, 30, 40), V2(0.5f, 0.42f), Tip));
    }
    IconCircle(Canvas, V2(0.5f, 0.42f), 0.16f, Gradient(Yellow, Red, V2(0.44f, 0.34f), V2(0.6f, 0.54f)));
    IconCircle(Canvas, V2(0.5f, 0.42f), 0.07f, Solid(IconColor(255, 250, 230)));
    for(u32 Pip = 0; Pip < 3; Pip++)
    {
        float X = 0.28f + 0.22f * (float)Pip;
        IconCircle(Canvas, V2(X, 0.84f), 0.07f, Gradient(Yellow, Orange, V2(X, 0.78f), V2(X, 0.9f)));
        IconTriangle(Canvas, V2(X - 0.06f, 0.82f), V2(X + 0.06f, 0.82f), V2(X, 0.7f), Solid(Orange));
    }
}

// NOTE(zoubir): Giant Fireball: a huge slow fireball with a long tail
internal void
PaintRoleGiantFireballIcon(icon_canvas *Canvas)
{
    v4 Yellow = IconColor(255, 235, 140);
    v4 Orange = IconColor(255, 120, 30);
    v4 Red = IconColor(180, 30, 15);
    IconGlow(Canvas, V2(0.58f, 0.44f), 0.5f, IconColor(255, 110, 30, 150));
    IconCapsule(Canvas, V2(0.1f, 0.88f), V2(0.5f, 0.52f), 0.16f,
                Gradient(IconColor(255, 90, 20, 0), IconColor(255, 140, 40, 210),
                         V2(0.1f, 0.88f), V2(0.5f, 0.52f)));
    IconCircle(Canvas, V2(0.58f, 0.44f), 0.29f, Gradient(Yellow, Red, V2(0.48f, 0.32f), V2(0.74f, 0.64f)));
    IconCircle(Canvas, V2(0.58f, 0.44f), 0.17f, Gradient(IconColor(255, 250, 220), Orange,
                                                        V2(0.52f, 0.36f), V2(0.66f, 0.54f)));
    IconSparkle(Canvas, V2(0.82f, 0.18f), 0.07f, Solid(IconColor(255, 255, 230)));
}

// NOTE(zoubir): Combustion: a flame rising out of a burning heart
internal void
PaintRoleCombustionIcon(icon_canvas *Canvas)
{
    v4 Yellow = IconColor(255, 235, 130);
    v4 Orange = IconColor(255, 130, 30);
    v4 Red = IconColor(200, 30, 20);
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.5f, IconColor(255, 90, 20, 160));
    IconTriangle(Canvas, V2(0.26f, 0.66f), V2(0.74f, 0.66f), V2(0.5f, 0.08f),
                 Gradient(Yellow, Red, V2(0.5f, 0.08f), V2(0.5f, 0.66f)));
    IconTriangle(Canvas, V2(0.18f, 0.7f), V2(0.42f, 0.7f), V2(0.26f, 0.32f), Solid(Orange));
    IconTriangle(Canvas, V2(0.58f, 0.7f), V2(0.82f, 0.7f), V2(0.74f, 0.32f), Solid(Orange));
    IconCircle(Canvas, V2(0.5f, 0.68f), 0.22f, Gradient(Yellow, Red, V2(0.44f, 0.58f), V2(0.6f, 0.84f)));
    IconCircle(Canvas, V2(0.5f, 0.66f), 0.09f, Solid(IconColor(255, 250, 230)));
}

// NOTE(zoubir): Last Stand: a shield planted in the ground with a green
// cross of healing on its field
internal void
PaintRoleLastStandIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(120, 220, 160, 110));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.4f, 0.04f, Solid(IconColor(200, 220, 255, 200)));
    v2 Shield[5] = {V2(0.3f, 0.14f), V2(0.7f, 0.14f), V2(0.7f, 0.6f), V2(0.5f, 0.86f),
                    V2(0.3f, 0.6f)};
    IconPolygon(Canvas, Shield, 5, Gradient(IconColor(235, 238, 245), IconColor(110, 120, 140),
                                            V2(0.5f, 0.14f), V2(0.5f, 0.86f)));
    v2 Field[5] = {V2(0.35f, 0.2f), V2(0.65f, 0.2f), V2(0.65f, 0.58f), V2(0.5f, 0.78f),
                   V2(0.35f, 0.58f)};
    IconPolygon(Canvas, Field, 5, Gradient(IconColor(80, 120, 210), IconColor(30, 50, 110),
                                           V2(0.5f, 0.2f), V2(0.5f, 0.78f)));
    IconCapsule(Canvas, V2(0.5f, 0.3f), V2(0.5f, 0.6f), 0.05f, Solid(IconColor(120, 240, 150)));
    IconCapsule(Canvas, V2(0.36f, 0.44f), V2(0.64f, 0.44f), 0.05f, Solid(IconColor(120, 240, 150)));
}

// NOTE(zoubir): Radiance: a gold sun with rays reaching three small allies
internal void
PaintRoleRadianceIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 220, 110);
    v4 Pale = IconColor(255, 250, 220);
    IconGlow(Canvas, V2(0.5f, 0.42f), 0.5f, IconColor(255, 220, 120, 150));
    for(u32 Ray = 0; Ray < 12; Ray++)
    {
        float Angle = 2.f * Pi32 * (float)Ray / 12.f;
        v2 Tip = V2(0.5f + 0.32f * Cos(Angle), 0.42f + 0.32f * Sin(Angle));
        IconCapsule(Canvas, V2(0.5f, 0.42f), Tip, 0.025f,
                    Gradient(Gold, IconColor(255, 220, 110, 30), V2(0.5f, 0.42f), Tip));
    }
    IconCircle(Canvas, V2(0.5f, 0.42f), 0.15f, Gradient(Pale, Gold, V2(0.45f, 0.36f), V2(0.58f, 0.5f)));
    for(u32 Ally = 0; Ally < 3; Ally++)
    {
        float X = 0.24f + 0.26f * (float)Ally;
        IconCircle(Canvas, V2(X, 0.76f), 0.05f, Solid(IconColor(140, 240, 160)));
        IconCapsule(Canvas, V2(X, 0.82f), V2(X, 0.92f), 0.045f, Solid(IconColor(110, 210, 140)));
    }
}

// NOTE(zoubir): Shield Throw: a round shield spinning through the air,
// its path bending from one foe to the next
internal void
PaintRoleShieldThrowIcon(icon_canvas *Canvas)
{
    v4 Trail = IconColor(170, 200, 255);
    IconGlow(Canvas, V2(0.56f, 0.44f), 0.44f, IconColor(140, 180, 255, 100));
    IconCapsule(Canvas, V2(0.12f, 0.84f), V2(0.34f, 0.58f), 0.03f,
                Gradient(IconColor(170, 200, 255, 0), Trail, V2(0.12f, 0.84f), V2(0.34f, 0.58f)));
    IconCapsule(Canvas, V2(0.68f, 0.34f), V2(0.86f, 0.62f), 0.025f,
                Gradient(Trail, IconColor(170, 200, 255, 0), V2(0.68f, 0.34f), V2(0.86f, 0.62f)));
    IconCircle(Canvas, V2(0.88f, 0.66f), 0.05f, Solid(IconColor(255, 120, 90)));
    IconCircle(Canvas, V2(0.52f, 0.44f), 0.22f,
               Gradient(IconColor(235, 238, 245), IconColor(110, 120, 140), V2(0.42f, 0.3f), V2(0.62f, 0.6f)));
    IconCircle(Canvas, V2(0.52f, 0.44f), 0.16f,
               Gradient(IconColor(80, 120, 210), IconColor(30, 50, 110), V2(0.46f, 0.34f), V2(0.6f, 0.56f)));
    IconCircle(Canvas, V2(0.52f, 0.44f), 0.06f, Solid(IconColor(240, 205, 90)));
    IconArc(Canvas, V2(0.52f, 0.44f), 0.28f, 0.02f, Solid(IconColor(220, 230, 255, 160)), 3.6f, 5.4f);
}

// NOTE(zoubir): Holy Fire: a column of golden light striking down,
// flames curling where it lands
internal void
PaintRoleHolyFireIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 215, 110);
    v4 Pale = IconColor(255, 250, 225);
    IconGlow(Canvas, V2(0.5f, 0.62f), 0.46f, IconColor(255, 200, 90, 120));
    v2 Beam[4] = {V2(0.4f, 0.1f), V2(0.6f, 0.1f), V2(0.56f, 0.7f), V2(0.44f, 0.7f)};
    IconPolygon(Canvas, Beam, 4,
                Gradient(IconColor(255, 250, 225, 40), Pale, V2(0.5f, 0.1f), V2(0.5f, 0.7f)));
    IconTriangle(Canvas, V2(0.26f, 0.84f), V2(0.4f, 0.84f), V2(0.3f, 0.6f),
                 Gradient(Gold, IconColor(255, 140, 60), V2(0.3f, 0.6f), V2(0.33f, 0.84f)));
    IconTriangle(Canvas, V2(0.6f, 0.84f), V2(0.74f, 0.84f), V2(0.7f, 0.6f),
                 Gradient(Gold, IconColor(255, 140, 60), V2(0.7f, 0.6f), V2(0.67f, 0.84f)));
    IconTriangle(Canvas, V2(0.38f, 0.86f), V2(0.62f, 0.86f), V2(0.5f, 0.52f),
                 Gradient(Pale, Gold, V2(0.5f, 0.52f), V2(0.5f, 0.86f)));
    IconCircle(Canvas, V2(0.5f, 0.74f), 0.07f, Solid(IconColor(255, 255, 245)));
    IconSparkle(Canvas, V2(0.74f, 0.26f), 0.07f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Shield Bash: the kite shield driven forward, its rim
// leading, with a burst of force off its face
internal void
PaintRoleShieldBashIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.62f, 0.46f), 0.42f, IconColor(140, 180, 255, 110));
    // NOTE(zoubir): the push behind it, three streaks
    for(u32 Streak = 0; Streak < 3; Streak++)
    {
        float Y = 0.32f + 0.14f * (float)Streak;
        IconCapsule(Canvas, V2(0.08f, Y + 0.04f), V2(0.34f, Y), 0.022f,
                    Gradient(IconColor(170, 200, 255, 0), IconColor(200, 220, 255, 200),
                             V2(0.08f, Y), V2(0.34f, Y)));
    }
    v2 Rim[5] = {V2(0.32f, 0.16f), V2(0.66f, 0.16f), V2(0.66f, 0.52f), V2(0.49f, 0.86f),
                 V2(0.32f, 0.52f)};
    IconPolygon(Canvas, Rim, 5, Gradient(IconColor(235, 238, 245), IconColor(110, 120, 140),
                                         V2(0.36f, 0.18f), V2(0.6f, 0.8f)));
    v2 Field[5] = {V2(0.37f, 0.22f), V2(0.61f, 0.22f), V2(0.61f, 0.5f), V2(0.49f, 0.76f),
                   V2(0.37f, 0.5f)};
    IconPolygon(Canvas, Field, 5, Gradient(IconColor(80, 120, 210), IconColor(30, 50, 110),
                                           V2(0.42f, 0.24f), V2(0.56f, 0.7f)));
    IconCircle(Canvas, V2(0.49f, 0.42f), 0.06f, Solid(IconColor(240, 205, 90)));
    // NOTE(zoubir): the blow landing off its face
    IconArc(Canvas, V2(0.66f, 0.44f), 0.16f, 0.024f, Solid(IconColor(255, 245, 220, 230)), -1.1f, 1.1f);
    IconArc(Canvas, V2(0.66f, 0.44f), 0.24f, 0.016f, Solid(IconColor(220, 230, 255, 140)), -0.9f, 0.9f);
    IconSparkle(Canvas, V2(0.86f, 0.3f), 0.06f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Smite Bolt: a small bolt of gold light flying, a pale
// trail behind it
internal void
PaintRoleSmiteBoltIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 215, 110);
    IconGlow(Canvas, V2(0.64f, 0.38f), 0.36f, IconColor(255, 200, 90, 130));
    IconCapsule(Canvas, V2(0.14f, 0.84f), V2(0.6f, 0.42f), 0.05f,
                Gradient(IconColor(255, 250, 225, 0), IconColor(255, 245, 210, 210),
                         V2(0.14f, 0.84f), V2(0.6f, 0.42f)));
    IconCapsule(Canvas, V2(0.26f, 0.8f), V2(0.6f, 0.46f), 0.02f,
                Gradient(IconColor(255, 255, 255, 0), IconColor(255, 255, 255, 240),
                         V2(0.26f, 0.8f), V2(0.6f, 0.46f)));
    IconCircle(Canvas, V2(0.66f, 0.36f), 0.13f,
               Gradient(IconColor(255, 255, 240), Gold, V2(0.62f, 0.3f), V2(0.72f, 0.44f)));
    IconCircle(Canvas, V2(0.64f, 0.34f), 0.05f, Solid(IconColor(255, 255, 255)));
    IconSparkle(Canvas, V2(0.84f, 0.18f), 0.07f, Solid(IconColor(255, 255, 255)));
    IconSparkle(Canvas, V2(0.4f, 0.7f), 0.04f, Solid(IconColor(255, 240, 200)));
}

typedef void role_icon_painter(icon_canvas *Canvas);
typedef void talent_icon_painter(icon_canvas *Canvas);

#include "classes/class_icons.cpp"

// NOTE(zoubir): the painters in RoleSpells' order (RoleKeys order: A, R,
// C, V, W, X, right click), by player_role; 0 for a key the class has no
// spell on
global_variable role_icon_painter *StrikerIconPainters[ROLE_KEYS] =
{
    PaintRoleInfernoIcon, PaintRoleGiantFireballIcon, PaintRoleDetonateIcon, PaintRoleCombustionIcon,
};
global_variable role_icon_painter *TankIconPainters[ROLE_KEYS] =
{
    PaintRoleTauntIcon, PaintRoleShieldSlamIcon, PaintRoleInterceptIcon, PaintRoleLastStandIcon,
    PaintRoleShieldThrowIcon, 0, PaintRoleShieldBashIcon,
};
global_variable role_icon_painter *HealerIconPainters[ROLE_KEYS] =
{
    PaintRoleMendingBoltIcon, PaintRoleWardIcon, PaintRoleSanctuaryIcon, PaintRoleRadianceIcon,
    PaintRoleHolyFireIcon, 0, PaintRoleSmiteBoltIcon,
};
global_variable role_icon_painter **RoleIconPainters[PlayerRole_Count] =
{
    StrikerIconPainters, TankIconPainters, HealerIconPainters,
    RangerIconPainters, BerserkerIconPainters, ShadowbladeIconPainters,
};
#define ROLE_ICON_COUNT (PlayerRole_Count * ROLE_KEYS)

// NOTE(zoubir): the painter of atlas cell Cell of the role icons
inline role_icon_painter *
RoleIconPainterAt(u32 Cell)
{
    role_icon_painter *Result = Cell < ROLE_ICON_COUNT ?
        RoleIconPainters[Cell / ROLE_KEYS][Cell % ROLE_KEYS] : 0;
    return Result;
}

// NOTE(zoubir): the icon for Role's Key, as an index into RoleIconPainters,
// or ROLE_ICON_COUNT for none
inline u32
RoleIconIndex(u32 Role, u32 Key)
{
    u32 Result = (Role < PlayerRole_Count && Key < ROLE_KEYS) ? Role * ROLE_KEYS + Key :
        ROLE_ICON_COUNT;
    return Result;
}
