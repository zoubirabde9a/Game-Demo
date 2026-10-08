/* Glory icons (announce_icons.cpp): progress and reward. A crown for the
   lead and a boss brought down, a trophy for a level cleared, a check for
   a room cleared or a vote passed, a gate for a dungeon room, an hourglass
   for a countdown. Painted with icon_canvas.cpp, coordinates 0..1, Y
   down. */

internal void
PaintAnnounceCrown(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.52f), 0.46f, IconColor(255, 210, 80, 100));
    icon_paint Gold = Gradient(IconColor(255, 228, 120), IconColor(196, 120, 24),
                               V2(0.5f, 0.2f), V2(0.5f, 0.8f));
    IconTriangle(Canvas, V2(0.17f, 0.56f), V2(0.17f, 0.26f), V2(0.4f, 0.56f), Gold);
    IconTriangle(Canvas, V2(0.33f, 0.56f), V2(0.5f, 0.18f), V2(0.67f, 0.56f), Gold);
    IconTriangle(Canvas, V2(0.6f, 0.56f), V2(0.83f, 0.26f), V2(0.83f, 0.56f), Gold);
    v2 Band[4] = {V2(0.17f, 0.52f), V2(0.83f, 0.52f), V2(0.83f, 0.78f), V2(0.17f, 0.78f)};
    IconPolygon(Canvas, Band, 4, Gold);
    IconCapsule(Canvas, V2(0.2f, 0.71f), V2(0.8f, 0.71f), 0.018f,
                Solid(IconColor(150, 84, 20)));
    icon_paint Pearl = Solid(IconColor(255, 250, 230));
    IconCircle(Canvas, V2(0.17f, 0.25f), 0.045f, Pearl);
    IconCircle(Canvas, V2(0.5f, 0.17f), 0.05f, Pearl);
    IconCircle(Canvas, V2(0.83f, 0.25f), 0.045f, Pearl);
    IconCircle(Canvas, V2(0.5f, 0.61f), 0.065f,
               Gradient(IconColor(255, 110, 120), IconColor(170, 20, 50),
                        V2(0.46f, 0.56f), V2(0.54f, 0.66f)));
    IconCircle(Canvas, V2(0.31f, 0.62f), 0.04f, Solid(IconColor(90, 190, 255)));
    IconCircle(Canvas, V2(0.69f, 0.62f), 0.04f, Solid(IconColor(90, 190, 255)));
}

internal void
PaintAnnounceTrophy(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.42f), 0.48f, IconColor(255, 210, 80, 110));
    icon_paint Gold = Gradient(IconColor(255, 232, 130), IconColor(190, 112, 20),
                               V2(0.3f, 0.16f), V2(0.7f, 0.6f));
    IconArc(Canvas, V2(0.28f, 0.32f), 0.11f, 0.05f, Gold, 0.5f * Pi32, 1.5f * Pi32);
    IconArc(Canvas, V2(0.72f, 0.32f), 0.11f, 0.05f, Gold, -0.5f * Pi32, 0.5f * Pi32);
    v2 Cup[7] = {V2(0.27f, 0.16f), V2(0.73f, 0.16f), V2(0.7f, 0.36f), V2(0.62f, 0.5f),
                 V2(0.5f, 0.56f), V2(0.38f, 0.5f), V2(0.3f, 0.36f)};
    IconPolygon(Canvas, Cup, 7, Gold);
    IconCapsule(Canvas, V2(0.5f, 0.56f), V2(0.5f, 0.72f), 0.04f, Gold);
    v2 Base[4] = {V2(0.33f, 0.73f), V2(0.67f, 0.73f), V2(0.71f, 0.85f), V2(0.29f, 0.85f)};
    IconPolygon(Canvas, Base, 4, Gradient(IconColor(140, 86, 50), IconColor(84, 48, 30),
                                         V2(0.5f, 0.73f), V2(0.5f, 0.85f)));
    IconCapsule(Canvas, V2(0.36f, 0.22f), V2(0.39f, 0.4f), 0.022f,
                Solid(IconColor(255, 250, 220, 190)));
    IconSparkle(Canvas, V2(0.58f, 0.32f), 0.09f, Solid(IconColor(255, 255, 255)));
}

