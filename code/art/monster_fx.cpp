/* Monster effect sprites: one row of SHOT_FRAMES frames per shot style,
   SHOT_FRAME_SIZE pixels square, drawn by code like the monster sheets.
   Shots are seen from above at an angle, so they are drawn round with a
   bright core and a trail; the game mirrors them for leftward shots. */

typedef void shot_draw_function(sprite_canvas *Canvas, u32 Frame);

internal void
DrawShot_Ember(sprite_canvas *Canvas, u32 Frame)
{
    color_ramp Fire = Ramp(ART_RGB(150, 34, 14), ART_RGB(230, 90, 20),
                           ART_RGB(255, 170, 40), ART_RGB(255, 240, 160));
    float Flicker = (Frame % 2) ? 0.5f : 0.f;
    // NOTE(zoubir): licks of flame trailing to the left
    for(u32 Lick = 0; Lick < 3; Lick++)
    {
        float Y = 6.f + 2.f * Lick + (float)((Frame + Lick) % 2);
        float Reach = 3.f + (float)((Frame + Lick * 2) % 3);
        FillLimb(Canvas, V2(8.f, Y + 1.f), V2(8.f - Reach - 2.f, Y), 2.f, 0.6f,
                 Fire, -0.3f);
    }
    FillBlob(Canvas, 9.f, 8.f, 4.f + Flicker, 4.f + Flicker, Fire, 0.15f);
    FillDot(Canvas, 9.5f, 7.5f, 1.6f, Fire.C[3]);
    OutlineFrame(Canvas, ART_RGB(70, 16, 8));
}

internal void
DrawShot_Bile(sprite_canvas *Canvas, u32 Frame)
{
    color_ramp Bile = Ramp(ART_RGB(50, 100, 26), ART_RGB(110, 190, 36),
                           ART_RGB(170, 236, 64), ART_RGB(226, 255, 150));
    float Wobble = (Frame % 2) ? 0.6f : -0.6f;
    FillBlob(Canvas, 8.f, 8.f, 4.5f + Wobble, 4.5f - Wobble, Bile, 0.1f);
    FillDot(Canvas, 3.5f - (float)(Frame % 2), 9.f, 1.3f, Bile.C[1]);
    FillDot(Canvas, 2.f, 6.5f + (float)(Frame % 3), 0.8f, Bile.C[1]);
    FillDot(Canvas, 9.f, 6.5f, 1.2f, Bile.C[3]);
    OutlineFrame(Canvas, ART_RGB(20, 40, 10));
}

internal void
DrawShot_Spine(sprite_canvas *Canvas, u32 Frame)
{
    color_ramp Bone = Ramp(ART_RGB(110, 80, 70), ART_RGB(176, 140, 120),
                           ART_RGB(220, 196, 170), ART_RGB(250, 240, 220));
    color_ramp Venom = Ramp(ART_RGB(70, 20, 80), ART_RGB(130, 40, 140),
                            ART_RGB(190, 80, 200), ART_RGB(240, 150, 250));
    // NOTE(zoubir): a tumbling barb, turning a quarter each frame
    float Angle = 0.5f * Pi32 * (float)Frame / (float)SHOT_FRAMES;
    v2 Axis = V2(Cos(Angle), Sin(Angle));
    v2 Center = V2(8.f, 8.f);
    FillLimb(Canvas, Center - 5.f * Axis, Center + 5.f * Axis, 1.6f, 0.5f, Bone);
    FillBlob(Canvas, Center.X - 2.5f * Axis.X, Center.Y - 2.5f * Axis.Y,
             2.f, 2.f, Venom, 0.1f);
    OutlineFrame(Canvas, ART_RGB(30, 14, 24));
}

global_variable shot_draw_function *ShotDrawFunctions[ShotStyle_Count] =
{
    DrawShot_Ember,
    DrawShot_Bile,
    DrawShot_Spine,
};

// NOTE(zoubir): one texture per style, SHOT_FRAMES frames in a row
internal void
BuildShotSheet(monster_shot_style Style, u32 *Pixels)
{
    u32 Width = SHOT_FRAME_SIZE * SHOT_FRAMES;
    ZeroSize(Pixels, Width * SHOT_FRAME_SIZE * sizeof(u32));
    for(u32 Frame = 0; Frame < SHOT_FRAMES; Frame++)
    {
        sprite_canvas Canvas = CanvasFrame(Pixels, Width, SHOT_FRAME_SIZE,
                                           Frame, 0);
        ShotDrawFunctions[Style](&Canvas, Frame);
    }
}

