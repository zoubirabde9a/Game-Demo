/* Duel perk icons: one per perk that can roll into a wild slot of the
   duel tree (sim/progression/talents/duel_perks.cpp), painted like the
   other talent icons. DuelPerkIconPainters lists them in duel_perk order;
   the perks that were fixed passives before wear their old icons
   (talent_icons.cpp). */

// NOTE(zoubir): a flame of three layers round Base, Size tall
internal void
PaintDuelFlame(icon_canvas *Canvas, v2 Base, float R, v4 Outer, v4 Middle, v4 Inner)
{
    v4 Colours[3] = {Outer, Middle, Inner};
    for(u32 Layer = 0; Layer < 3; Layer++)
    {
        float Size = R * (1.f - 0.3f * (float)Layer);
        v2 At = Base - V2(0.f, 0.02f * (float)Layer);
        IconCircle(Canvas, At, Size, Solid(Colours[Layer]));
        IconTriangle(Canvas, At + V2(-0.92f * Size, -0.35f * Size), At + V2(0.92f * Size, -0.35f * Size),
                     At + V2(0.08f * Size, -2.4f * Size), Solid(Colours[Layer]));
    }
}

// NOTE(zoubir): Kindling: a small flame under a turning arrow
internal void
PaintDuelKindlingIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.44f, IconColor(255, 120, 30, 130));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.34f, 0.05f, Solid(IconColor(255, 220, 140)), -0.9f * Pi32, 0.6f * Pi32);
    IconTriangle(Canvas, V2(0.74f, 0.66f), V2(0.92f, 0.62f), V2(0.8f, 0.82f), Solid(IconColor(255, 220, 140)));
    PaintDuelFlame(Canvas, V2(0.5f, 0.66f), 0.14f, IconColor(220, 60, 20), IconColor(255, 150, 40),
                   IconColor(255, 235, 140));
}

// NOTE(zoubir): Searing: a white-hot fireball with a red rim
internal void
PaintDuelSearingIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(255, 70, 20, 170));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.27f, Gradient(IconColor(255, 240, 200), IconColor(200, 30, 10),
                                                      V2(0.4f, 0.4f), V2(0.7f, 0.7f)));
    IconCircle(Canvas, V2(0.46f, 0.45f), 0.12f, Solid(IconColor(255, 255, 240)));
    IconSparkle(Canvas, V2(0.78f, 0.24f), 0.1f, Solid(IconColor(255, 230, 160)));
}

// NOTE(zoubir): Scorch: a fireball trailing a smoky slow ring
internal void
PaintDuelScorchIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.62f, 0.4f), 0.4f, IconColor(255, 110, 30, 130));
    IconArc(Canvas, V2(0.4f, 0.66f), 0.22f, 0.05f, Solid(IconColor(140, 120, 110)), 0.f, 2.f * Pi32);
    IconArc(Canvas, V2(0.4f, 0.66f), 0.12f, 0.035f, Solid(IconColor(110, 95, 90)), 0.f, 2.f * Pi32);
    IconCircle(Canvas, V2(0.66f, 0.36f), 0.15f, Gradient(IconColor(255, 230, 120), IconColor(210, 50, 20),
                                                        V2(0.6f, 0.3f), V2(0.76f, 0.46f)));
}

// NOTE(zoubir): Overcharge: a crackling orb, bolts leaving it
internal void
PaintDuelOverchargeIcon(icon_canvas *Canvas)
{
    v4 Bolt = IconColor(255, 240, 150);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(255, 140, 40, 160));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.2f, Gradient(IconColor(255, 250, 220), IconColor(255, 110, 20),
                                                     V2(0.44f, 0.44f), V2(0.62f, 0.62f)));
    for(u32 Ray = 0; Ray < 4; Ray++)
    {
        float Angle = 0.25f * Pi32 + 0.5f * Pi32 * (float)Ray;
        v2 Out = V2(Cos(Angle), Sin(Angle));
        v2 Side = V2(-Out.Y, Out.X);
        v2 A = V2(0.5f, 0.5f) + 0.24f * Out;
        v2 B = V2(0.5f, 0.5f) + 0.33f * Out + 0.05f * Side;
        v2 C = V2(0.5f, 0.5f) + 0.44f * Out;
        IconCapsule(Canvas, A, B, 0.022f, Solid(Bolt));
        IconCapsule(Canvas, B, C, 0.022f, Solid(Bolt));
    }
}

