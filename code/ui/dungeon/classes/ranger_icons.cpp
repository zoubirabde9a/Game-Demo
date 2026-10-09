/* Ranger icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Ranger's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty. Every one is built from the
   same arrow (wood shaft, steel head, teal fletching) and the class's
   teal light, so the set reads as one kit. */

// NOTE(zoubir): an arrow from Tail to Head, Thick its shaft's half width
internal void
PaintRangerArrow(icon_canvas *Canvas, v2 Tail, v2 Head, float Thick)
{
    v2 Dir = NormalizeOr(Head - Tail, V2(1.f, 0.f));
    v2 Side = V2(-Dir.Y, Dir.X);
    float HeadLength = 7.f * Thick;
    v2 Base = Head - HeadLength * Dir;
    IconCapsule(Canvas, Tail, Base, Thick,
                Gradient(IconColor(150, 100, 55), IconColor(215, 165, 100), Tail, Base));
    IconTriangle(Canvas, Head, Base + 2.8f * Thick * Side, Base - 2.8f * Thick * Side,
                 Gradient(IconColor(250, 252, 255), IconColor(120, 130, 145), Head - 0.5f * HeadLength * Side,
                          Base + 2.f * Thick * Side));
    for(float Sign = -1.f; Sign <= 1.f; Sign += 2.f)
    {
        IconTriangle(Canvas, Tail + 6.f * Thick * Dir, Tail + 1.f * Thick * Dir,
                     Tail - 1.5f * Thick * Dir + Sign * 3.4f * Thick * Side,
                     Gradient(IconColor(110, 230, 190), IconColor(40, 140, 115), Tail + 6.f * Thick * Dir,
                              Tail - 1.5f * Thick * Dir));
    }
}

// NOTE(zoubir): a reticle of four arcs and four ticks round C
internal void
PaintRangerReticle(icon_canvas *Canvas, v2 C, float R, float Width, v4 Color)
{
    for(u32 Quarter = 0; Quarter < 4; Quarter++)
    {
        float From = 0.5f * Pi32 * (float)Quarter + 0.3f;
        IconArc(Canvas, C, R, Width, Solid(Color), From, From + 0.5f * Pi32 - 0.6f);
        float A = 0.5f * Pi32 * (float)Quarter;
        v2 Dir = V2(Cos(A), Sin(A));
        IconCapsule(Canvas, C + (R + 1.6f * Width) * Dir, C + (R - 1.8f * Width) * Dir, 0.45f * Width,
                    Solid(Color));
    }
}

// NOTE(zoubir): Quick Shot: a longbow, its arrow just leaving the string
internal void
PaintRangerQuickShotIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.62f, 0.4f), 0.46f, IconColor(80, 220, 180, 110));
    // NOTE(zoubir): the bow, its back to the target, the string behind
    IconArc(Canvas, V2(0.12f, 0.88f), 0.62f, 0.07f,
            Gradient(IconColor(120, 75, 40), IconColor(225, 175, 110), V2(0.1f, 0.3f), V2(0.6f, 0.9f)),
            -1.45f, -0.1f);
    IconCapsule(Canvas, V2(0.16f, 0.27f), V2(0.73f, 0.84f), 0.012f, Solid(IconColor(240, 235, 215)));
    // NOTE(zoubir): speed lines behind it
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Off = -0.06f + 0.06f * (float)Line;
        IconCapsule(Canvas, V2(0.2f + Off, 0.84f + Off), V2(0.42f + Off, 0.62f + Off), 0.012f,
                    Gradient(IconColor(160, 255, 220, 0), IconColor(160, 255, 220, 200),
                             V2(0.2f, 0.84f), V2(0.42f, 0.62f)));
    }
    PaintRangerArrow(Canvas, V2(0.34f, 0.7f), V2(0.9f, 0.14f), 0.038f);
    IconSparkle(Canvas, V2(0.86f, 0.17f), 0.09f, Solid(IconColor(220, 255, 240)));
}

