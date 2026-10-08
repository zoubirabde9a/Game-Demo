/* Druid icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Druid's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). Every one is built from the same few things (an
   olive leaf, a pink five-petal flower, a gold star, a silver moon, bark
   brown) on the class's green light, so the set reads as one kit; the
   stat talents use the shared stat icons. */

// NOTE(zoubir): a leaf from Stem to Tip, Wide across at its widest
internal void
PaintDruidLeaf(icon_canvas *Canvas, v2 Stem, v2 Tip, float Wide, v4 Light, v4 Dark)
{
    v2 Dir = NormalizeOr(Tip - Stem, V2(0.f, -1.f));
    v2 Side = V2(-Dir.Y, Dir.X);
    float Long = Length(Tip - Stem);
    v2 Points[8];
    for(u32 Index = 0; Index < 4; Index++)
    {
        float Along = (float)(Index + 1) / 5.f;
        float Bulge = Wide * Sin(Pi32 * Along) * (1.f - 0.25f * Along);
        Points[Index] = Stem + (Along * Long) * Dir + Bulge * Side;
        Points[7 - Index] = Stem + (Along * Long) * Dir - Bulge * Side;
    }
    v2 Outline[10];
    Outline[0] = Stem;
    for(u32 Index = 0; Index < 4; Index++)
    {
        Outline[1 + Index] = Points[Index];
    }
    Outline[5] = Tip;
    for(u32 Index = 4; Index < 8; Index++)
    {
        Outline[2 + Index] = Points[Index];
    }
    IconPolygon(Canvas, Outline, 10, Gradient(Light, Dark, Stem + 0.5f * Wide * Side, Tip - 0.5f * Wide * Side));
    IconCapsule(Canvas, Stem - 0.12f * Long * Dir, Tip - 0.1f * Long * Dir, 0.012f,
                Solid(IconColor(235, 250, 200, 200)));
}

// NOTE(zoubir): the class's own leaf, olive
inline void
PaintDruidOliveLeaf(icon_canvas *Canvas, v2 Stem, v2 Tip, float Wide)
{
    PaintDruidLeaf(Canvas, Stem, Tip, Wide, IconColor(200, 235, 100), IconColor(70, 115, 30));
}

// NOTE(zoubir): a five-petal flower at C, R across
internal void
PaintDruidFlower(icon_canvas *Canvas, v2 C, float R, float Spin)
{
    for(u32 Petal = 0; Petal < 5; Petal++)
    {
        float A = Spin + 2.f * Pi32 * (float)Petal / 5.f;
        v2 Dir = V2(Cos(A), Sin(A));
        IconCircle(Canvas, C + 0.55f * R * Dir, 0.42f * R,
                   Gradient(IconColor(255, 215, 230), IconColor(225, 95, 150), C, C + R * Dir));
    }
    IconCircle(Canvas, C, 0.3f * R, Gradient(IconColor(255, 245, 170), IconColor(240, 180, 60),
                                             C - V2(0.2f * R, 0.2f * R), C + V2(0.2f * R, 0.2f * R)));
}

// NOTE(zoubir): a crescent moon at C, R across, horns to the right
internal void
PaintDruidMoon(icon_canvas *Canvas, v2 C, float R)
{
    IconArc(Canvas, C, 0.7f * R, 0.55f * R,
            Gradient(IconColor(250, 252, 255), IconColor(150, 175, 230), C - V2(R, R), C + V2(R, R)),
            0.55f * Pi32, 1.95f * Pi32);
}

// NOTE(zoubir): a gnarled staff from Foot to Head, a glowing seed in its head
internal void
PaintDruidStaff(icon_canvas *Canvas, v2 Foot, v2 Head, v4 SeedColor)
{
    v2 Dir = NormalizeOr(Head - Foot, V2(0.f, -1.f));
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Last = Foot;
    for(u32 Knot = 1; Knot <= 4; Knot++)
    {
        float Along = (float)Knot / 4.f;
        v2 P = Foot + Along * (Head - Foot) + (0.03f * Sin(6.f * Along)) * Side;
        IconCapsule(Canvas, Last, P, 0.035f - 0.004f * (float)Knot,
                    Gradient(IconColor(160, 115, 70), IconColor(80, 52, 30), Last + 0.03f * Side, Last - 0.03f * Side));
        Last = P;
    }
    IconArc(Canvas, Head + 0.07f * Dir, 0.07f, 0.03f, Solid(IconColor(110, 75, 45)), 0.f, 0.f);
    IconGlow(Canvas, Head + 0.07f * Dir, 0.16f, SeedColor);
    IconCircle(Canvas, Head + 0.07f * Dir, 0.035f, Solid(IconColor(255, 255, 240)));
}

