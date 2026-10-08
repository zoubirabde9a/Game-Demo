/* Stat talent icons (sim/dungeon/role_stats.cpp): one picture per stat,
   shared by every class's stat talents, so a class's tree reads "more
   health" the same way in each. Included by role_icons.cpp before the
   classes' icon rows, which list them. */

// NOTE(zoubir): Damage: a blade pointing up, sparks off its tip
internal void
PaintStatDamageIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.45f), 0.46f, IconColor(255, 110, 60, 130));
    v2 Blade[4] = {V2(0.5f, 0.1f), V2(0.6f, 0.26f), V2(0.56f, 0.66f), V2(0.44f, 0.66f)};
    IconPolygon(Canvas, Blade, 4, Gradient(IconColor(250, 250, 255), IconColor(140, 145, 165),
                                           V2(0.45f, 0.2f), V2(0.58f, 0.66f)));
    v2 Edge[4] = {V2(0.5f, 0.1f), V2(0.6f, 0.26f), V2(0.56f, 0.66f), V2(0.5f, 0.66f)};
    IconPolygon(Canvas, Edge, 4, Solid(IconColor(255, 255, 255, 110)));
    IconCapsule(Canvas, V2(0.32f, 0.68f), V2(0.68f, 0.68f), 0.035f, Solid(IconColor(220, 170, 70)));
    IconCapsule(Canvas, V2(0.5f, 0.7f), V2(0.5f, 0.88f), 0.04f, Solid(IconColor(110, 70, 40)));
    IconSparkle(Canvas, V2(0.68f, 0.16f), 0.08f, Solid(IconColor(255, 220, 120)));
    IconSparkle(Canvas, V2(0.3f, 0.24f), 0.05f, Solid(IconColor(255, 170, 90)));
}

// NOTE(zoubir): Armor: a round shield with a steel boss
internal void
PaintStatArmorIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(140, 170, 220, 120));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.34f, Gradient(IconColor(200, 210, 230), IconColor(80, 90, 115),
                                                       V2(0.35f, 0.25f), V2(0.65f, 0.8f)));
    IconArc(Canvas, V2(0.5f, 0.5f), 0.31f, 0.04f, Solid(IconColor(230, 190, 90)));
    IconCircle(Canvas, V2(0.5f, 0.5f), 0.11f, Gradient(IconColor(250, 250, 255), IconColor(120, 125, 145),
                                                       V2(0.45f, 0.42f), V2(0.56f, 0.6f)));
    IconCapsule(Canvas, V2(0.5f, 0.22f), V2(0.5f, 0.36f), 0.025f, Solid(IconColor(70, 80, 100)));
    IconCapsule(Canvas, V2(0.5f, 0.64f), V2(0.5f, 0.78f), 0.025f, Solid(IconColor(70, 80, 100)));
}

// NOTE(zoubir): Vitality: a red heart with a light on it
internal void
PaintStatVitalityIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 70, 90, 130));
    icon_paint Red = Gradient(IconColor(255, 120, 130), IconColor(170, 20, 40),
                              V2(0.4f, 0.25f), V2(0.6f, 0.8f));
    IconCircle(Canvas, V2(0.37f, 0.4f), 0.16f, Red);
    IconCircle(Canvas, V2(0.63f, 0.4f), 0.16f, Red);
    IconTriangle(Canvas, V2(0.22f, 0.46f), V2(0.78f, 0.46f), V2(0.5f, 0.82f), Red);
    IconCircle(Canvas, V2(0.34f, 0.35f), 0.05f, Solid(IconColor(255, 230, 235, 200)));
}

// NOTE(zoubir): Haste: an hourglass with its sand running
internal void
PaintStatHasteIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(120, 200, 255, 120));
    v4 Wood = IconColor(150, 100, 60);
    IconCapsule(Canvas, V2(0.28f, 0.16f), V2(0.72f, 0.16f), 0.04f, Solid(Wood));
    IconCapsule(Canvas, V2(0.28f, 0.84f), V2(0.72f, 0.84f), 0.04f, Solid(Wood));
    icon_paint Glass = Solid(IconColor(200, 230, 255, 150));
    IconTriangle(Canvas, V2(0.32f, 0.2f), V2(0.68f, 0.2f), V2(0.5f, 0.5f), Glass);
    IconTriangle(Canvas, V2(0.32f, 0.8f), V2(0.68f, 0.8f), V2(0.5f, 0.5f), Glass);
    icon_paint Sand = Solid(IconColor(255, 210, 110));
    IconTriangle(Canvas, V2(0.4f, 0.3f), V2(0.6f, 0.3f), V2(0.5f, 0.46f), Sand);
    IconTriangle(Canvas, V2(0.36f, 0.8f), V2(0.64f, 0.8f), V2(0.5f, 0.64f), Sand);
    IconCapsule(Canvas, V2(0.5f, 0.48f), V2(0.5f, 0.66f), 0.012f, Sand);
}

// NOTE(zoubir): Healing: a green cross in a soft light
internal void
PaintStatHealingIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(110, 240, 150, 140));
    icon_paint Green = Gradient(IconColor(200, 255, 210), IconColor(40, 170, 80),
                                V2(0.4f, 0.25f), V2(0.6f, 0.8f));
    IconCapsule(Canvas, V2(0.5f, 0.2f), V2(0.5f, 0.8f), 0.1f, Green);
    IconCapsule(Canvas, V2(0.2f, 0.5f), V2(0.8f, 0.5f), 0.1f, Green);
    IconSparkle(Canvas, V2(0.76f, 0.24f), 0.07f, Solid(IconColor(240, 255, 240)));
}

// NOTE(zoubir): Swiftness: a winged boot's wing, swept back
internal void
PaintStatSwiftnessIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(140, 230, 255, 120));
    for(u32 Feather = 0; Feather < 4; Feather++)
    {
        float Y = 0.3f + 0.12f * (float)Feather;
        float Start = 0.72f - 0.05f * (float)Feather;
        IconCapsule(Canvas, V2(Start, Y), V2(0.2f + 0.06f * (float)Feather, Y - 0.08f), 0.045f,
                    Gradient(IconColor(250, 252, 255), IconColor(150, 200, 235),
                             V2(Start, Y), V2(0.2f, Y)));
    }
    IconCircle(Canvas, V2(0.74f, 0.42f), 0.1f, Solid(IconColor(230, 190, 90)));
}

// NOTE(zoubir): Lifesteal: a red drop falling into a curl of light
internal void
PaintStatLifestealIcon(icon_canvas *Canvas)
{
    IconGlow(Canvas, V2(0.5f, 0.55f), 0.46f, IconColor(200, 40, 70, 140));
    icon_paint Blood = Gradient(IconColor(255, 120, 130), IconColor(140, 10, 30),
                                V2(0.42f, 0.3f), V2(0.58f, 0.8f));
    IconTriangle(Canvas, V2(0.5f, 0.12f), V2(0.34f, 0.5f), V2(0.66f, 0.5f), Blood);
    IconCircle(Canvas, V2(0.5f, 0.56f), 0.17f, Blood);
    IconCircle(Canvas, V2(0.45f, 0.52f), 0.04f, Solid(IconColor(255, 220, 225, 200)));
    IconArc(Canvas, V2(0.5f, 0.6f), 0.3f, 0.035f, Solid(IconColor(255, 170, 180)),
            0.15f * Pi32, 0.85f * Pi32);
}
