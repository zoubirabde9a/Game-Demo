/* Berserker icons (ui/dungeon/role_icons.cpp): the ability bar's pictures for
   the Berserker's spells, in RoleKeys order, and the talent panel's for its
   branch, painted in code on the icon canvas (ui/ability_icons/
   icon_canvas.cpp). 0 leaves a cell empty.

   They share a great axe (BerserkerIconAxe): a dark haft bound in leather
   and a broad steel blade, lit along its edge, over a blood-red glow. A
   talent that changes a spell shows that spell with a badge saying how. */

// NOTE(zoubir): the axe's blade: its socket on the haft at Socket, the haft
// running along Along, the edge facing Across; Size about the blade's
// height. Two convex halves and a lit edge
internal void
BerserkerIconBlade(icon_canvas *Canvas, v2 Socket, v2 Along, v2 Across, float Size)
{
    v4 Steel = IconColor(235, 236, 242);
    v4 Dark = IconColor(105, 108, 122);
    v2 NeckTop = Socket + 0.32f * Size * Along + 0.18f * Size * Across;
    v2 NeckLow = Socket - 0.22f * Size * Along + 0.18f * Size * Across;
    v2 Horn = Socket + 0.62f * Size * Along + 0.78f * Size * Across;
    v2 EdgeMid = Socket + 0.1f * Size * Along + 0.95f * Size * Across;
    v2 Beard = Socket - 0.6f * Size * Along + 0.74f * Size * Across;
    v2 Top[4] = {Socket + 0.36f * Size * Along, Horn, EdgeMid, NeckTop};
    v2 Low[5] = {NeckTop, EdgeMid, Beard, NeckLow, Socket - 0.24f * Size * Along};
    icon_paint Paint = Gradient(Dark, Steel, Socket, Socket + 0.9f * Size * Across);
    IconPolygon(Canvas, Top, 4, Paint);
    IconPolygon(Canvas, Low, 5, Paint);
    v2 Spike[3] = {Socket + 0.16f * Size * Along, Socket - 0.16f * Size * Along,
                   Socket - 0.48f * Size * Across};
    IconPolygon(Canvas, Spike, 3, Gradient(Steel, Dark, Spike[2], Socket));
    IconCapsule(Canvas, Horn, EdgeMid, 0.018f, Solid(IconColor(255, 255, 255)));
    IconCapsule(Canvas, EdgeMid, Beard, 0.018f, Solid(IconColor(255, 255, 255)));
    // NOTE(zoubir): a rune burning in the steel
    v2 Rune = Socket + 0.05f * Size * Along + 0.5f * Size * Across;
    IconGlow(Canvas, Rune, 0.08f, IconColor(255, 120, 60, 200));
    IconCapsule(Canvas, Rune - 0.06f * Along, Rune + 0.06f * Along, 0.012f, Solid(IconColor(255, 170, 90)));
    IconCapsule(Canvas, Rune - 0.05f * Across, Rune + 0.05f * Across, 0.012f, Solid(IconColor(255, 170, 90)));
}

// NOTE(zoubir): a great axe from Butt to Head (the blade's socket), the edge
// on Side of the haft (+1 to its right going up it)
internal void
BerserkerIconAxe(icon_canvas *Canvas, v2 Butt, v2 Head, float Side, float BladeSize)
{
    v2 Along = Head - Butt;
    float Length = SquareRoot(DotProduct(Along, Along));
    Along = Along * (1.f / Length);
    v2 Across = Side * V2(-Along.Y, Along.X);
    v2 Top = Head + 0.1f * Along;
    IconCapsule(Canvas, Butt, Top, 0.05f, Gradient(IconColor(150, 100, 60), IconColor(80, 50, 30), Butt, Top));
    for(u32 Band = 0; Band < 3; Band++)
    {
        v2 At = Butt + (0.12f + 0.08f * (float)Band) * Length * Along;
        IconCapsule(Canvas, At - 0.055f * Across, At + 0.055f * Across, 0.018f, Solid(IconColor(40, 26, 20)));
    }
    IconCircle(Canvas, Butt, 0.04f, Solid(IconColor(90, 90, 100)));
    BerserkerIconBlade(Canvas, Head, Along, Across, BladeSize);
}

