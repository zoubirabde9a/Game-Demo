/* People icons (announce_icons.cpp): who comes and goes, and how they
   are. A player with a green plus (joined) or a grey minus (left), a
   heart (back up) and a broken heart (down), a ballot box for a vote,
   signal bars for the server and the same bars struck through when it is
   lost. Painted with icon_canvas.cpp, coordinates 0..1, Y down. */

// NOTE(zoubir): a head and shoulders, with a round badge at its lower
// right in Badge carrying a plus (Plus) or a minus
internal void
PaintAnnouncePerson(icon_canvas *Canvas, v4 Badge, v4 BadgeDark, bool32 Plus)
{
    icon_paint Body = Gradient(IconColor(226, 232, 244), IconColor(140, 150, 176),
                               V2(0.4f, 0.12f), V2(0.4f, 0.9f));
    IconCircle(Canvas, V2(0.42f, 0.32f), 0.15f, Body);
    IconCapsule(Canvas, V2(0.26f, 0.74f), V2(0.58f, 0.74f), 0.16f, Body);
    IconCircle(Canvas, V2(0.72f, 0.68f), 0.19f, Solid(IconColor(20, 23, 30)));
    IconCircle(Canvas, V2(0.72f, 0.68f), 0.16f,
               Gradient(Badge, BadgeDark, V2(0.72f, 0.52f), V2(0.72f, 0.84f)));
    icon_paint White = Solid(IconColor(255, 255, 255));
    IconCapsule(Canvas, V2(0.64f, 0.68f), V2(0.8f, 0.68f), 0.03f, White);
    if (Plus)
    {
        IconCapsule(Canvas, V2(0.72f, 0.6f), V2(0.72f, 0.76f), 0.03f, White);
    }
}

internal void
PaintAnnounceJoined(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(90, 220, 120, 70));
    PaintAnnouncePerson(Canvas, IconColor(110, 220, 130), IconColor(30, 130, 70), true);
}

internal void
PaintAnnounceLeft(icon_canvas *Canvas)
{
    PaintAnnouncePerson(Canvas, IconColor(170, 176, 190), IconColor(90, 96, 112), false);
}

// NOTE(zoubir): a heart in Paint
internal void
PaintAnnounceHeartShape(icon_canvas *Canvas, icon_paint Paint)
{
    IconCircle(Canvas, V2(0.35f, 0.38f), 0.18f, Paint);
    IconCircle(Canvas, V2(0.65f, 0.38f), 0.18f, Paint);
    IconTriangle(Canvas, V2(0.18f, 0.45f), V2(0.82f, 0.45f), V2(0.5f, 0.84f), Paint);
}

internal void
PaintAnnounceHeart(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 110, 140, 100));
    PaintAnnounceHeartShape(Canvas, Gradient(IconColor(255, 120, 140), IconColor(190, 30, 60),
                                             V2(0.3f, 0.2f), V2(0.6f, 0.84f)));
    IconCapsule(Canvas, V2(0.28f, 0.32f), V2(0.34f, 0.27f), 0.03f,
                Solid(IconColor(255, 230, 236, 220)));
    IconSparkle(Canvas, V2(0.68f, 0.3f), 0.1f, Solid(IconColor(255, 255, 255)));
}

internal void
PaintAnnounceHeartBroken(icon_canvas *Canvas)
{
    PaintAnnounceHeartShape(Canvas, Gradient(IconColor(170, 60, 70), IconColor(90, 20, 34),
                                             V2(0.3f, 0.2f), V2(0.6f, 0.84f)));
    icon_paint Crack = Solid(IconColor(24, 14, 20));
    IconCapsule(Canvas, V2(0.5f, 0.26f), V2(0.44f, 0.44f), 0.025f, Crack);
    IconCapsule(Canvas, V2(0.44f, 0.44f), V2(0.56f, 0.58f), 0.025f, Crack);
    IconCapsule(Canvas, V2(0.56f, 0.58f), V2(0.5f, 0.8f), 0.025f, Crack);
}

internal void
PaintAnnounceBallot(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 210, 80, 80));
    v2 Paper[4] = {V2(0.33f, 0.12f), V2(0.67f, 0.12f), V2(0.67f, 0.56f), V2(0.33f, 0.56f)};
    IconPolygon(Canvas, Paper, 4, Gradient(IconColor(255, 255, 250), IconColor(214, 214, 206),
                                          V2(0.5f, 0.12f), V2(0.5f, 0.56f)));
    icon_paint Tick = Solid(IconColor(40, 160, 80));
    IconCapsule(Canvas, V2(0.41f, 0.31f), V2(0.48f, 0.39f), 0.03f, Tick);
    IconCapsule(Canvas, V2(0.48f, 0.39f), V2(0.6f, 0.22f), 0.03f, Tick);
    v2 Box[4] = {V2(0.18f, 0.48f), V2(0.82f, 0.48f), V2(0.82f, 0.86f), V2(0.18f, 0.86f)};
    IconPolygon(Canvas, Box, 4, Gradient(IconColor(250, 206, 80), IconColor(176, 110, 30),
                                        V2(0.5f, 0.48f), V2(0.5f, 0.86f)));
    IconCapsule(Canvas, V2(0.3f, 0.56f), V2(0.7f, 0.56f), 0.025f,
                Solid(IconColor(70, 40, 16)));
}

// NOTE(zoubir): four bars rising; Lost greys them and strikes them out
internal void
PaintAnnounceSignalBars(icon_canvas *Canvas, bool32 Lost)
{
    v4 Lit = Lost ? IconColor(130, 136, 150) : IconColor(110, 220, 130);
    v4 Base = Lost ? IconColor(80, 84, 96) : IconColor(40, 140, 70);
    for(u32 Index = 0; Index < 4; Index++)
    {
        float X = 0.24f + 0.17f * (float)Index;
        float Top = 0.68f - 0.15f * (float)Index;
        IconCapsule(Canvas, V2(X, Top), V2(X, 0.8f), 0.05f,
                    Gradient(Lit, Base, V2(X, Top), V2(X, 0.8f)));
    }
    if (Lost)
    {
        IconCapsule(Canvas, V2(0.16f, 0.2f), V2(0.84f, 0.86f), 0.05f,
                    Solid(IconColor(20, 23, 30)));
        IconCapsule(Canvas, V2(0.16f, 0.2f), V2(0.84f, 0.86f), 0.03f,
                    Solid(IconColor(240, 80, 64)));
    }
}

internal void
PaintAnnounceSignal(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.6f), 0.44f, IconColor(90, 220, 120, 70));
    PaintAnnounceSignalBars(Canvas, false);
}

internal void
PaintAnnounceSignalLost(icon_canvas *Canvas)
{
    PaintAnnounceSignalBars(Canvas, true);
}