// NOTE(zoubir): Kill Shot: an arrow driven through a red mark, a skull's
// glow behind it
internal void
PaintRangerKillShotIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.6f, 0.4f), 0.5f, IconColor(255, 70, 60, 140));
    // NOTE(zoubir): the mark, a red ring with a cross in it
    IconArc(Canvas, V2(0.62f, 0.38f), 0.22f, 0.035f, Solid(IconColor(255, 90, 80)), 0.f, 2.f * Pi32);
    IconCapsule(Canvas, V2(0.62f, 0.2f), V2(0.62f, 0.56f), 0.014f, Solid(IconColor(255, 140, 120)));
    IconCapsule(Canvas, V2(0.44f, 0.38f), V2(0.8f, 0.38f), 0.014f, Solid(IconColor(255, 140, 120)));
    PaintRangerArrow(Canvas, V2(0.14f, 0.86f), V2(0.74f, 0.26f), 0.042f);
    IconSparkle(Canvas, V2(0.74f, 0.26f), 0.1f, Solid(IconColor(255, 230, 210)));
}

// NOTE(zoubir): Explosive Trap: a round trap on the ground, a burst of
// orange flame rising out of it
internal void
PaintRangerExplosiveTrapIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.5f, IconColor(255, 150, 60, 150));
    IconCircle(Canvas, V2(0.5f, 0.78f), 0.2f, Solid(IconColor(90, 70, 50)));
    IconArc(Canvas, V2(0.5f, 0.78f), 0.2f, 0.04f, Solid(IconColor(200, 170, 110)), 0.f, 2.f * Pi32);
    for(u32 Ray = 0; Ray < 7; Ray++)
    {
        float A = Pi32 * (0.15f + 0.7f * (float)Ray / 6.f);
        v2 End = V2(0.5f - 0.38f * Cos(A), 0.66f - 0.48f * Sin(A));
        IconCapsule(Canvas, V2(0.5f, 0.7f), End, 0.035f,
                    Gradient(IconColor(255, 230, 120), IconColor(255, 90, 40, 60), V2(0.5f, 0.7f), End));
    }
    IconSparkle(Canvas, V2(0.5f, 0.3f), 0.1f, Solid(IconColor(255, 245, 210)));
}

// NOTE(zoubir): Volley: arrows coming down on a teal circle on the ground
internal void
PaintRangerVolleyIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.74f), 0.46f, IconColor(80, 220, 180, 140));
    // NOTE(zoubir): the circle seen flat, a ring of beads
    for(u32 Bead = 0; Bead < 18; Bead++)
    {
        float A = 2.f * Pi32 * (float)Bead / 18.f;
        float Near = 0.5f + 0.5f * Sin(A);
        IconCircle(Canvas, V2(0.5f + 0.4f * Cos(A), 0.8f + 0.13f * Sin(A)), 0.03f + 0.012f * Near,
                   Solid(IconColor(120, 240, 200, 150 + (u32)(105.f * Near))));
    }
    for(u32 Arrow = 0; Arrow < 3; Arrow++)
    {
        float X = 0.24f + 0.26f * (float)Arrow;
        float Y = Arrow == 1 ? 0.7f : 0.8f;
        IconCapsule(Canvas, V2(X - 0.09f, Y - 0.62f), V2(X - 0.02f, Y - 0.2f), 0.02f,
                    Gradient(IconColor(160, 255, 220, 0), IconColor(160, 255, 220, 170),
                             V2(X - 0.09f, Y - 0.62f), V2(X, Y - 0.2f)));
        PaintRangerArrow(Canvas, V2(X - 0.07f, Y - 0.5f), V2(X, Y), 0.034f);
    }
    IconSparkle(Canvas, V2(0.5f, 0.72f), 0.08f, Solid(IconColor(230, 255, 245)));
}