/* Ground hazards: HAZARD_FRAMES frames per style, HAZARD_FRAME_SIZE
   square, drawn as a flat patch in the middle of the frame height so the
   patch sits on the ground like the telegraph rings. */

typedef void hazard_draw_function(sprite_canvas *Canvas, u32 Frame);

#define HAZARD_CENTER (0.5f * (float)HAZARD_FRAME_SIZE)
#define HAZARD_RX (0.47f * (float)HAZARD_FRAME_SIZE)
#define HAZARD_RY (0.28f * (float)HAZARD_FRAME_SIZE)

internal void
DrawHazard_Bile(sprite_canvas *Canvas, u32 Frame)
{
    color_ramp Bile = Ramp(ART_RGB(40, 80, 24), ART_RGB(76, 130, 34),
                           ART_RGB(120, 180, 48), ART_RGB(190, 230, 110));
    // NOTE(zoubir): a flat pool: dark rim, lighter middle, one glossy
    // streak. FillBlob would shade it like a ball
    FillFlatEllipse(Canvas, HAZARD_CENTER, HAZARD_CENTER, HAZARD_RX * 0.8f,
                    HAZARD_RY * 0.8f, Bile.C[1]);
    FillFlatEllipse(Canvas, HAZARD_CENTER - 13.f, HAZARD_CENTER + 3.f, 7.f, 4.f,
                    Bile.C[1]);
    FillFlatEllipse(Canvas, HAZARD_CENTER + 14.f, HAZARD_CENTER - 3.f, 6.f, 3.5f,
                    Bile.C[1]);
    FillFlatEllipse(Canvas, HAZARD_CENTER + 1.f, HAZARD_CENTER + 1.f,
                    HAZARD_RX * 0.6f, HAZARD_RY * 0.55f, Bile.C[2]);
    FillFlatEllipse(Canvas, HAZARD_CENTER - 5.f, HAZARD_CENTER - 3.f, 5.f, 1.2f,
                    Bile.C[3]);
    // NOTE(zoubir): bubbles swell and pop in turn
    float BubbleX[3] = {-8.f, 6.f, 1.f};
    float BubbleY[3] = {1.f, 3.f, -3.f};
    for(u32 Bubble = 0; Bubble < 3; Bubble++)
    {
        u32 Age = (Frame + Bubble) % HAZARD_FRAMES;
        if (Age < 3)
        {
            FillBlob(Canvas, HAZARD_CENTER + BubbleX[Bubble],
                     HAZARD_CENTER + BubbleY[Bubble],
                     1.f + (float)Age, 1.f + 0.7f * (float)Age, Bile, 0.2f);
        }
    }
    OutlineFrame(Canvas, ART_RGB(20, 40, 12));
}

internal void
DrawHazard_Web(sprite_canvas *Canvas, u32 Frame)
{
    color_ramp Silk = Ramp(ART_RGB(150, 150, 160), ART_RGB(200, 200, 210),
                           ART_RGB(230, 230, 236), ART_RGB(255, 255, 255));
    v2 Center = V2(HAZARD_CENTER, HAZARD_CENTER);
    u32 Spokes = 8;
    for(u32 Spoke = 0; Spoke < Spokes; Spoke++)
    {
        float Angle = 2.f * Pi32 * (float)Spoke / (float)Spokes + 0.2f;
        v2 Tip = Center + V2(HAZARD_RX * Cos(Angle), HAZARD_RY * Sin(Angle));
        FillLimb(Canvas, Center, Tip, 0.6f, 0.5f, Silk, -0.1f);
    }
    // NOTE(zoubir): rings connect neighbouring spokes
    for(u32 Ring = 1; Ring <= 3; Ring++)
    {
        float Scale = 0.3f * (float)Ring;
        for(u32 Spoke = 0; Spoke < Spokes; Spoke++)
        {
            float A0 = 2.f * Pi32 * (float)Spoke / (float)Spokes + 0.2f;
            float A1 = 2.f * Pi32 * (float)(Spoke + 1) / (float)Spokes + 0.2f;
            v2 P0 = Center + Scale * V2(HAZARD_RX * Cos(A0), HAZARD_RY * Sin(A0));
            v2 P1 = Center + Scale * V2(HAZARD_RX * Cos(A1), HAZARD_RY * Sin(A1));
            FillLimb(Canvas, P0, P1, 0.5f, 0.5f, Silk, -0.2f);
        }
    }
    // NOTE(zoubir): a glint running around the outer ring
    float GlintAngle = 2.f * Pi32 * (float)Frame / (float)HAZARD_FRAMES;
    FillDot(Canvas, Center.X + 0.9f * HAZARD_RX * Cos(GlintAngle),
            Center.Y + 0.9f * HAZARD_RY * Sin(GlintAngle), 1.2f, Silk.C[3]);
}