// NOTE(zoubir): Rejuvenation: leaves spiralling round a soft green heart
internal void
PaintDruidRejuvenationIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.52f), 0.5f, IconColor(150, 220, 90, 150));
    IconArc(Canvas, V2(0.5f, 0.52f), 0.3f, 0.03f, Solid(IconColor(200, 245, 150, 160)), 0.3f, 1.7f * Pi32);
    for(u32 Leaf = 0; Leaf < 4; Leaf++)
    {
        float A = 2.f * Pi32 * (float)Leaf / 4.f + 0.4f;
        v2 At = V2(0.5f, 0.52f) + 0.3f * V2(Cos(A), Sin(A));
        v2 Along = V2(-Sin(A), Cos(A));
        PaintDruidOliveLeaf(Canvas, At - 0.1f * Along, At + 0.14f * Along, 0.06f);
    }
    IconSparkle(Canvas, V2(0.5f, 0.52f), 0.12f, Solid(IconColor(240, 255, 210)));
}

// NOTE(zoubir): Starfire: a gold star falling, a trail of light behind it
internal void
PaintDruidStarfireIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.62f, 0.62f), 0.5f, IconColor(255, 210, 110, 150));
    IconCapsule(Canvas, V2(0.12f, 0.12f), V2(0.56f, 0.56f), 0.07f,
                Gradient(IconColor(255, 220, 140, 0), IconColor(255, 230, 160, 220), V2(0.12f, 0.12f), V2(0.56f, 0.56f)));
    IconSparkle(Canvas, V2(0.62f, 0.62f), 0.3f, Gradient(IconColor(255, 255, 240), IconColor(255, 190, 70),
                                                         V2(0.5f, 0.5f), V2(0.8f, 0.8f)));
    IconCircle(Canvas, V2(0.62f, 0.62f), 0.06f, Solid(IconColor(255, 255, 255)));
    IconSparkle(Canvas, V2(0.28f, 0.74f), 0.07f, Solid(IconColor(255, 240, 200)));
}

// NOTE(zoubir): Entangling Roots: vines rising out of the ground, curling
internal void
PaintDruidRootsIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.8f), 0.45f, IconColor(110, 170, 60, 140));
    for(u32 Bead = 0; Bead < 14; Bead++)
    {
        float A = 2.f * Pi32 * (float)Bead / 14.f;
        IconCircle(Canvas, V2(0.5f + 0.38f * Cos(A), 0.84f + 0.1f * Sin(A)), 0.025f,
                   Solid(IconColor(90, 140, 45, 200)));
    }
    for(u32 Vine = 0; Vine < 3; Vine++)
    {
        float X = 0.28f + 0.22f * (float)Vine;
        v2 Last = V2(X, 0.86f);
        for(u32 Step = 1; Step <= 5; Step++)
        {
            float Up = (float)Step / 5.f;
            v2 P = V2(X + 0.07f * Sin(5.f * Up + (float)Vine), 0.86f - (0.55f + 0.1f * (float)(Vine % 2)) * Up);
            IconCapsule(Canvas, Last, P, 0.04f * (1.1f - 0.6f * Up),
                        Gradient(IconColor(140, 105, 60), IconColor(70, 110, 35), Last, P));
            Last = P;
        }
        PaintDruidOliveLeaf(Canvas, Last, Last + V2(Vine == 1 ? -0.12f : 0.12f, -0.06f), 0.04f);
    }
}

// NOTE(zoubir): Tranquility: rings of green light round a raised staff
internal void
PaintDruidTranquilityIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.55f, IconColor(150, 220, 90, 130));
    for(u32 Ring = 0; Ring < 3; Ring++)
    {
        float R = 0.16f + 0.13f * (float)Ring;
        IconArc(Canvas, V2(0.5f, 0.78f), R, 0.025f, Solid(IconColor(200, 245, 140, 220 - 60 * Ring)), Pi32 + 0.2f,
                2.f * Pi32 - 0.2f);
    }
    PaintDruidStaff(Canvas, V2(0.5f, 0.92f), V2(0.5f, 0.16f), IconColor(200, 255, 140, 220));
    for(u32 Mote = 0; Mote < 5; Mote++)
    {
        float X = 0.18f + 0.16f * (float)Mote;
        IconCircle(Canvas, V2(X, 0.4f + 0.1f * Sin(2.f * (float)Mote)), 0.022f, Solid(IconColor(230, 255, 200)));
    }
}

