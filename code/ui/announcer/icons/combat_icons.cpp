/* Combat icons (announce_icons.cpp): the fight's own pictures. Crossed
   swords for a fight, a skull for kills, a horned skull with burning eyes
   for a boss, a drop of blood for first blood, a flame for a killing
   spree, a target for a boss nearly down, a gravestone for a wipe. All
   painted with icon_canvas.cpp, coordinates 0..1, Y down. */

// NOTE(zoubir): one sword from its hilt to its tip: an edged blade lit
// on one side, a gold guard across it, a grip and a pommel behind
internal void
PaintAnnounceBlade(icon_canvas *Canvas, v2 Hilt, v2 Tip)
{
    v2 Along = (1.f / Length(Tip - Hilt)) * (Tip - Hilt);
    v2 Across = V2(-Along.Y, Along.X);
    v2 Near = Tip - 0.12f * Along;
    float Half = 0.05f;
    v2 Lit[4] = {Hilt + Half * Across, Near + Half * Across, Tip, Hilt};
    v2 Dark[4] = {Hilt, Tip, Near - Half * Across, Hilt - Half * Across};
    IconPolygon(Canvas, Lit, 4, Gradient(IconColor(240, 245, 252), IconColor(200, 210, 228),
                                        Hilt, Tip));
    IconPolygon(Canvas, Dark, 4, Gradient(IconColor(126, 138, 162), IconColor(176, 186, 206),
                                         Hilt, Tip));
    v4 Gold = IconColor(244, 196, 76);
    v4 GoldDark = IconColor(160, 104, 30);
    IconCapsule(Canvas, Hilt + 0.14f * Across, Hilt - 0.14f * Across, 0.045f,
                Gradient(Gold, GoldDark, Hilt + 0.14f * Across, Hilt - 0.14f * Across));
    IconCapsule(Canvas, Hilt, Hilt - 0.13f * Along, 0.035f, Solid(IconColor(110, 64, 40)));
    IconCircle(Canvas, Hilt - 0.16f * Along, 0.05f, Gradient(Gold, GoldDark,
                                                            Hilt - 0.2f * Along, Hilt));
}

internal void
PaintAnnounceSwords(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.45f), 0.46f, IconColor(255, 120, 80, 90));
    PaintAnnounceBlade(Canvas, V2(0.27f, 0.73f), V2(0.86f, 0.14f));
    PaintAnnounceBlade(Canvas, V2(0.73f, 0.73f), V2(0.14f, 0.14f));
    IconSparkle(Canvas, V2(0.5f, 0.44f), 0.1f, Solid(IconColor(255, 255, 255)));
}

// NOTE(zoubir): a skull in Bone, its eyes in Eyes (dark sockets, or a
// glow for a boss)
internal void
PaintAnnounceSkullShape(icon_canvas *Canvas, v4 Bone, v4 BoneDark, v4 Eyes)
{
    icon_paint Fill = Gradient(Bone, BoneDark, V2(0.5f, 0.14f), V2(0.5f, 0.86f));
    IconCircle(Canvas, V2(0.5f, 0.43f), 0.29f, Fill);
    IconCapsule(Canvas, V2(0.4f, 0.68f), V2(0.6f, 0.68f), 0.13f, Fill);
    v4 Hole = IconColor(28, 20, 26);
    IconCircle(Canvas, V2(0.38f, 0.47f), 0.085f, Solid(Hole));
    IconCircle(Canvas, V2(0.62f, 0.47f), 0.085f, Solid(Hole));
    if (Eyes.W > 0.f)
    {
        IconGlow(Canvas, V2(0.38f, 0.47f), 0.1f, Eyes);
        IconGlow(Canvas, V2(0.62f, 0.47f), 0.1f, Eyes);
        IconCircle(Canvas, V2(0.38f, 0.47f), 0.03f, Solid(IconColor(255, 230, 160)));
        IconCircle(Canvas, V2(0.62f, 0.47f), 0.03f, Solid(IconColor(255, 230, 160)));
    }
    IconTriangle(Canvas, V2(0.5f, 0.55f), V2(0.45f, 0.64f), V2(0.55f, 0.64f), Solid(Hole));
    for(u32 Tooth = 0; Tooth < 3; Tooth++)
    {
        float X = 0.43f + 0.07f * (float)Tooth;
        IconCapsule(Canvas, V2(X, 0.72f), V2(X, 0.79f), 0.012f, Solid(Hole));
    }
}

internal void
PaintAnnounceSkull(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 210, 110, 80));
    PaintAnnounceSkullShape(Canvas, IconColor(250, 244, 226), IconColor(196, 182, 156),
                            IconColor(0, 0, 0, 0));
}