internal void
PaintAnnounceCheck(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(90, 220, 120, 90));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.36f,
               Gradient(IconColor(110, 220, 130), IconColor(30, 130, 70),
                        V2(0.5f, 0.14f), V2(0.5f, 0.86f)));
    icon_paint White = Solid(IconColor(250, 255, 250));
    IconCapsule(Canvas, V2(0.33f, 0.51f), V2(0.45f, 0.64f), 0.055f, White);
    IconCapsule(Canvas, V2(0.45f, 0.64f), V2(0.68f, 0.37f), 0.055f, White);
}

internal void
PaintAnnounceCross(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(240, 70, 60, 90));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.36f,
               Gradient(IconColor(240, 96, 84), IconColor(150, 28, 30),
                        V2(0.5f, 0.14f), V2(0.5f, 0.86f)));
    icon_paint White = Solid(IconColor(255, 246, 244));
    IconCapsule(Canvas, V2(0.37f, 0.37f), V2(0.63f, 0.63f), 0.055f, White);
    IconCapsule(Canvas, V2(0.63f, 0.37f), V2(0.37f, 0.63f), 0.055f, White);
}

// NOTE(zoubir): a stone archway with a dark way through and its portcullis
internal void
PaintAnnounceGate(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.46f, IconColor(190, 130, 255, 100));
    icon_paint Stone = Gradient(IconColor(204, 196, 220), IconColor(110, 100, 132),
                                V2(0.3f, 0.12f), V2(0.7f, 0.88f));
    IconCircle(Canvas, V2(0.5f, 0.4f), 0.28f, Stone);
    v2 Wall[4] = {V2(0.22f, 0.4f), V2(0.78f, 0.4f), V2(0.78f, 0.88f), V2(0.22f, 0.88f)};
    IconPolygon(Canvas, Wall, 4, Stone);
    icon_paint Dark = Gradient(IconColor(70, 40, 110), IconColor(20, 12, 34),
                               V2(0.5f, 0.24f), V2(0.5f, 0.88f));
    IconCircle(Canvas, V2(0.5f, 0.43f), 0.18f, Dark);
    v2 Way[4] = {V2(0.32f, 0.43f), V2(0.68f, 0.43f), V2(0.68f, 0.88f), V2(0.32f, 0.88f)};
    IconPolygon(Canvas, Way, 4, Dark);
    IconGlow(Canvas, V2(0.5f, 0.66f), 0.16f, IconColor(200, 140, 255, 140));
    icon_paint Bar = Solid(IconColor(150, 140, 166));
    for(u32 Index = 0; Index < 3; Index++)
    {
        float X = 0.4f + 0.1f * (float)Index;
        IconCapsule(Canvas, V2(X, 0.3f), V2(X, 0.6f), 0.016f, Bar);
    }
    IconCapsule(Canvas, V2(0.34f, 0.46f), V2(0.66f, 0.46f), 0.016f, Bar);
}

internal void
PaintAnnounceHourglass(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.44f, IconColor(140, 200, 255, 80));
    icon_paint Glass = Solid(IconColor(190, 225, 255, 110));
    IconTriangle(Canvas, V2(0.28f, 0.18f), V2(0.72f, 0.18f), V2(0.5f, 0.5f), Glass);
    IconTriangle(Canvas, V2(0.5f, 0.5f), V2(0.28f, 0.82f), V2(0.72f, 0.82f), Glass);
    icon_paint Sand = Gradient(IconColor(255, 226, 130), IconColor(220, 160, 50),
                               V2(0.5f, 0.3f), V2(0.5f, 0.82f));
    IconTriangle(Canvas, V2(0.4f, 0.36f), V2(0.6f, 0.36f), V2(0.5f, 0.5f), Sand);
    IconTriangle(Canvas, V2(0.5f, 0.62f), V2(0.32f, 0.8f), V2(0.68f, 0.8f), Sand);
    IconCapsule(Canvas, V2(0.5f, 0.5f), V2(0.5f, 0.66f), 0.01f, Sand);
    icon_paint Wood = Gradient(IconColor(176, 110, 60), IconColor(100, 58, 30),
                               V2(0.5f, 0.1f), V2(0.5f, 0.9f));
    IconCapsule(Canvas, V2(0.22f, 0.16f), V2(0.78f, 0.16f), 0.045f, Wood);
    IconCapsule(Canvas, V2(0.22f, 0.84f), V2(0.78f, 0.84f), 0.045f, Wood);
}