// NOTE(zoubir): Regrowth: a flower opening over a pair of leaves
internal void
PaintDruidRegrowthIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.45f), 0.5f, IconColor(240, 140, 180, 120));
    IconCapsule(Canvas, V2(0.5f, 0.92f), V2(0.5f, 0.5f), 0.025f, Solid(IconColor(90, 140, 45)));
    PaintDruidOliveLeaf(Canvas, V2(0.5f, 0.78f), V2(0.2f, 0.62f), 0.08f);
    PaintDruidOliveLeaf(Canvas, V2(0.5f, 0.72f), V2(0.8f, 0.56f), 0.08f);
    PaintDruidFlower(Canvas, V2(0.5f, 0.38f), 0.26f, -0.5f * Pi32);
    IconSparkle(Canvas, V2(0.8f, 0.2f), 0.08f, Solid(IconColor(255, 230, 240)));
}

// NOTE(zoubir): Moonfire: a big crescent moon, its light pouring down on
// a burning spot below
internal void
PaintDruidMoonfireIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.4f, 0.36f), 0.5f, IconColor(150, 180, 255, 150));
    IconCapsule(Canvas, V2(0.52f, 0.46f), V2(0.74f, 0.84f), 0.08f,
                Gradient(IconColor(200, 215, 255, 40), IconColor(225, 235, 255, 210), V2(0.52f, 0.46f),
                         V2(0.74f, 0.84f)));
    IconCapsule(Canvas, V2(0.56f, 0.52f), V2(0.74f, 0.84f), 0.025f, Solid(IconColor(255, 255, 255, 230)));
    IconGlow(Canvas, V2(0.74f, 0.86f), 0.2f, IconColor(200, 220, 255, 220));
    PaintDruidMoon(Canvas, V2(0.36f, 0.32f), 0.34f);
    IconSparkle(Canvas, V2(0.74f, 0.86f), 0.1f, Solid(IconColor(245, 250, 255)));
}

// NOTE(zoubir): Wrath: a green bolt, a leaf at its heart, flying right
internal void
PaintDruidWrathIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.62f, 0.42f), 0.48f, IconColor(150, 220, 90, 160));
    IconCapsule(Canvas, V2(0.1f, 0.78f), V2(0.6f, 0.44f), 0.09f,
                Gradient(IconColor(150, 220, 90, 0), IconColor(170, 235, 110, 220), V2(0.1f, 0.78f), V2(0.6f, 0.44f)));
    IconCircle(Canvas, V2(0.64f, 0.4f), 0.17f, Gradient(IconColor(235, 255, 190), IconColor(110, 180, 50),
                                                        V2(0.55f, 0.3f), V2(0.75f, 0.5f)));
    PaintDruidOliveLeaf(Canvas, V2(0.56f, 0.48f), V2(0.74f, 0.32f), 0.06f);
}

// NOTE(zoubir): Nature's Wrath: two crossed leaves, a spark between them
internal void
PaintDruidNaturesWrathIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(150, 220, 90, 140));
    PaintDruidOliveLeaf(Canvas, V2(0.2f, 0.85f), V2(0.78f, 0.18f), 0.12f);
    PaintDruidOliveLeaf(Canvas, V2(0.8f, 0.85f), V2(0.22f, 0.18f), 0.12f);
    IconSparkle(Canvas, V2(0.5f, 0.5f), 0.16f, Solid(IconColor(255, 255, 220)));
}

// NOTE(zoubir): Verdancy: a sprout in full leaf, a glow at its root
internal void
PaintDruidVerdancyIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.8f), 0.5f, IconColor(170, 235, 100, 170));
    IconCapsule(Canvas, V2(0.5f, 0.9f), V2(0.5f, 0.3f), 0.03f, Solid(IconColor(90, 140, 45)));
    PaintDruidOliveLeaf(Canvas, V2(0.5f, 0.72f), V2(0.14f, 0.5f), 0.1f);
    PaintDruidOliveLeaf(Canvas, V2(0.5f, 0.6f), V2(0.86f, 0.4f), 0.1f);
    PaintDruidOliveLeaf(Canvas, V2(0.5f, 0.42f), V2(0.5f, 0.08f), 0.09f);
}

