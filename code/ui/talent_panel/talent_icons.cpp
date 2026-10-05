/* Talent icons: one per talent, painted in code like the ability bar's
   (ui/ability_icons/icon_canvas.cpp) into one texture on first use. An
   ability talent reuses its ability's icon; the passives have theirs
   here. TalentIconPainters lists them in talent_id order. */

// NOTE(zoubir): Swift Flames: a fireball streaking right, speed lines
// behind it
internal void
PaintSwiftFlamesIcon(icon_canvas *Canvas)
{
    v4 Orange = IconColor(255, 140, 40);
    v4 Yellow = IconColor(255, 225, 110);
    v4 White = IconColor(255, 250, 230);
    IconGlow(Canvas, V2(0.66f, 0.5f), 0.4f, IconColor(255, 120, 30, 140));
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Y = 0.36f + 0.14f * (float)Line;
        float Start = 0.12f + 0.06f * (float)(Line % 2);
        IconCapsule(Canvas, V2(Start, Y), V2(0.5f, Y), 0.025f,
                    Gradient(IconColor(255, 160, 60, 0), Yellow, V2(Start, Y), V2(0.5f, Y)));
    }
    IconCircle(Canvas, V2(0.66f, 0.5f), 0.19f,
               Gradient(Yellow, IconColor(210, 50, 20), V2(0.58f, 0.4f), V2(0.78f, 0.66f)));
    IconCircle(Canvas, V2(0.64f, 0.47f), 0.1f, Gradient(White, Orange, V2(0.6f, 0.42f), V2(0.7f, 0.55f)));
}

// NOTE(zoubir): Twin Flame: two fireballs leaving side by side, fanning out
internal void
PaintTwinFlameIcon(icon_canvas *Canvas)
{
    v4 Yellow = IconColor(255, 220, 90);
    v4 Deep = IconColor(205, 45, 20);
    v4 White = IconColor(255, 250, 225);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 110, 30, 120));
    v2 Orbs[2] = {V2(0.66f, 0.3f), V2(0.66f, 0.7f)};
    for(u32 Orb = 0; Orb < 2; Orb++)
    {
        v2 P = Orbs[Orb];
        IconCapsule(Canvas, V2(0.16f, 0.5f), P, 0.06f,
                    Gradient(IconColor(255, 120, 30, 0), IconColor(255, 140, 40, 220),
                             V2(0.16f, 0.5f), P));
        IconCircle(Canvas, P, 0.15f, Gradient(Yellow, Deep, P - V2(0.08f, 0.08f), P + V2(0.1f, 0.1f)));
        IconCircle(Canvas, P - V2(0.03f, 0.03f), 0.06f, Solid(White));
    }
}

// NOTE(zoubir): Pyre: a tall flame on crossed logs
internal void
PaintPyreIcon(icon_canvas *Canvas)
{
    v4 Wood = IconColor(120, 70, 40);
    v4 Red = IconColor(220, 50, 20);
    v4 Orange = IconColor(255, 140, 30);
    v4 Yellow = IconColor(255, 230, 120);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(255, 90, 20, 150));
    IconCapsule(Canvas, V2(0.22f, 0.84f), V2(0.78f, 0.7f), 0.05f, Solid(Wood));
    IconCapsule(Canvas, V2(0.22f, 0.7f), V2(0.78f, 0.84f), 0.05f,
                Gradient(IconColor(150, 90, 50), Wood, V2(0.2f, 0.7f), V2(0.8f, 0.84f)));
    // NOTE(zoubir): a flame is a round base under a point, three times
    // over, each smaller and brighter
    float Sizes[3] = {0.22f, 0.15f, 0.08f};
    v4 Colours[3] = {Red, Orange, Yellow};
    for(u32 Layer = 0; Layer < 3; Layer++)
    {
        float R = Sizes[Layer];
        v2 Base = V2(0.5f, 0.66f - 0.02f * (float)Layer);
        IconCircle(Canvas, Base, R, Solid(Colours[Layer]));
        IconTriangle(Canvas, Base + V2(-0.92f * R, -0.35f * R), Base + V2(0.92f * R, -0.35f * R),
                     Base + V2(0.08f * R, -2.6f * R), Solid(Colours[Layer]));
    }
}