// NOTE(zoubir): Piercing Shot: a heavy arrow and its beam of light, three
// foes burst along the line it went through
internal void
PaintRangerPiercingShotIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.52f, IconColor(80, 220, 180, 140));
    v2 From = V2(0.06f, 0.94f);
    v2 To = V2(0.9f, 0.1f);
    IconCapsule(Canvas, From, To - V2(0.08f, -0.08f), 0.1f,
                Gradient(IconColor(90, 220, 180, 0), IconColor(110, 240, 200, 210), From, To));
    IconCapsule(Canvas, From, To - V2(0.1f, -0.1f), 0.03f,
                Gradient(IconColor(220, 255, 240, 0), IconColor(255, 255, 255), From, To));
    for(u32 Foe = 0; Foe < 3; Foe++)
    {
        v2 P = From + (0.24f + 0.22f * (float)Foe) * (To - From);
        IconArc(Canvas, P, 0.07f + 0.015f * (float)Foe, 0.025f, Solid(IconColor(230, 255, 245, 230)));
        for(u32 Ray = 0; Ray < 4; Ray++)
        {
            float A = 0.5f * Pi32 * (float)Ray + 0.78f;
            v2 Dir = V2(Cos(A), Sin(A));
            IconCapsule(Canvas, P + 0.1f * Dir, P + 0.15f * Dir, 0.012f, Solid(IconColor(200, 255, 235)));
        }
    }
    PaintRangerArrow(Canvas, V2(0.5f, 0.5f), V2(0.94f, 0.06f), 0.045f);
}

// NOTE(zoubir): Hunter's Mark: a reticle over a foe's head
internal void
PaintRangerHuntersMarkIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(80, 220, 180, 130));
    IconCapsule(Canvas, V2(0.5f, 0.7f), V2(0.5f, 0.98f), 0.2f,
                Gradient(IconColor(150, 130, 150), IconColor(60, 50, 66), V2(0.4f, 0.66f), V2(0.6f, 0.98f)));
    IconCircle(Canvas, V2(0.5f, 0.48f), 0.21f, Gradient(IconColor(175, 160, 175), IconColor(70, 60, 78),
                                                       V2(0.42f, 0.32f), V2(0.6f, 0.68f)));
    IconCircle(Canvas, V2(0.43f, 0.48f), 0.035f, Solid(IconColor(255, 80, 60)));
    IconCircle(Canvas, V2(0.57f, 0.48f), 0.035f, Solid(IconColor(255, 80, 60)));
    PaintRangerReticle(Canvas, V2(0.5f, 0.48f), 0.34f, 0.065f, IconColor(130, 245, 205));
    IconCircle(Canvas, V2(0.5f, 0.48f), 0.025f, Solid(IconColor(230, 255, 245)));
}

// NOTE(zoubir): Disengage: a leap curving back, a snare left behind
internal void
PaintRangerDisengageIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.42f, 0.5f), 0.46f, IconColor(80, 220, 180, 110));
    IconArc(Canvas, V2(0.5f, 0.78f), 0.38f, 0.07f,
            Gradient(IconColor(110, 240, 200, 40), IconColor(170, 255, 230), V2(0.86f, 0.7f), V2(0.18f, 0.5f)),
            3.4f, 5.9f);
    IconTriangle(Canvas, V2(0.02f, 0.64f), V2(0.26f, 0.62f), V2(0.12f, 0.4f), Solid(IconColor(170, 255, 230)));
    // NOTE(zoubir): the snare: a steel ring of teeth
    v2 Trap = V2(0.7f, 0.76f);
    IconArc(Canvas, Trap, 0.19f, 0.05f, Gradient(IconColor(235, 240, 245), IconColor(110, 120, 130),
                                                 Trap - V2(0.f, 0.14f), Trap + V2(0.f, 0.14f)));
    for(u32 Tooth = 0; Tooth < 8; Tooth++)
    {
        float A = 2.f * Pi32 * (float)Tooth / 8.f;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 Side = V2(-Dir.Y, Dir.X);
        IconTriangle(Canvas, Trap + 0.17f * Dir + 0.04f * Side, Trap + 0.17f * Dir - 0.04f * Side,
                     Trap + 0.06f * Dir, Solid(IconColor(225, 230, 235)));
    }
    IconCircle(Canvas, Trap, 0.05f, Solid(IconColor(110, 240, 200)));
}