// NOTE(zoubir): Cleave: a great axe at the end of a wide crimson swing
internal void
PaintBerserkerCleaveIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(230, 40, 40, 140));
    IconArc(Canvas, V2(0.42f, 0.58f), 0.36f, 0.12f,
            Gradient(IconColor(120, 0, 10, 0), IconColor(255, 70, 60, 230), V2(0.1f, 0.6f), V2(0.7f, 0.2f)),
            3.3f, 5.6f);
    IconArc(Canvas, V2(0.42f, 0.58f), 0.42f, 0.02f, Solid(IconColor(255, 210, 180, 220)), 3.5f, 5.6f);
    BerserkerIconAxe(Canvas, V2(0.2f, 0.9f), V2(0.66f, 0.3f), 1.f, 0.4f);
}

// NOTE(zoubir): Leap: an arc over to a red ring cracking the ground, the
// axe coming down at its end
internal void
PaintBerserkerLeapIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.6f, 0.72f), 0.42f, IconColor(230, 50, 40, 150));
    for(u32 Dot = 0; Dot < 12; Dot++)
    {
        float Angle = 2.f * Pi32 * (float)Dot / 12.f;
        IconCircle(Canvas, V2(0.62f + 0.24f * Cos(Angle), 0.8f + 0.08f * Sin(Angle)), 0.025f,
                   Solid(IconColor(255, 80, 60)));
    }
    IconArc(Canvas, V2(0.4f, 0.8f), 0.32f, 0.035f,
            Gradient(IconColor(255, 200, 170, 40), IconColor(255, 120, 90, 230), V2(0.1f, 0.8f), V2(0.6f, 0.5f)),
            3.3f, 5.6f);
    IconCapsule(Canvas, V2(0.62f, 0.8f), V2(0.5f, 0.92f), 0.015f, Solid(IconColor(60, 20, 20)));
    IconCapsule(Canvas, V2(0.62f, 0.8f), V2(0.8f, 0.9f), 0.015f, Solid(IconColor(60, 20, 20)));
    BerserkerIconAxe(Canvas, V2(0.86f, 0.2f), V2(0.66f, 0.62f), -1.f, 0.3f);
}

// NOTE(zoubir): Whirlwind: the axe flat in a ring of red crescents
internal void
PaintBerserkerWhirlwindIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.5f, IconColor(230, 40, 40, 150));
    for(u32 Blade = 0; Blade < 3; Blade++)
    {
        float Start = 2.f * Pi32 * (float)Blade / 3.f;
        IconArc(Canvas, V2(0.5f, 0.5f), 0.34f, 0.08f,
                Gradient(IconColor(140, 0, 10, 0), IconColor(255, 80, 60, 230),
                         V2(0.5f + 0.34f * Cos(Start), 0.5f + 0.34f * Sin(Start)),
                         V2(0.5f + 0.34f * Cos(Start + 1.6f), 0.5f + 0.34f * Sin(Start + 1.6f))),
                Start, Start + 1.6f);
    }
    IconArc(Canvas, V2(0.5f, 0.5f), 0.4f, 0.015f, Solid(IconColor(255, 210, 180, 200)));
    BerserkerIconAxe(Canvas, V2(0.18f, 0.56f), V2(0.66f, 0.46f), -1.f, 0.32f);
}

// NOTE(zoubir): Execute: a huge blade chopping straight down onto a skull,
// the ground splitting
internal void
PaintBerserkerExecuteIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.66f), 0.5f, IconColor(255, 40, 30, 170));
    IconCapsule(Canvas, V2(0.5f, 0.82f), V2(0.2f, 0.94f), 0.02f, Solid(IconColor(90, 10, 10)));
    IconCapsule(Canvas, V2(0.5f, 0.82f), V2(0.8f, 0.95f), 0.02f, Solid(IconColor(90, 10, 10)));
    IconCircle(Canvas, V2(0.36f, 0.72f), 0.13f, Gradient(IconColor(240, 235, 220), IconColor(150, 140, 130),
                                                         V2(0.3f, 0.62f), V2(0.42f, 0.84f)));
    IconCircle(Canvas, V2(0.32f, 0.71f), 0.03f, Solid(IconColor(40, 10, 10)));
    IconCircle(Canvas, V2(0.4f, 0.71f), 0.03f, Solid(IconColor(40, 10, 10)));
    BerserkerIconAxe(Canvas, V2(0.7f, 0.06f), V2(0.62f, 0.5f), 1.f, 0.38f);
    IconSparkle(Canvas, V2(0.5f, 0.8f), 0.1f, Solid(IconColor(255, 230, 200)));
}

