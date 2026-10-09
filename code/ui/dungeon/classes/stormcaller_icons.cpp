/* Stormcaller icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Stormcaller's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty. Every one is built from the same
   jagged bolt (a white core in a yellow glow) and the class's yellow light,
   violet where a Static Field is in it, so the set reads as one kit. */

// NOTE(zoubir): a jagged bolt through the Count points, Thick its core's
// half width; Glow the colour round it
internal void
PaintStormBolt(icon_canvas *Canvas, v2 *Points, u32 Count, float Thick, v4 Glow)
{
    for(u32 Index = 1; Index < Count; Index++)
    {
        IconCapsule(Canvas, Points[Index - 1], Points[Index], 2.6f * Thick, Solid(Glow));
    }
    for(u32 Index = 1; Index < Count; Index++)
    {
        IconCapsule(Canvas, Points[Index - 1], Points[Index], Thick,
                    Gradient(IconColor(255, 255, 255), IconColor(255, 245, 190), Points[Index - 1], Points[Index]));
    }
}

// NOTE(zoubir): a zigzag bolt from A to B in Bends bends, each Swing off
// the line, alternating sides
internal void
PaintStormZigzag(icon_canvas *Canvas, v2 A, v2 B, u32 Bends, float Swing, float Thick, v4 Glow)
{
    v2 Points[12];
    u32 Count = 0;
    v2 Dir = NormalizeOr(B - A, V2(1.f, 0.f));
    v2 Side = V2(-Dir.Y, Dir.X);
    for(u32 Step = 0; Step <= Bends + 1 && Count < ArrayCount(Points); Step++)
    {
        float At = (float)Step / (float)(Bends + 1);
        float Off = (Step == 0 || Step == Bends + 1) ? 0.f : ((Step % 2) ? Swing : -Swing);
        Points[Count++] = A + At * (B - A) + Off * Side;
    }
    PaintStormBolt(Canvas, Points, Count, Thick, Glow);
}

// NOTE(zoubir): a foe struck: a dark head with a burst of light on it
internal void
PaintStormFoe(icon_canvas *Canvas, v2 P, float R)
{
    IconCircle(Canvas, P, R, Gradient(IconColor(150, 140, 150), IconColor(55, 50, 62), P - V2(0.6f * R, 0.7f * R),
                                      P + V2(0.6f * R, 0.7f * R)));
    IconCircle(Canvas, P - V2(0.35f * R, 0.05f * R), 0.18f * R, Solid(IconColor(255, 230, 120)));
    IconCircle(Canvas, P + V2(0.35f * R, -0.05f * R), 0.18f * R, Solid(IconColor(255, 230, 120)));
}

#define STORM_ICON_GLOW IconColor(250, 220, 80, 150)
#define STORM_ICON_HALO IconColor(250, 210, 70, 120)
#define STORM_ICON_VIOLET IconColor(190, 140, 255, 150)

// NOTE(zoubir): Spark: a short bolt striking a foe and leaping on to a second
internal void
PaintStormcallerSparkIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.46f, 0.46f), 0.48f, STORM_ICON_HALO);
    PaintStormZigzag(Canvas, V2(0.08f, 0.14f), V2(0.5f, 0.56f), 2, 0.07f, 0.03f, STORM_ICON_GLOW);
    PaintStormFoe(Canvas, V2(0.56f, 0.62f), 0.13f);
    PaintStormZigzag(Canvas, V2(0.64f, 0.56f), V2(0.86f, 0.3f), 1, 0.04f, 0.018f, STORM_ICON_GLOW);
    IconSparkle(Canvas, V2(0.87f, 0.28f), 0.09f, Solid(IconColor(255, 250, 210)));
    IconSparkle(Canvas, V2(0.5f, 0.56f), 0.12f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Chain Lightning: one bolt leaping across three foes
internal void
PaintStormcallerChainIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.52f, IconColor(150, 200, 255, 130));
    v4 Blue = IconColor(140, 200, 255, 170);
    v2 Foes[3] = {V2(0.24f, 0.72f), V2(0.52f, 0.34f), V2(0.82f, 0.68f)};
    PaintStormZigzag(Canvas, V2(0.04f, 0.1f), Foes[0], 2, 0.06f, 0.028f, Blue);
    PaintStormZigzag(Canvas, Foes[0], Foes[1], 2, 0.05f, 0.024f, Blue);
    PaintStormZigzag(Canvas, Foes[1], Foes[2], 2, 0.05f, 0.02f, Blue);
    for(u32 Foe = 0; Foe < 3; Foe++)
    {
        PaintStormFoe(Canvas, Foes[Foe], 0.11f - 0.015f * (float)Foe);
    }
}