// NOTE(zoubir): Quickstep: two afterimage boots of light
internal void
PaintDuelQuickstepIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.56f, 0.5f), 0.44f, IconColor(140, 120, 255, 120));
    for(u32 Image = 0; Image < 3; Image++)
    {
        float X = 0.26f + 0.2f * (float)Image;
        u32 Alpha = 90 + 80 * Image;
        v4 Colour = IconColor(200, 190, 255, Alpha);
        IconCapsule(Canvas, V2(X, 0.3f), V2(X, 0.62f), 0.07f, Solid(Colour));
        IconCapsule(Canvas, V2(X, 0.66f), V2(X + 0.12f, 0.66f), 0.06f, Solid(Colour));
    }
}

// NOTE(zoubir): Keen Edge: a blade with a bright edge line
internal void
PaintDuelKeenEdgeIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.44f, IconColor(140, 210, 255, 110));
    IconTriangle(Canvas, V2(0.2f, 0.8f), V2(0.32f, 0.84f), V2(0.82f, 0.16f), Solid(IconColor(170, 185, 205)));
    IconCapsule(Canvas, V2(0.27f, 0.8f), V2(0.8f, 0.18f), 0.015f, Solid(IconColor(255, 255, 255)));
    IconCapsule(Canvas, V2(0.16f, 0.7f), V2(0.36f, 0.9f), 0.035f, Solid(IconColor(150, 110, 60)));
    IconSparkle(Canvas, V2(0.72f, 0.3f), 0.09f, Solid(IconColor(230, 250, 255)));
}

// NOTE(zoubir): Hunter: a kunai inside a target's ring
internal void
PaintDuelHunterIcon(icon_canvas *Canvas)
{
    v4 Red = IconColor(235, 80, 70);
    IconArc(Canvas, V2(0.5f, 0.5f), 0.34f, 0.04f, Solid(Red), 0.f, 2.f * Pi32);
    IconArc(Canvas, V2(0.5f, 0.5f), 0.18f, 0.03f, Solid(Red), 0.f, 2.f * Pi32);
    IconTriangle(Canvas, V2(0.42f, 0.6f), V2(0.6f, 0.42f), V2(0.86f, 0.14f), Solid(IconColor(200, 210, 225)));
    IconCapsule(Canvas, V2(0.2f, 0.82f), V2(0.44f, 0.58f), 0.03f, Solid(IconColor(90, 70, 60)));
}

// NOTE(zoubir): Adrenaline: a heart with a pulse line through it
internal void
PaintDuelAdrenalineIcon(icon_canvas *Canvas)
{
    v4 Red = IconColor(230, 50, 70);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 60, 80, 120));
    IconCircle(Canvas, V2(0.38f, 0.42f), 0.16f, Solid(Red));
    IconCircle(Canvas, V2(0.62f, 0.42f), 0.16f, Solid(Red));
    IconTriangle(Canvas, V2(0.23f, 0.48f), V2(0.77f, 0.48f), V2(0.5f, 0.82f), Solid(Red));
    v4 Line = IconColor(255, 245, 230);
    v2 Points[6] = {V2(0.12f, 0.52f), V2(0.36f, 0.52f), V2(0.44f, 0.34f), V2(0.54f, 0.68f),
                    V2(0.62f, 0.52f), V2(0.88f, 0.52f)};
    for(u32 Index = 0; Index + 1 < ArrayCount(Points); Index++)
    {
        IconCapsule(Canvas, Points[Index], Points[Index + 1], 0.025f, Solid(Line));
    }
}

// NOTE(zoubir): Phantom: a pale see-through figure
internal void
PaintDuelPhantomIcon(icon_canvas *Canvas)
{
    v4 Pale = IconColor(200, 220, 255, 170);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(150, 170, 255, 110));
    IconCircle(Canvas, V2(0.5f, 0.3f), 0.12f, Solid(Pale));
    IconCapsule(Canvas, V2(0.5f, 0.44f), V2(0.5f, 0.66f), 0.13f, Solid(Pale));
    IconTriangle(Canvas, V2(0.37f, 0.66f), V2(0.63f, 0.66f), V2(0.5f, 0.9f), Solid(Pale));
    IconCircle(Canvas, V2(0.45f, 0.29f), 0.025f, Solid(IconColor(40, 50, 90)));
    IconCircle(Canvas, V2(0.55f, 0.29f), 0.025f, Solid(IconColor(40, 50, 90)));
}