// NOTE(zoubir): Bloodthirst: a blade dripping red into a glowing drop
internal void
PaintBerserkerBloodthirstIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.5f, IconColor(220, 20, 40, 160));
    BerserkerIconAxe(Canvas, V2(0.16f, 0.86f), V2(0.5f, 0.36f), 1.f, 0.34f);
    v2 Drop[5] = {V2(0.72f, 0.42f), V2(0.84f, 0.64f), V2(0.8f, 0.76f), V2(0.64f, 0.76f), V2(0.6f, 0.64f)};
    IconPolygon(Canvas, Drop, 5, Gradient(IconColor(255, 110, 110), IconColor(150, 0, 20),
                                          V2(0.68f, 0.5f), V2(0.74f, 0.8f)));
    IconCircle(Canvas, V2(0.72f, 0.68f), 0.08f, Gradient(IconColor(255, 110, 110), IconColor(150, 0, 20),
                                                        V2(0.66f, 0.62f), V2(0.78f, 0.76f)));
    IconCircle(Canvas, V2(0.69f, 0.64f), 0.022f, Solid(IconColor(255, 220, 220)));
    IconCircle(Canvas, V2(0.58f, 0.86f), 0.025f, Solid(IconColor(200, 20, 30)));
}

// NOTE(zoubir): Berserk: a dark helm with burning red eyes, rage flaring
// off it
internal void
PaintBerserkerBerserkIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.52f, IconColor(255, 50, 20, 180));
    v2 Flame[5] = {V2(0.5f, 0.04f), V2(0.78f, 0.34f), V2(0.74f, 0.6f), V2(0.26f, 0.6f), V2(0.22f, 0.34f)};
    IconPolygon(Canvas, Flame, 5, Gradient(IconColor(255, 200, 120), IconColor(200, 20, 10),
                                           V2(0.5f, 0.04f), V2(0.5f, 0.6f)));
    IconTriangle(Canvas, V2(0.16f, 0.5f), V2(0.3f, 0.4f), V2(0.08f, 0.16f), Solid(IconColor(255, 110, 40)));
    IconTriangle(Canvas, V2(0.84f, 0.5f), V2(0.7f, 0.4f), V2(0.92f, 0.16f), Solid(IconColor(255, 110, 40)));
    v2 Helm[6] = {V2(0.26f, 0.38f), V2(0.74f, 0.38f), V2(0.78f, 0.66f), V2(0.62f, 0.92f),
                  V2(0.38f, 0.92f), V2(0.22f, 0.66f)};
    IconPolygon(Canvas, Helm, 6, Gradient(IconColor(110, 100, 105), IconColor(30, 24, 28),
                                          V2(0.4f, 0.38f), V2(0.6f, 0.92f)));
    IconCapsule(Canvas, V2(0.3f, 0.6f), V2(0.46f, 0.64f), 0.03f, Solid(IconColor(255, 60, 40)));
    IconCapsule(Canvas, V2(0.7f, 0.6f), V2(0.54f, 0.64f), 0.03f, Solid(IconColor(255, 60, 40)));
    IconGlow(Canvas, V2(0.38f, 0.62f), 0.1f, IconColor(255, 120, 80, 220));
    IconGlow(Canvas, V2(0.62f, 0.62f), 0.1f, IconColor(255, 120, 80, 220));
    IconCapsule(Canvas, V2(0.5f, 0.66f), V2(0.5f, 0.86f), 0.02f, Solid(IconColor(20, 14, 16)));
}

// NOTE(zoubir): a badge in the bottom-right corner saying how a talent
// changes its spell: "more" an arrow up, "wider" two rings
internal void
BerserkerIconBadge(icon_canvas *Canvas, v4 Color)
{
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.17f, Solid(IconColor(16, 18, 26)));
    IconCircle(Canvas, V2(0.78f, 0.78f), 0.14f, Solid(Color));
}