// NOTE(zoubir): Static Field: a violet ring on the ground, arcs inside it
internal void
PaintStormcallerFieldIcon(icon_canvas *Canvas)
{
    v2 C = V2(0.5f, 0.66f);
    IconGlow(Canvas, C, 0.5f, STORM_ICON_VIOLET);
    for(u32 Bead = 0; Bead < 20; Bead++)
    {
        float A = 2.f * Pi32 * (float)Bead / 20.f;
        float Near = 0.5f + 0.5f * Sin(A);
        IconCircle(Canvas, C + V2(0.42f * Cos(A), 0.2f * Sin(A)), 0.026f + 0.012f * Near,
                   Solid(IconColor(210, 170, 255, 140 + (u32)(115.f * Near))));
    }
    // NOTE(zoubir): the dome, arcs from the rim over the middle
    v4 Arc = IconColor(190, 140, 255, 190);
    PaintStormZigzag(Canvas, C + V2(-0.42f, 0.f), C + V2(0.f, -0.42f), 2, 0.04f, 0.016f, Arc);
    PaintStormZigzag(Canvas, C + V2(0.f, -0.42f), C + V2(0.42f, 0.f), 2, 0.04f, 0.016f, Arc);
    PaintStormZigzag(Canvas, C + V2(-0.2f, 0.17f), C + V2(0.f, -0.42f), 2, 0.035f, 0.014f, Arc);
    PaintStormFoe(Canvas, C + V2(0.12f, 0.02f), 0.1f);
    IconSparkle(Canvas, C + V2(0.f, -0.42f), 0.08f, Solid(IconColor(240, 225, 255)));
}

// NOTE(zoubir): Lightning Dash: a bolt racing sideways, speed lines behind
internal void
PaintStormcallerDashIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.6f, 0.5f), 0.46f, STORM_ICON_HALO);
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Y = 0.32f + 0.18f * (float)Line;
        IconCapsule(Canvas, V2(0.04f, Y), V2(0.38f, Y), 0.016f,
                    Gradient(IconColor(255, 240, 160, 0), IconColor(255, 240, 160, 200), V2(0.04f, Y), V2(0.38f, Y)));
    }
    PaintStormZigzag(Canvas, V2(0.2f, 0.5f), V2(0.88f, 0.5f), 4, 0.1f, 0.035f, STORM_ICON_GLOW);
    IconSparkle(Canvas, V2(0.88f, 0.5f), 0.12f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): a dark storm cloud over C, W wide
internal void
PaintStormCloud(icon_canvas *Canvas, v2 C, float W)
{
    v2 Puffs[5] = {V2(-0.32f, 0.06f), V2(-0.14f, -0.08f), V2(0.08f, -0.12f), V2(0.28f, -0.02f), V2(0.f, 0.06f)};
    float Sizes[5] = {0.17f, 0.21f, 0.24f, 0.19f, 0.22f};
    for(u32 Puff = 0; Puff < 5; Puff++)
    {
        v2 P = C + W * Puffs[Puff];
        float R = W * Sizes[Puff];
        IconCircle(Canvas, P, R, Gradient(IconColor(120, 120, 140), IconColor(40, 40, 56), P - V2(0.f, R),
                                          P + V2(0.f, R)));
    }
}

// NOTE(zoubir): Eye of the Storm: a dark cloud with an eye of light in it,
// bolts falling from it
internal void
PaintStormcallerEyeIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.45f), 0.52f, IconColor(150, 200, 255, 130));
    v4 Blue = IconColor(140, 200, 255, 170);
    PaintStormZigzag(Canvas, V2(0.3f, 0.4f), V2(0.2f, 0.94f), 2, 0.05f, 0.022f, Blue);
    PaintStormZigzag(Canvas, V2(0.7f, 0.4f), V2(0.82f, 0.92f), 2, 0.05f, 0.022f, Blue);
    PaintStormCloud(Canvas, V2(0.5f, 0.36f), 0.9f);
    v2 Lid[4] = {V2(0.32f, 0.34f), V2(0.5f, 0.24f), V2(0.68f, 0.34f), V2(0.5f, 0.44f)};
    IconPolygon(Canvas, Lid, 4, Gradient(IconColor(255, 250, 220), IconColor(250, 210, 90), V2(0.5f, 0.24f),
                                         V2(0.5f, 0.44f)));
    IconCircle(Canvas, V2(0.5f, 0.34f), 0.055f, Solid(IconColor(40, 60, 120)));
    IconCircle(Canvas, V2(0.485f, 0.325f), 0.018f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Thunderclap: a bolt from the sky onto the ground, a ring
// bursting where it lands
internal void
PaintStormcallerThunderclapIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.78f), 0.5f, STORM_ICON_HALO);
    v2 Ground = V2(0.5f, 0.8f);
    IconArc(Canvas, Ground, 0.3f, 0.03f, Solid(IconColor(255, 230, 120, 200)));
    IconArc(Canvas, Ground, 0.18f, 0.025f, Solid(IconColor(255, 250, 220, 230)));
    v2 Points[6] = {V2(0.6f, 0.0f), V2(0.42f, 0.24f), V2(0.6f, 0.36f), V2(0.38f, 0.56f), V2(0.56f, 0.64f),
                    Ground};
    PaintStormBolt(Canvas, Points, 6, 0.045f, STORM_ICON_GLOW);
    IconSparkle(Canvas, Ground, 0.16f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): the talent badges, as role_talent_icons.cpp paints them