// NOTE(zoubir): Fleet Foot: a white wing with speed lines under it
internal void
PaintFleetFootIcon(icon_canvas *Canvas)
{
    v4 White = IconColor(240, 250, 255);
    v4 Sky = IconColor(120, 220, 245);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.44f, IconColor(80, 210, 240, 120));
    for(u32 Feather = 0; Feather < 4; Feather++)
    {
        float Lift = 0.1f * (float)Feather;
        v2 Root = V2(0.62f, 0.56f - 0.03f * (float)Feather);
        v2 Tip = V2(0.18f + 0.06f * (float)Feather, 0.62f - 0.12f - Lift);
        IconCapsule(Canvas, Root, Tip, 0.055f - 0.008f * (float)Feather,
                    Gradient(White, Sky, Root, Tip));
    }
    IconCircle(Canvas, V2(0.66f, 0.56f), 0.07f, Solid(White));
    for(u32 Line = 0; Line < 3; Line++)
    {
        float Y = 0.72f + 0.07f * (float)Line;
        IconCapsule(Canvas, V2(0.3f + 0.08f * (float)Line, Y), V2(0.84f, Y), 0.018f,
                    Gradient(IconColor(120, 220, 245, 0), Sky, V2(0.3f, Y), V2(0.84f, Y)));
    }
}

// NOTE(zoubir): Momentum: two arrows chasing each other round a bolt
internal void
PaintMomentumIcon(icon_canvas *Canvas)
{
    v4 Cyan = IconColor(100, 230, 245);
    v4 Gold = IconColor(255, 220, 90);
    v2 Centre = V2(0.5f, 0.5f);
    IconGlow(Canvas, Centre, 0.46f, IconColor(60, 200, 240, 120));
    for(u32 Half = 0; Half < 2; Half++)
    {
        float Start = (float)Half * Pi32 + 0.2f;
        float End = Start + 0.75f * Pi32;
        IconArc(Canvas, Centre, 0.33f, 0.06f, Solid(Cyan), Start, End);
        v2 Tip = Centre + 0.33f * V2(Cos(End), Sin(End));
        v2 Along = V2(-Sin(End), Cos(End));
        v2 Out = V2(Cos(End), Sin(End));
        IconTriangle(Canvas, Tip + 0.12f * Along, Tip - 0.05f * Along + 0.1f * Out,
                     Tip - 0.05f * Along - 0.1f * Out, Solid(Cyan));
    }
    v2 Bolt[4] = {V2(0.54f, 0.24f), V2(0.40f, 0.52f), V2(0.50f, 0.52f), V2(0.46f, 0.76f)};
    IconTriangle(Canvas, Bolt[0], Bolt[1], Bolt[2], Solid(Gold));
    IconTriangle(Canvas, V2(0.58f, 0.46f), V2(0.44f, 0.46f), Bolt[3], Solid(Gold));
}

// NOTE(zoubir): Ward: a glowing hexagon, a smaller one inside it
internal void
PaintWardIcon(icon_canvas *Canvas)
{
    v4 Gold = IconColor(255, 215, 110);
    v4 Pale = IconColor(255, 245, 210);
    v2 Centre = V2(0.5f, 0.5f);
    IconGlow(Canvas, Centre, 0.48f, IconColor(255, 200, 80, 140));
    v2 Outer[6];
    v2 Inner[6];
    for(u32 Corner = 0; Corner < 6; Corner++)
    {
        float Angle = (float)Corner * Pi32 / 3.f - 0.5f * Pi32;
        Outer[Corner] = Centre + 0.36f * V2(Cos(Angle), Sin(Angle));
        Inner[Corner] = Centre + 0.27f * V2(Cos(Angle), Sin(Angle));
    }
    IconPolygon(Canvas, Outer, 6, Gradient(Pale, Gold, V2(0.5f, 0.14f), V2(0.5f, 0.86f)));
    IconPolygon(Canvas, Inner, 6, Gradient(IconColor(120, 80, 20), IconColor(60, 35, 10),
                                           V2(0.5f, 0.2f), V2(0.5f, 0.8f)));
    for(u32 Corner = 0; Corner < 6; Corner++)
    {
        IconCapsule(Canvas, Inner[Corner], Inner[(Corner + 1) % 6], 0.012f,
                    Solid(IconColor(255, 230, 150, 160)));
    }
    IconSparkle(Canvas, Centre, 0.13f, Solid(Pale));
}