internal void
BerserkerIconBadgeMore(icon_canvas *Canvas)
{
    BerserkerIconBadge(Canvas, IconColor(255, 205, 90));
    IconTriangle(Canvas, V2(0.78f, 0.67f), V2(0.86f, 0.79f), V2(0.7f, 0.79f), Solid(IconColor(30, 24, 10)));
    IconCapsule(Canvas, V2(0.78f, 0.78f), V2(0.78f, 0.88f), 0.025f, Solid(IconColor(30, 24, 10)));
}

internal void
BerserkerIconBadgeWider(icon_canvas *Canvas)
{
    BerserkerIconBadge(Canvas, IconColor(255, 160, 80));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.05f, 0.02f, Solid(IconColor(30, 16, 8)));
    IconArc(Canvas, V2(0.78f, 0.78f), 0.1f, 0.02f, Solid(IconColor(30, 16, 8)));
}

// NOTE(zoubir): Brutality: two great axes crossed over a red glow
internal void
PaintBrutalityIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.45f), 0.5f, IconColor(230, 40, 40, 160));
    BerserkerIconAxe(Canvas, V2(0.16f, 0.9f), V2(0.62f, 0.3f), 1.f, 0.3f);
    BerserkerIconAxe(Canvas, V2(0.84f, 0.9f), V2(0.38f, 0.3f), -1.f, 0.3f);
}

// NOTE(zoubir): Unbridled Wrath: a heart of blood with rage boiling up
// out of it
internal void
PaintUnbridledWrathIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.5f, IconColor(255, 40, 30, 170));
    for(u32 Tongue = 0; Tongue < 3; Tongue++)
    {
        float X = 0.32f + 0.18f * (float)Tongue;
        float Top = Tongue == 1 ? 0.06f : 0.2f;
        IconTriangle(Canvas, V2(X - 0.1f, 0.56f), V2(X + 0.1f, 0.56f), V2(X, Top),
                     Gradient(IconColor(255, 200, 120), IconColor(220, 30, 20), V2(X, Top), V2(X, 0.56f)));
    }
    IconCircle(Canvas, V2(0.4f, 0.62f), 0.15f, Gradient(IconColor(255, 90, 90), IconColor(140, 0, 20),
                                                        V2(0.34f, 0.52f), V2(0.46f, 0.76f)));
    IconCircle(Canvas, V2(0.6f, 0.62f), 0.15f, Gradient(IconColor(255, 90, 90), IconColor(140, 0, 20),
                                                        V2(0.54f, 0.52f), V2(0.66f, 0.76f)));
    IconTriangle(Canvas, V2(0.26f, 0.68f), V2(0.74f, 0.68f), V2(0.5f, 0.92f),
                 Gradient(IconColor(210, 40, 50), IconColor(120, 0, 20), V2(0.5f, 0.68f), V2(0.5f, 0.92f)));
    IconCircle(Canvas, V2(0.36f, 0.57f), 0.035f, Solid(IconColor(255, 210, 210)));
}

internal void PaintSweepingStrikesIcon(icon_canvas *C) { PaintBerserkerCleaveIcon(C); BerserkerIconBadgeWider(C); }
internal void PaintMassacreIcon(icon_canvas *C) { PaintBerserkerExecuteIcon(C); BerserkerIconBadgeMore(C); }
internal void PaintBloodthirstTalentIcon(icon_canvas *C) { PaintBerserkerBloodthirstIcon(C); }

global_variable role_icon_painter *BerserkerIconPainters[ROLE_KEYS] =
{
    PaintBerserkerLeapIcon, PaintBerserkerWhirlwindIcon, 0,
    PaintBerserkerBerserkIcon, PaintBerserkerExecuteIcon, 0,
    PaintBerserkerCleaveIcon,
};
global_variable talent_icon_painter *BerserkerTalentIconPainters[ROLE_TALENTS] =
{
    PaintBrutalityIcon, PaintBloodthirstTalentIcon, PaintUnbridledWrathIcon, PaintSweepingStrikesIcon,
    PaintBerserkerBerserkIcon, PaintMassacreIcon,
    PaintStatVitalityIcon, PaintStatLifestealIcon, 0, PaintStatDamageIcon,
    PaintStatArmorIcon, 0,
};