// (it is built after this file)
internal void
PaintStormcallerBadge(icon_canvas *Canvas, v4 Color)
{
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.17f, Solid(IconColor(16, 18, 26)));
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.14f, Solid(Color));
}

// NOTE(zoubir): Voltage: a bolt in a ring of light
internal void
PaintStormcallerVoltageIcon(icon_canvas *Canvas)
{
    v2 C = V2(0.5f, 0.5f);
    IconGlow(Canvas, C, 0.5f, STORM_ICON_HALO);
    IconArc(Canvas, C, 0.36f, 0.05f, Gradient(IconColor(255, 245, 190), IconColor(200, 150, 40), C - V2(0.f, 0.36f),
                                              C + V2(0.f, 0.36f)));
    v2 Points[4] = {V2(0.58f, 0.12f), V2(0.38f, 0.52f), V2(0.6f, 0.5f), V2(0.42f, 0.88f)};
    PaintStormBolt(Canvas, Points, 4, 0.045f, STORM_ICON_GLOW);
}

// NOTE(zoubir): Conductor: Chain Lightning, and a badge with a fork
internal void
PaintStormcallerConductorIcon(icon_canvas *Canvas)
{
    PaintStormcallerChainIcon(Canvas);
    PaintStormcallerBadge(Canvas, IconColor(140, 200, 255));
    v4 Dark = IconColor(10, 20, 40);
    IconCapsule(Canvas, V2(0.7f, 0.84f), V2(0.78f, 0.74f), 0.016f, Solid(Dark));
    IconCapsule(Canvas, V2(0.78f, 0.74f), V2(0.86f, 0.84f), 0.016f, Solid(Dark));
    IconCapsule(Canvas, V2(0.78f, 0.74f), V2(0.78f, 0.86f), 0.016f, Solid(Dark));
}

// NOTE(zoubir): Live Wire: a coiled wire round a crackling nova, safe in hand
internal void
PaintStormcallerLiveWireIcon(icon_canvas *Canvas)
{
    v2 C = V2(0.5f, 0.5f);
    IconGlow(Canvas, C, 0.52f, STORM_ICON_HALO);
    for(u32 Ray = 0; Ray < 8; Ray++)
    {
        float A = 2.f * Pi32 * (float)Ray / 8.f + 0.2f;
        v2 Dir = V2(Cos(A), Sin(A));
        PaintStormZigzag(Canvas, C + 0.12f * Dir, C + 0.44f * Dir, 1, 0.03f, 0.014f, STORM_ICON_GLOW);
    }
    for(u32 Loop = 0; Loop < 3; Loop++)
    {
        IconArc(Canvas, C, 0.14f + 0.07f * (float)Loop, 0.022f,
                Solid(IconColor(220, 120, 60, 230)), 0.3f + 0.6f * (float)Loop, 4.6f + 0.6f * (float)Loop);
    }
    IconCircle(Canvas, C, 0.08f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Capacitor: a cell filling with lightning, an arrow round
// it giving Charge back
internal void
PaintStormcallerCapacitorIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, STORM_ICON_HALO);
    v2 Cell[4] = {V2(0.3f, 0.2f), V2(0.7f, 0.2f), V2(0.7f, 0.88f), V2(0.3f, 0.88f)};
    IconPolygon(Canvas, Cell, 4, Gradient(IconColor(70, 70, 86), IconColor(30, 30, 40), V2(0.3f, 0.2f), V2(0.7f, 0.88f)));
    v2 Cap[4] = {V2(0.42f, 0.12f), V2(0.58f, 0.12f), V2(0.58f, 0.2f), V2(0.42f, 0.2f)};
    IconPolygon(Canvas, Cap, 4, Solid(IconColor(160, 160, 176)));
    v2 Fill[4] = {V2(0.34f, 0.46f), V2(0.66f, 0.46f), V2(0.66f, 0.84f), V2(0.34f, 0.84f)};
    IconPolygon(Canvas, Fill, 4, Gradient(IconColor(255, 240, 150), IconColor(220, 160, 30), V2(0.5f, 0.46f),
                                          V2(0.5f, 0.84f)));
    v2 Points[4] = {V2(0.56f, 0.28f), V2(0.42f, 0.56f), V2(0.58f, 0.56f), V2(0.44f, 0.82f)};
    PaintStormBolt(Canvas, Points, 4, 0.026f, IconColor(255, 255, 255, 120));
    IconArc(Canvas, V2(0.5f, 0.52f), 0.42f, 0.03f, Solid(IconColor(255, 230, 120, 220)), 3.6f, 5.9f);
    IconTriangle(Canvas, V2(0.86f, 0.42f), V2(0.92f, 0.28f), V2(0.76f, 0.3f), Solid(IconColor(255, 230, 120)));
}

