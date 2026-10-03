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