internal void
DrawHazard_Embers(sprite_canvas *Canvas, u32 Frame)
{
    color_ramp Ash = Ramp(ART_RGB(26, 20, 20), ART_RGB(44, 34, 30),
                          ART_RGB(64, 50, 44), ART_RGB(88, 70, 60));
    color_ramp Coal = Ramp(ART_RGB(150, 30, 10), ART_RGB(230, 90, 20),
                           ART_RGB(255, 170, 40), ART_RGB(255, 240, 160));
    FillBlob(Canvas, HAZARD_CENTER, HAZARD_CENTER, HAZARD_RX * 0.85f,
             HAZARD_RY * 0.8f, Ash, -0.1f);
    float CoalX[5] = {-10.f, -3.f, 5.f, 12.f, 0.f};
    float CoalY[5] = {1.f, -4.f, 2.f, -2.f, 5.f};
    for(u32 Ember = 0; Ember < 5; Ember++)
    {
        float Glow = ((Frame + Ember) % 2) ? 0.25f : -0.1f;
        FillBlob(Canvas, HAZARD_CENTER + CoalX[Ember], HAZARD_CENTER + CoalY[Ember],
                 2.5f, 1.8f, Coal, Glow);
    }
    // NOTE(zoubir): one flame licking up from a different coal each frame
    u32 Lit = Frame % 5;
    v2 Base = V2(HAZARD_CENTER + CoalX[Lit], HAZARD_CENTER + CoalY[Lit]);
    FillLimb(Canvas, Base, Base + V2(0.5f, -7.f), 2.f, 0.5f, Coal, 0.2f);
    OutlineFrame(Canvas, ART_RGB(16, 10, 8));
}

internal void
DrawHazard_Goo(sprite_canvas *Canvas, u32 Frame)
{
    color_ramp Goo = Ramp(ART_RGB(30, 26, 60), ART_RGB(56, 48, 104),
                          ART_RGB(88, 80, 150), ART_RGB(150, 160, 220));
    // NOTE(zoubir): a splat: center pool plus tendrils thrown outward
    FillFlatEllipse(Canvas, HAZARD_CENTER, HAZARD_CENTER, HAZARD_RX * 0.65f,
                    HAZARD_RY * 0.7f, Goo.C[1]);
    for(u32 Tendril = 0; Tendril < 6; Tendril++)
    {
        float Angle = 2.f * Pi32 * (float)Tendril / 6.f + 0.4f;
        float Reach = (Tendril % 2) ? 0.95f : 0.8f;
        v2 Tip = V2(HAZARD_CENTER + Reach * HAZARD_RX * Cos(Angle),
                    HAZARD_CENTER + Reach * HAZARD_RY * Sin(Angle));
        FillFlatEllipse(Canvas, Tip.X, Tip.Y, 3.f, 2.f, Goo.C[1]);
        FillFlatEllipse(Canvas, 0.5f * (Tip.X + HAZARD_CENTER),
                        0.5f * (Tip.Y + HAZARD_CENTER), 4.f, 2.5f, Goo.C[1]);
    }
    FillFlatEllipse(Canvas, HAZARD_CENTER + 1.f, HAZARD_CENTER + 1.f,
                    HAZARD_RX * 0.45f, HAZARD_RY * 0.45f, Goo.C[2]);
    // NOTE(zoubir): the sheen slides across as the goo settles
    float Sheen = -6.f + 4.f * (float)Frame;
    FillFlatEllipse(Canvas, HAZARD_CENTER + Sheen, HAZARD_CENTER - 3.f, 4.f, 1.f,
                    Goo.C[3]);
    OutlineFrame(Canvas, ART_RGB(14, 10, 30));
}

global_variable hazard_draw_function *HazardDrawFunctions[HazardStyle_Count] =
{
    DrawHazard_Bile,
    DrawHazard_Web,
    DrawHazard_Embers,
    DrawHazard_Goo,
};

internal void
BuildHazardSheet(monster_hazard_style Style, u32 *Pixels)
{
    u32 Width = HAZARD_FRAME_SIZE * HAZARD_FRAMES;
    ZeroSize(Pixels, Width * HAZARD_FRAME_SIZE * sizeof(u32));
    for(u32 Frame = 0; Frame < HAZARD_FRAMES; Frame++)
    {
        sprite_canvas Canvas = CanvasFrame(Pixels, Width, HAZARD_FRAME_SIZE,
                                           Frame, 0);
        HazardDrawFunctions[Style](&Canvas, Frame);
    }
}