// NOTE(zoubir): Arc Field: Static Field, and a badge with rings widening
internal void
PaintStormcallerArcFieldIcon(icon_canvas *Canvas)
{
    PaintStormcallerFieldIcon(Canvas);
    PaintStormcallerBadge(Canvas, IconColor(190, 140, 255));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.05f, 0.02f, Solid(IconColor(30, 10, 40)));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.1f, 0.02f, Solid(IconColor(30, 10, 40)));
}

// NOTE(zoubir): Stormbringer, the capstone: a cloud calling three bolts down
// at once
internal void
PaintStormcallerStormbringerIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.52f, STORM_ICON_HALO);
    PaintStormZigzag(Canvas, V2(0.22f, 0.36f), V2(0.14f, 0.92f), 2, 0.05f, 0.024f, STORM_ICON_GLOW);
    PaintStormZigzag(Canvas, V2(0.5f, 0.38f), V2(0.52f, 0.96f), 3, 0.06f, 0.034f, STORM_ICON_GLOW);
    PaintStormZigzag(Canvas, V2(0.78f, 0.36f), V2(0.88f, 0.92f), 2, 0.05f, 0.024f, STORM_ICON_GLOW);
    PaintStormCloud(Canvas, V2(0.5f, 0.28f), 0.92f);
    IconSparkle(Canvas, V2(0.52f, 0.94f), 0.12f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): Ball Lightning: a crackling ball rolling to the right,
// arcs reaching out of it
internal void
PaintStormcallerBallIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.55f, 0.5f), 0.5f, IconColor(255, 230, 90, 150));
    IconCircle(Canvas, V2(0.55f, 0.5f), 0.22f,
               Gradient(IconColor(255, 255, 230), IconColor(240, 180, 40), V2(0.48f, 0.42f), V2(0.7f, 0.66f)));
    for(u32 Arc = 0; Arc < 5; Arc++)
    {
        float A = 2.f * Pi32 * (float)Arc / 5.f + 0.4f;
        v2 Mid = V2(0.55f + 0.32f * Cos(A), 0.5f + 0.32f * Sin(A));
        v2 Tip = V2(0.55f + 0.44f * Cos(A + 0.25f), 0.5f + 0.44f * Sin(A + 0.25f));
        IconCapsule(Canvas, V2(0.55f + 0.2f * Cos(A), 0.5f + 0.2f * Sin(A)), Mid, 0.02f,
                    Solid(IconColor(255, 245, 160)));
        IconCapsule(Canvas, Mid, Tip, 0.015f, Solid(IconColor(255, 245, 160, 200)));
    }
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Y = 0.4f + 0.1f * (float)Line;
        IconCapsule(Canvas, V2(0.04f, Y), V2(0.26f, Y), 0.012f,
                    Gradient(IconColor(255, 240, 150, 0), IconColor(255, 240, 150, 200), V2(0.04f, 0.f), V2(0.26f, 0.f)));
    }
}

global_variable role_icon_painter *StormcallerIconPainters[ROLE_KEYS] =
{
    PaintStormcallerChainIcon, PaintStormcallerFieldIcon, PaintStormcallerDashIcon, PaintStormcallerEyeIcon,
    PaintStormcallerThunderclapIcon, PaintStormcallerSparkIcon, PaintStormcallerBallIcon,
};
global_variable talent_icon_painter *StormcallerTalentIconPainters[ROLE_TALENTS] =
{
    PaintStormcallerVoltageIcon, PaintStormcallerDashIcon, PaintStormcallerConductorIcon,
    PaintStormcallerLiveWireIcon, PaintStormcallerEyeIcon, PaintStormcallerCapacitorIcon,
    PaintStatDamageIcon, PaintStatVitalityIcon, PaintStormcallerArcFieldIcon, PaintStatHasteIcon,
    PaintStatSwiftnessIcon, PaintStormcallerStormbringerIcon,
};
