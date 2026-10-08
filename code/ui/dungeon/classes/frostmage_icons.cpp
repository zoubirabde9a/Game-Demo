/* Frost Mage icons (ui/dungeon/role_icons.cpp): the ability bar's pictures
   for the Frost Mage's spells, in RoleKeys order, and the talent panel's
   for its branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty. Every one is built from the
   same shard of ice (pale on its lit face, deep blue in shade) and the
   class's pale blue light, so the set reads as one kit. */

// NOTE(zoubir): a shard of ice from Base to its point at Tip, Width
// across at its widest
internal void
PaintFrostShard(icon_canvas *Canvas, v2 Base, v2 Tip, float Width)
{
    v2 Dir = NormalizeOr(Tip - Base, V2(0.f, -1.f));
    v2 Side = V2(-Dir.Y, Dir.X);
    v2 Mid = Base + 0.38f * (Tip - Base);
    IconTriangle(Canvas, Tip, Mid + 0.5f * Width * Side, Base,
                 Gradient(IconColor(245, 252, 255), IconColor(150, 210, 250), Tip, Base));
    IconTriangle(Canvas, Tip, Mid - 0.5f * Width * Side, Base,
                 Gradient(IconColor(140, 200, 245), IconColor(40, 90, 170), Tip, Base));
}

// NOTE(zoubir): a six-armed snowflake at C, R out
internal void
PaintFrostFlake(icon_canvas *Canvas, v2 C, float R, float Thick, v4 Color)
{
    for(u32 Arm = 0; Arm < 6; Arm++)
    {
        float A = Pi32 * (float)Arm / 3.f - 0.5f * Pi32;
        v2 Dir = V2(Cos(A), Sin(A));
        IconCapsule(Canvas, C, C + R * Dir, Thick, Solid(Color));
        v2 Fork = C + 0.62f * R * Dir;
        for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
        {
            float B = A + Sign * 0.75f;
            IconCapsule(Canvas, Fork, Fork + 0.3f * R * V2(Cos(B), Sin(B)), 0.75f * Thick, Solid(Color));
        }
    }
}

// NOTE(zoubir): a small "more" badge in the bottom-right corner
internal void
IconFrostBadgeMore(icon_canvas *Canvas)
{
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.16f, Solid(IconColor(12, 16, 26)));
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.13f, Solid(IconColor(255, 205, 90)));
    IconTriangle(Canvas, V2(0.8f, 0.7f), V2(0.87f, 0.8f), V2(0.73f, 0.8f), Solid(IconColor(30, 24, 10)));
    IconCapsule(Canvas, V2(0.8f, 0.79f), V2(0.8f, 0.88f), 0.022f, Solid(IconColor(30, 24, 10)));
}

// NOTE(zoubir): a small "longer" badge: a clock face
internal void
IconFrostBadgeLonger(icon_canvas *Canvas)
{
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.16f, Solid(IconColor(12, 16, 26)));
    IconCircle(Canvas, V2(0.8f, 0.8f), 0.13f, Solid(IconColor(150, 220, 255)));
    IconCapsule(Canvas, V2(0.8f, 0.8f), V2(0.8f, 0.71f), 0.022f, Solid(IconColor(16, 24, 40)));
    IconCapsule(Canvas, V2(0.8f, 0.8f), V2(0.87f, 0.83f), 0.022f, Solid(IconColor(16, 24, 40)));
}

// NOTE(zoubir): Frostbolt: a shard flying up and right in a frosty trail
internal void
PaintFrostboltIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.6f, 0.4f), 0.46f, IconColor(120, 200, 255, 130));
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Off = -0.07f + 0.07f * (float)Line;
        IconCapsule(Canvas, V2(0.12f + Off, 0.86f + Off), V2(0.42f + Off, 0.56f + Off), 0.018f,
                    Gradient(IconColor(200, 235, 255, 0), IconColor(200, 235, 255, 220),
                             V2(0.12f, 0.86f), V2(0.42f, 0.56f)));
    }
    IconGlow(Canvas, V2(0.58f, 0.42f), 0.22f, IconColor(220, 245, 255, 200));
    PaintFrostShard(Canvas, V2(0.36f, 0.64f), V2(0.86f, 0.14f), 0.2f);
    IconSparkle(Canvas, V2(0.84f, 0.16f), 0.1f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Blizzard: shards falling on a pale circle on the ground
internal void
PaintBlizzardIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.74f), 0.46f, IconColor(120, 200, 255, 140));
    IconArc(Canvas, V2(0.5f, 0.78f), 0.3f, 0.04f, Solid(IconColor(200, 235, 255)));
    for(u32 Shard = 0; Shard < 5; Shard++)
    {
        float X = 0.22f + 0.14f * (float)Shard;
        float Y = 0.3f + 0.12f * (float)((Shard * 3) % 4);
        PaintFrostShard(Canvas, V2(X - 0.06f, Y - 0.2f), V2(X, Y + 0.08f), 0.08f);
    }
    PaintFrostFlake(Canvas, V2(0.78f, 0.2f), 0.1f, 0.012f, IconColor(240, 250, 255));
}