// NOTE(zoubir): Rapid Fire: three arrows one after another
internal void
PaintRangerRapidFireIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.56f, 0.5f), 0.46f, IconColor(80, 220, 180, 120));
    for(u32 Arrow = 0; Arrow < 3; Arrow++)
    {
        float Y = 0.26f + 0.24f * (float)Arrow;
        float X = 0.08f * (float)(Arrow == 1 ? 2 : 0);
        IconCapsule(Canvas, V2(0.04f + X, Y), V2(0.4f + X, Y), 0.02f,
                    Gradient(IconColor(160, 255, 220, 0), IconColor(160, 255, 220, 200), V2(0.04f + X, Y),
                             V2(0.4f + X, Y)));
        PaintRangerArrow(Canvas, V2(0.3f + X, Y), V2(0.84f + X, Y), 0.034f);
    }
    IconSparkle(Canvas, V2(0.92f, 0.5f), 0.07f, Solid(IconColor(220, 255, 240)));
}

// NOTE(zoubir): the talent badges, as role_talent_icons.cpp paints them
// (it is built after this file)
internal void
PaintRangerBadge(icon_canvas *Canvas, v4 Color)
{
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.17f, Solid(IconColor(16, 18, 26)));
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.14f, Solid(Color));
}

// NOTE(zoubir): Marksman: an arrow in the middle of a target
internal void
PaintRangerMarksmanIcon(icon_canvas *Canvas)
{
    v2 C = V2(0.46f, 0.54f);
    IconGlow(Canvas, C, 0.46f, IconColor(80, 220, 180, 110));
    IconCircle(Canvas, C, 0.36f, Gradient(IconColor(240, 235, 220), IconColor(170, 160, 140), C - V2(0.2f, 0.3f),
                                          C + V2(0.2f, 0.3f)));
    IconCircle(Canvas, C, 0.27f, Solid(IconColor(40, 140, 115)));
    IconCircle(Canvas, C, 0.18f, Solid(IconColor(240, 235, 220)));
    IconCircle(Canvas, C, 0.09f, Solid(IconColor(90, 210, 170)));
    PaintRangerArrow(Canvas, V2(0.92f, 0.08f), C + V2(0.02f, -0.02f), 0.026f);
}

// NOTE(zoubir): Barrage: Volley, wider
internal void
PaintRangerBarrageIcon(icon_canvas *Canvas)
{
    PaintRangerVolleyIcon(Canvas);
    PaintRangerBadge(Canvas, IconColor(255, 160, 80));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.05f, 0.02f, Solid(IconColor(30, 16, 8)));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.1f, 0.02f, Solid(IconColor(30, 16, 8)));
}

// NOTE(zoubir): Deadeye: a gold eye in a reticle
internal void
PaintRangerDeadeyeIcon(icon_canvas *Canvas)
{
    v2 C = V2(0.5f, 0.5f);
    IconGlow(Canvas, C, 0.5f, IconColor(255, 210, 120, 140));
    v2 Lid[4] = {V2(0.14f, 0.5f), V2(0.5f, 0.28f), V2(0.86f, 0.5f), V2(0.5f, 0.72f)};
    IconPolygon(Canvas, Lid, 4, Gradient(IconColor(250, 245, 230), IconColor(190, 180, 160), V2(0.5f, 0.28f),
                                         V2(0.5f, 0.72f)));
    IconCircle(Canvas, C, 0.15f, Gradient(IconColor(255, 230, 140), IconColor(200, 130, 40), C - V2(0.08f, 0.1f),
                                          C + V2(0.08f, 0.1f)));
    IconCircle(Canvas, C, 0.065f, Solid(IconColor(30, 20, 15)));
    IconCircle(Canvas, C - V2(0.04f, 0.05f), 0.025f, Solid(IconColor(255, 255, 255)));
    PaintRangerReticle(Canvas, C, 0.4f, 0.04f, IconColor(255, 215, 120));
}

// NOTE(zoubir): Lethal Mark: the mark, and a second one it jumps to
internal void
PaintRangerLethalMarkIcon(icon_canvas *Canvas)
{
    PaintRangerHuntersMarkIcon(Canvas);
    PaintRangerBadge(Canvas, IconColor(255, 110, 90));
    PaintRangerReticle(Canvas, V2(0.78f, 0.78f), 0.075f, 0.022f, IconColor(40, 10, 8));
}