// NOTE(zoubir): Second Wind: a green wisp rising through two chevrons
internal void
PaintSecondWindIcon(icon_canvas *Canvas)
{
    v4 Green = IconColor(120, 240, 170);
    v4 Pale = IconColor(225, 255, 235);
    IconGlow(Canvas, V2(0.5f, 0.5f), 0.46f, IconColor(80, 230, 150, 130));
    IconArc(Canvas, V2(0.42f, 0.62f), 0.2f, 0.05f, Solid(Green), 0.5f * Pi32, 1.6f * Pi32);
    IconArc(Canvas, V2(0.58f, 0.42f), 0.16f, 0.045f, Solid(Pale), -0.5f * Pi32, 0.6f * Pi32);
    for(u32 Chevron = 0; Chevron < 2; Chevron++)
    {
        float Y = 0.3f + 0.16f * (float)Chevron;
        IconCapsule(Canvas, V2(0.62f, Y + 0.1f), V2(0.74f, Y), 0.03f, Solid(Green));
        IconCapsule(Canvas, V2(0.74f, Y), V2(0.86f, Y + 0.1f), 0.03f, Solid(Green));
    }
    IconCircle(Canvas, V2(0.42f, 0.84f), 0.04f, Solid(Pale));
}

typedef void talent_icon_painter(icon_canvas *Canvas);

global_variable talent_icon_painter *TalentIconPainters[Talent_Count] =
{
    PaintFireballIcon,
    PaintSwiftFlamesIcon,
    PaintLaunchIcon,
    PaintShockwaveIcon,
    PaintFrostNovaIcon,
    PaintTwinFlameIcon,
    PaintPyreIcon,

    PaintDashIcon,
    PaintFleetFootIcon,
    PaintBlinkIcon,
    PaintSwordIcon,
    PaintPushIcon,
    PaintSlamIcon,
    PaintMomentumIcon,

    PaintShieldIcon,
    PaintWardIcon,
    PaintRewindWorldIcon,
    PaintRewindSelfIcon,
    PaintGravityWellIcon,
    PaintRewindBubbleIcon,
    PaintSecondWindIcon,
};

#define TALENT_ICON_SIZE 96
#define TALENT_ATLAS_COLUMNS 8
#define TALENT_ATLAS_ROWS ((Talent_Count + TALENT_ATLAS_COLUMNS - 1) / TALENT_ATLAS_COLUMNS)

// NOTE(zoubir): every talent's icon in one texture, cell N for talent N
internal u32
BuildTalentIconAtlas(open_gl *OpenGL, memory_arena *Scratch)
{
    u32 Width = TALENT_ATLAS_COLUMNS * TALENT_ICON_SIZE;
    u32 Height = TALENT_ATLAS_ROWS * TALENT_ICON_SIZE;
    temporary_memory Temp = BeginTemporaryMemory(Scratch);
    u32 *Pixels = AllocateArray(Scratch, Width * Height, u32);
    memset(Pixels, 0, Width * Height * sizeof(u32));
    icon_canvas Canvas = {};
    Canvas.Size = TALENT_ICON_SIZE;
    Canvas.Pixels = AllocateArray(Scratch, TALENT_ICON_SIZE * TALENT_ICON_SIZE, v4);
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        memset(Canvas.Pixels, 0, TALENT_ICON_SIZE * TALENT_ICON_SIZE * sizeof(v4));
        TalentIconPainters[Talent](&Canvas);
        u32 Column = Talent % TALENT_ATLAS_COLUMNS;
        u32 Row = Talent / TALENT_ATLAS_COLUMNS;
        IconFinish(&Canvas, Pixels + Row * TALENT_ICON_SIZE * Width + Column * TALENT_ICON_SIZE,
                   Width);
    }
    u32 Result = RenderUploadTexture(OpenGL, Width, Height, Pixels, true);
    EndTemporaryMemory(Temp);
    return Result;
}

// NOTE(zoubir): talent Talent's cell, as AbilityIconUvs gives a slot's
inline v4
TalentIconUvs(u32 Talent)
{
    float Width = (float)(TALENT_ATLAS_COLUMNS * TALENT_ICON_SIZE);
    float Height = (float)(TALENT_ATLAS_ROWS * TALENT_ICON_SIZE);
    float Left = (float)((Talent % TALENT_ATLAS_COLUMNS) * TALENT_ICON_SIZE) + 0.5f;
    float Top = (float)((Talent / TALENT_ATLAS_COLUMNS) * TALENT_ICON_SIZE) + 0.5f;
    float Size = (float)TALENT_ICON_SIZE - 1.f;
    v4 Result = V4(Left / Width, 1.f - (Top + Size) / Height, (Left + Size) / Width,
                   1.f - Top / Height);
    return Result;
}