// NOTE(zoubir): Glacial Spike: one great spike thrusting up and right,
// five small icicles round its base
internal void
PaintGlacialSpikeIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.55f, 0.45f), 0.5f, IconColor(120, 200, 255, 150));
    PaintFrostShard(Canvas, V2(0.18f, 0.84f), V2(0.88f, 0.1f), 0.32f);
    for(u32 Icicle = 0; Icicle < 5; Icicle++)
    {
        float A = 0.7f * Pi32 + 0.22f * (float)Icicle;
        v2 Base = V2(0.3f, 0.7f) + 0.16f * V2(Cos(A), Sin(A));
        PaintFrostShard(Canvas, Base, Base + 0.12f * V2(Cos(A), Sin(A)), 0.05f);
    }
    IconSparkle(Canvas, V2(0.86f, 0.13f), 0.1f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Ice Barrier: a faceted shield of ice
internal void
PaintIceBarrierIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(140, 210, 255, 130));
    v2 Shell[6];
    for(u32 Corner = 0; Corner < 6; Corner++)
    {
        float A = Pi32 * (float)Corner / 3.f - 0.5f * Pi32;
        Shell[Corner] = V2(0.5f, 0.52f) + 0.36f * V2(Cos(A), Sin(A));
    }
    IconPolygon(Canvas, Shell, 6, Gradient(IconColor(230, 248, 255, 230), IconColor(60, 120, 200, 230),
                                           V2(0.3f, 0.2f), V2(0.7f, 0.85f)));
    for(u32 Corner = 0; Corner < 6; Corner++)
    {
        IconCapsule(Canvas, V2(0.5f, 0.52f), Shell[Corner], 0.012f, Solid(IconColor(245, 252, 255, 200)));
        IconCapsule(Canvas, Shell[Corner], Shell[(Corner + 1) % 6], 0.018f, Solid(IconColor(245, 252, 255)));
    }
}

// NOTE(zoubir): Frozen Orb: a pale ball with a snowflake in it, shards
// flung from it in a spiral
internal void
PaintFrozenOrbIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(120, 200, 255, 160));
    for(u32 Shard = 0; Shard < 6; Shard++)
    {
        float A = 2.f * Pi32 * (float)Shard / 6.f + 0.3f;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 Bent = V2(Cos(A + 0.5f), Sin(A + 0.5f));
        PaintFrostShard(Canvas, V2(0.5f, 0.5f) + 0.26f * Dir, V2(0.5f, 0.5f) + 0.44f * Bent, 0.06f);
    }
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.22f, Gradient(IconColor(250, 254, 255), IconColor(120, 190, 245),
                                                     V2(0.4f, 0.35f), V2(0.6f, 0.7f)));
    PaintFrostFlake(Canvas, V2(0.5f, 0.5f), 0.15f, 0.014f, IconColor(50, 110, 190));
}

// NOTE(zoubir): Frost Nova: a ring of spikes standing up round a figure
internal void
PaintFrostMageNovaIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.5f, IconColor(120, 200, 255, 150));
    IconArc(Canvas, V2(0.5f, 0.66f), 0.32f, 0.05f, Solid(IconColor(200, 235, 255)));
    for(u32 Spike = 0; Spike < 9; Spike++)
    {
        float A = 2.f * Pi32 * (float)Spike / 9.f;
        v2 Base = V2(0.5f, 0.66f) + V2(0.32f * Cos(A), 0.2f * Sin(A));
        PaintFrostShard(Canvas, Base, Base + V2(0.f, -0.16f - 0.04f * Sin(A)), 0.07f);
    }
    IconCircle(Canvas, V2(0.5f, 0.38f), 0.07f, Solid(IconColor(60, 110, 180)));
    IconCapsule(Canvas, V2(0.5f, 0.46f), V2(0.5f, 0.64f), 0.06f, Solid(IconColor(60, 110, 180)));
}