// NOTE(zoubir): Pinning Volley: Volley, and an ice-blue badge with a pin
// driven into the ground, for the slow that holds on
internal void
PaintRangerPinningVolleyIcon(icon_canvas *Canvas)
{
    PaintRangerVolleyIcon(Canvas);
    PaintRangerBadge(Canvas, IconColor(150, 220, 255));
    v4 Dark = IconColor(10, 24, 40);
    IconCapsule(Canvas, V2(0.78f, 0.69f), V2(0.78f, 0.84f), 0.018f, Solid(Dark));
    IconTriangle(Canvas, V2(0.78f, 0.89f), V2(0.74f, 0.82f), V2(0.82f, 0.82f), Solid(Dark));
    IconCapsule(Canvas, V2(0.72f, 0.89f), V2(0.84f, 0.89f), 0.014f, Solid(Dark));
}

// NOTE(zoubir): Hunter's Net, the capstone: the snare at the middle of a
// net of light, three foes caught at its edge
internal void
PaintRangerHuntersNetIcon(icon_canvas *Canvas)
{
    v2 C = V2(0.5f, 0.54f);
    IconGlow(Canvas, C, 0.5f, IconColor(80, 220, 180, 140));
    v4 Strand = IconColor(150, 250, 215, 210);
    for(u32 Ring = 1; Ring <= 2; Ring++)
    {
        IconArc(Canvas, C, 0.2f * (float)Ring, 0.018f, Solid(Strand));
    }
    for(u32 Ray = 0; Ray < 8; Ray++)
    {
        float A = 2.f * Pi32 * (float)Ray / 8.f;
        v2 Dir = V2(Cos(A), Sin(A));
        IconCapsule(Canvas, C + 0.08f * Dir, C + 0.42f * Dir, 0.014f, Solid(Strand));
    }
    for(u32 Foe = 0; Foe < 3; Foe++)
    {
        float A = 2.f * Pi32 * (float)Foe / 3.f - 0.5f * Pi32;
        v2 P = C + 0.4f * V2(Cos(A), Sin(A));
        IconCircle(Canvas, P, 0.085f, Gradient(IconColor(175, 160, 175), IconColor(70, 60, 78),
                                               P - V2(0.05f, 0.06f), P + V2(0.05f, 0.06f)));
        IconArc(Canvas, P, 0.11f, 0.022f, Solid(IconColor(110, 240, 200)));
    }
    // NOTE(zoubir): the snare's steel jaws at the middle
    IconArc(Canvas, C, 0.11f, 0.035f, Gradient(IconColor(235, 240, 245), IconColor(110, 120, 130),
                                               C - V2(0.f, 0.1f), C + V2(0.f, 0.1f)));
    for(u32 Tooth = 0; Tooth < 6; Tooth++)
    {
        float A = 2.f * Pi32 * (float)Tooth / 6.f;
        v2 Dir = V2(Cos(A), Sin(A));
        v2 Side = V2(-Dir.Y, Dir.X);
        IconTriangle(Canvas, C + 0.1f * Dir + 0.03f * Side, C + 0.1f * Dir - 0.03f * Side, C + 0.03f * Dir,
                     Solid(IconColor(225, 230, 235)));
    }
    IconSparkle(Canvas, C, 0.06f, Solid(IconColor(230, 255, 245)));
}

global_variable role_icon_painter *RangerIconPainters[ROLE_KEYS] =
{
    PaintRangerVolleyIcon, PaintRangerPiercingShotIcon, PaintRangerDisengageIcon, PaintRangerRapidFireIcon,
    PaintRangerKillShotIcon, PaintRangerQuickShotIcon, PaintRangerExplosiveTrapIcon,
};
global_variable talent_icon_painter *RangerTalentIconPainters[ROLE_TALENTS] =
{
    PaintRangerMarksmanIcon, PaintRangerDisengageIcon, PaintRangerBarrageIcon, PaintRangerDeadeyeIcon,
    PaintRangerRapidFireIcon, PaintRangerLethalMarkIcon,
    PaintStatDamageIcon, PaintStatVitalityIcon, PaintRangerPinningVolleyIcon, PaintStatHasteIcon,
    PaintStatSwiftnessIcon, PaintRangerHuntersNetIcon,
};