// NOTE(zoubir): Thick Skin: three overlapping scales
internal void
PaintDuelThickSkinIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.44f, IconColor(120, 200, 140, 110));
    v2 Scales[3] = {V2(0.36f, 0.42f), V2(0.64f, 0.42f), V2(0.5f, 0.64f)};
    for(u32 Index = 0; Index < 3; Index++)
    {
        IconCircle(Canvas, Scales[Index], 0.17f, Gradient(IconColor(170, 220, 160), IconColor(60, 120, 80),
                                                        Scales[Index] - V2(0.1f, 0.1f),
                                                        Scales[Index] + V2(0.1f, 0.1f)));
        IconArc(Canvas, Scales[Index], 0.17f, 0.02f, Solid(IconColor(30, 70, 45)), 0.f, 2.f * Pi32);
    }
}

// NOTE(zoubir): Mending: a green cross with a halo
internal void
PaintDuelMendingIcon(icon_canvas *Canvas)
{
    v4 Green = IconColor(110, 230, 140);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(80, 230, 130, 130));
    IconCapsule(Canvas, V2(0.5f, 0.24f), V2(0.5f, 0.76f), 0.09f, Solid(Green));
    IconCapsule(Canvas, V2(0.24f, 0.5f), V2(0.76f, 0.5f), 0.09f, Solid(Green));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.06f, Solid(IconColor(235, 255, 240)));
}

// NOTE(zoubir): Steadfast: an anchor
internal void
PaintDuelSteadfastIcon(icon_canvas *Canvas)
{
    v4 Iron = IconColor(190, 200, 215);
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.44f, IconColor(120, 160, 220, 110));
    IconArc(Canvas, V2(0.5f, 0.22f), 0.07f, 0.03f, Solid(Iron), 0.f, 2.f * Pi32);
    IconCapsule(Canvas, V2(0.5f, 0.3f), V2(0.5f, 0.8f), 0.035f, Solid(Iron));
    IconCapsule(Canvas, V2(0.36f, 0.38f), V2(0.64f, 0.38f), 0.03f, Solid(Iron));
    IconArc(Canvas, V2(0.5f, 0.6f), 0.22f, 0.035f, Solid(Iron), 0.15f * Pi32, 0.85f * Pi32);
}

// NOTE(zoubir): Thorns: a ring of spikes
internal void
PaintDuelThornsIcon(icon_canvas *Canvas)
{
    v4 Vine = IconColor(90, 160, 70);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.44f, IconColor(110, 200, 90, 110));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.24f, 0.05f, Solid(Vine), 0.f, 2.f * Pi32);
    for(u32 Spike = 0; Spike < 8; Spike++)
    {
        float Angle = 0.25f * Pi32 * (float)Spike;
        v2 Out = V2(Cos(Angle), Sin(Angle));
        v2 Side = V2(-Out.Y, Out.X);
        v2 Root = V2(0.5f, 0.5f) + 0.25f * Out;
        IconTriangle(Canvas, Root + 0.05f * Side, Root - 0.05f * Side, Root + 0.17f * Out,
                     Solid(IconColor(200, 230, 150)));
    }
}

// NOTE(zoubir): Bastion: a tower shield with a green glow at its heart
internal void
PaintDuelBastionIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(120, 220, 170, 120));
    v2 Shape[5] = {V2(0.26f, 0.2f), V2(0.74f, 0.2f), V2(0.74f, 0.56f), V2(0.5f, 0.86f), V2(0.26f, 0.56f)};
    IconPolygon(Canvas, Shape, 5, Gradient(IconColor(170, 185, 210), IconColor(80, 95, 125),
                                          V2(0.3f, 0.2f), V2(0.7f, 0.8f)));
    IconCapsule(Canvas, V2(0.5f, 0.36f), V2(0.5f, 0.64f), 0.045f, Solid(IconColor(120, 240, 160)));
    IconCapsule(Canvas, V2(0.36f, 0.5f), V2(0.64f, 0.5f), 0.045f, Solid(IconColor(120, 240, 160)));
}