// NOTE(zoubir): Frostbite: a snowflake over a bright blade of ice
internal void
PaintFrostbiteIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(120, 200, 255, 140));
    PaintFrostShard(Canvas, V2(0.5f, 0.92f), V2(0.5f, 0.08f), 0.2f);
    PaintFrostFlake(Canvas, V2(0.5f, 0.46f), 0.26f, 0.02f, IconColor(240, 250, 255, 230));
    IconFrostBadgeMore(Canvas);
}

// NOTE(zoubir): Permafrost: Blizzard's flake held longer
internal void
PaintPermafrostIcon(icon_canvas *Canvas)
{
    PaintBlizzardIcon(Canvas);
    IconFrostBadgeLonger(Canvas);
}

// NOTE(zoubir): Splitting Ice: a spike breaking in two at its tip
internal void
PaintSplittingIceIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(120, 200, 255, 140));
    PaintFrostShard(Canvas, V2(0.14f, 0.86f), V2(0.56f, 0.44f), 0.22f);
    PaintFrostShard(Canvas, V2(0.56f, 0.44f), V2(0.9f, 0.1f), 0.12f);
    PaintFrostShard(Canvas, V2(0.56f, 0.44f), V2(0.92f, 0.52f), 0.12f);
    IconSparkle(Canvas, V2(0.56f, 0.44f), 0.12f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Fingers of Frost: an open hand of ice, a shard rising
// from the palm
internal void
PaintFingersOfFrostIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(150, 220, 255, 150));
    icon_paint Hand = Gradient(IconColor(220, 240, 255), IconColor(80, 140, 210), V2(0.5f, 0.4f), V2(0.5f, 0.9f));
    IconCircle(Canvas, V2(0.5f, 0.72f), 0.16f, Hand);
    for(u32 Finger = 0; Finger < 4; Finger++)
    {
        float X = 0.36f + 0.093f * (float)Finger;
        IconCapsule(Canvas, V2(X, 0.66f), V2(X, 0.5f - 0.04f * (Finger == 1 || Finger == 2)), 0.035f, Hand);
    }
    IconCapsule(Canvas, V2(0.36f, 0.76f), V2(0.24f, 0.62f), 0.035f, Hand);
    PaintFrostShard(Canvas, V2(0.5f, 0.42f), V2(0.5f, 0.08f), 0.12f);
    IconSparkle(Canvas, V2(0.5f, 0.1f), 0.1f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Deep Freeze: Frost Nova held longer
internal void
PaintDeepFreezeIcon(icon_canvas *Canvas)
{
    PaintFrostMageNovaIcon(Canvas);
    IconFrostBadgeLonger(Canvas);
}

// NOTE(zoubir): Absolute Zero: a block of ice with a cracked star in it
internal void
PaintAbsoluteZeroIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(170, 225, 255, 170));
    v2 Block[4] = {V2(0.2f, 0.18f), V2(0.8f, 0.18f), V2(0.84f, 0.84f), V2(0.16f, 0.84f)};
    IconPolygon(Canvas, Block, 4, Gradient(IconColor(235, 250, 255, 240), IconColor(70, 130, 210, 240),
                                           V2(0.3f, 0.18f), V2(0.7f, 0.84f)));
    for(u32 Edge = 0; Edge < 4; Edge++)
    {
        IconCapsule(Canvas, Block[Edge], Block[(Edge + 1) % 4], 0.015f, Solid(IconColor(250, 254, 255)));
    }
    PaintFrostFlake(Canvas, V2(0.5f, 0.5f), 0.22f, 0.02f, IconColor(40, 90, 170));
    IconCapsule(Canvas, V2(0.62f, 0.2f), V2(0.7f, 0.36f), 0.01f, Solid(IconColor(255, 255, 255)));
    IconCapsule(Canvas, V2(0.7f, 0.36f), V2(0.64f, 0.46f), 0.01f, Solid(IconColor(255, 255, 255)));
}

global_variable role_icon_painter *FrostMageIconPainters[ROLE_KEYS] =
{
    PaintBlizzardIcon, PaintGlacialSpikeIcon, PaintIceBarrierIcon, PaintFrozenOrbIcon,
    PaintFrostMageNovaIcon, PaintFrostboltIcon, 0,
};
global_variable talent_icon_painter *FrostMageTalentIconPainters[ROLE_TALENTS] =
{
    PaintFrostbiteIcon, PaintIceBarrierIcon, PaintPermafrostIcon, PaintSplittingIceIcon,
    PaintFrozenOrbIcon, PaintFingersOfFrostIcon,
    PaintStatDamageIcon, PaintStatArmorIcon, PaintDeepFreezeIcon, PaintStatHasteIcon,
    PaintStatVitalityIcon, PaintAbsoluteZeroIcon,
};