// NOTE(zoubir): Eclipse: a gold star in front of the silver moon
internal void
PaintDruidEclipseIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.55f, IconColor(200, 190, 255, 140));
    PaintDruidMoon(Canvas, V2(0.44f, 0.46f), 0.36f);
    IconSparkle(Canvas, V2(0.64f, 0.58f), 0.24f, Gradient(IconColor(255, 255, 240), IconColor(255, 190, 70),
                                                          V2(0.5f, 0.45f), V2(0.8f, 0.75f)));
}

// NOTE(zoubir): Symbiosis: a green bolt turning into a flower
internal void
PaintDruidSymbiosisIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.55f, IconColor(170, 220, 120, 130));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.32f, 0.05f, Gradient(IconColor(150, 220, 90), IconColor(240, 140, 180),
                                                            V2(0.2f, 0.5f), V2(0.8f, 0.5f)),
            0.75f * Pi32, 2.25f * Pi32);
    IconCircle(Canvas, V2(0.24f, 0.68f), 0.1f, Solid(IconColor(170, 235, 110)));
    PaintDruidFlower(Canvas, V2(0.72f, 0.3f), 0.18f, 0.f);
}

// NOTE(zoubir): Overgrowth: a flower bursting out of a thicket of leaves
internal void
PaintDruidOvergrowthIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.55f, IconColor(240, 140, 180, 110));
    for(u32 Leaf = 0; Leaf < 5; Leaf++)
    {
        float A = Pi32 + Pi32 * (float)Leaf / 4.f;
        PaintDruidOliveLeaf(Canvas, V2(0.5f, 0.56f), V2(0.5f, 0.56f) + 0.42f * V2(Cos(A), Sin(A)), 0.08f);
    }
    PaintDruidFlower(Canvas, V2(0.5f, 0.5f), 0.24f, 0.3f);
}

// NOTE(zoubir): Wild Growth: three leaf rings, one big and two small
internal void
PaintDruidWildGrowthIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.55f, IconColor(150, 220, 90, 150));
    v2 Centres[3] = {V2(0.5f, 0.36f), V2(0.24f, 0.72f), V2(0.76f, 0.72f)};
    float Sizes[3] = {0.2f, 0.13f, 0.13f};
    for(u32 Ring = 0; Ring < 3; Ring++)
    {
        IconArc(Canvas, Centres[Ring], Sizes[Ring], 0.025f, Solid(IconColor(200, 245, 150, 200)), 0.f, 0.f);
        for(u32 Leaf = 0; Leaf < 3; Leaf++)
        {
            float A = 2.f * Pi32 * (float)Leaf / 3.f + 0.3f * (float)Ring;
            v2 At = Centres[Ring] + Sizes[Ring] * V2(Cos(A), Sin(A));
            v2 Along = V2(-Sin(A), Cos(A));
            PaintDruidOliveLeaf(Canvas, At - 0.5f * Sizes[Ring] * Along, At + 0.6f * Sizes[Ring] * Along,
                                0.3f * Sizes[Ring]);
        }
    }
}

global_variable role_icon_painter *DruidIconPainters[ROLE_KEYS] =
{
    PaintDruidRejuvenationIcon, PaintDruidStarfireIcon, PaintDruidRootsIcon, PaintDruidTranquilityIcon,
    PaintDruidRegrowthIcon, PaintDruidMoonfireIcon, PaintDruidWrathIcon,
};
global_variable talent_icon_painter *DruidTalentIconPainters[ROLE_TALENTS] =
{
    PaintDruidNaturesWrathIcon, PaintDruidRootsIcon, PaintDruidVerdancyIcon, PaintDruidEclipseIcon,
    PaintDruidTranquilityIcon, PaintDruidSymbiosisIcon,
    PaintStatHealingIcon, PaintStatVitalityIcon, PaintDruidOvergrowthIcon, PaintStatHasteIcon,
    PaintStatDamageIcon, PaintDruidWildGrowthIcon,
};