internal void
PaintAnnounceBoss(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.48f, IconColor(255, 70, 50, 110));
    v4 Horn = IconColor(232, 214, 180);
    v4 HornTip = IconColor(120, 40, 36);
    IconTriangle(Canvas, V2(0.24f, 0.36f), V2(0.38f, 0.24f), V2(0.08f, 0.06f),
                 Gradient(Horn, HornTip, V2(0.3f, 0.3f), V2(0.08f, 0.06f)));
    IconTriangle(Canvas, V2(0.76f, 0.36f), V2(0.62f, 0.24f), V2(0.92f, 0.06f),
                 Gradient(Horn, HornTip, V2(0.7f, 0.3f), V2(0.92f, 0.06f)));
    PaintAnnounceSkullShape(Canvas, IconColor(236, 226, 206), IconColor(150, 130, 116),
                            IconColor(255, 80, 40, 230));
}

internal void
PaintAnnounceBlood(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.56f), 0.44f, IconColor(255, 40, 40, 90));
    icon_paint Red = Gradient(IconColor(255, 86, 76), IconColor(150, 10, 20),
                              V2(0.35f, 0.2f), V2(0.65f, 0.86f));
    IconTriangle(Canvas, V2(0.5f, 0.1f), V2(0.27f, 0.54f), V2(0.73f, 0.54f), Red);
    IconCircle(Canvas, V2(0.5f, 0.62f), 0.24f, Red);
    IconCapsule(Canvas, V2(0.38f, 0.56f), V2(0.4f, 0.68f), 0.03f,
                Solid(IconColor(255, 230, 230, 200)));
}

internal void
PaintAnnounceFlame(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.46f, IconColor(255, 140, 40, 110));
    icon_paint Outer = Gradient(IconColor(255, 200, 70), IconColor(220, 50, 20),
                                V2(0.5f, 0.9f), V2(0.5f, 0.1f));
    IconTriangle(Canvas, V2(0.5f, 0.06f), V2(0.26f, 0.62f), V2(0.74f, 0.62f), Outer);
    IconTriangle(Canvas, V2(0.74f, 0.2f), V2(0.56f, 0.6f), V2(0.8f, 0.62f), Outer);
    IconTriangle(Canvas, V2(0.24f, 0.28f), V2(0.2f, 0.64f), V2(0.44f, 0.6f), Outer);
    IconCircle(Canvas, V2(0.5f, 0.66f), 0.25f, Outer);
    icon_paint Inner = Gradient(IconColor(255, 252, 210), IconColor(255, 196, 60),
                                V2(0.5f, 0.88f), V2(0.5f, 0.4f));
    IconTriangle(Canvas, V2(0.5f, 0.36f), V2(0.37f, 0.7f), V2(0.63f, 0.7f), Inner);
    IconCircle(Canvas, V2(0.5f, 0.72f), 0.13f, Inner);
}

internal void
PaintAnnounceTarget(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 60, 50, 90));
    icon_paint Red = Solid(IconColor(240, 70, 56));
    icon_paint White = Solid(IconColor(252, 240, 236));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.31f, 0.075f, Red);
    IconArc(Canvas, V2(0.5f, 0.5f), 0.19f, 0.065f, White);
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.08f, Red);
    IconCapsule(Canvas, V2(0.5f, 0.04f), V2(0.5f, 0.2f), 0.025f, White);
    IconCapsule(Canvas, V2(0.5f, 0.8f), V2(0.5f, 0.96f), 0.025f, White);
    IconCapsule(Canvas, V2(0.04f, 0.5f), V2(0.2f, 0.5f), 0.025f, White);
    IconCapsule(Canvas, V2(0.8f, 0.5f), V2(0.96f, 0.5f), 0.025f, White);
}

internal void
PaintAnnounceGrave(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.46f, IconColor(170, 60, 70, 90));
    icon_paint Stone = Gradient(IconColor(196, 200, 212), IconColor(104, 108, 126),
                                V2(0.3f, 0.14f), V2(0.7f, 0.86f));
    IconCircle(Canvas, V2(0.5f, 0.4f), 0.26f, Stone);
    v2 Body[4] = {V2(0.24f, 0.4f), V2(0.76f, 0.4f), V2(0.76f, 0.84f), V2(0.24f, 0.84f)};
    IconPolygon(Canvas, Body, 4, Stone);
    icon_paint Carve = Solid(IconColor(60, 62, 78));
    IconCapsule(Canvas, V2(0.5f, 0.3f), V2(0.5f, 0.66f), 0.035f, Carve);
    IconCapsule(Canvas, V2(0.37f, 0.42f), V2(0.63f, 0.42f), 0.035f, Carve);
    IconCapsule(Canvas, V2(0.12f, 0.87f), V2(0.88f, 0.87f), 0.04f,
                Solid(IconColor(84, 70, 60)));
}
