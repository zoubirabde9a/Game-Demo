/* Cold tiles: the full tiles of snow and ice, eight cells each (see the
   top of terrain_art.cpp). Snow is wind-carved ridges with blue in the
   hollows and a few sparkles; ice is clear blue with glossy streaks, a
   crack and trapped air.
   Painted with the brushes in ground_paint.cpp; included by
   ground_tiles.cpp. */

// NOTE(zoubir): drifts with blue shade in their hollows and a few
// sparkles of frost
internal void
DrawSnowTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Snow = Ramp(ART_RGB(196, 208, 230), ART_RGB(214, 223, 240),
                           ART_RGB(230, 236, 248), ART_RGB(244, 247, 255));
    FillGround(Canvas, Snow, Seed, Variant, 0.08f, 0.55f, true);
    // NOTE(zoubir): broad ridges the wind carved, bright on the crest and a
    // cold blue in the hollow behind it
    PutWindRipples(Canvas, Seed, Variant, 2, 1, ART_RGB(250, 252, 255), 0.6f,
                   ART_RGB(168, 184, 220), 0.45f);
    for(u32 Sparkle = 0; Sparkle < 3; Sparkle++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 2, Sparkle, 2, &X, &Y);
        PutPixel(Canvas, X, Y, ART_RGB(255, 255, 255));
        if (DetailRoll(Detail, Sparkle, 3) == 0)
        {
            PutPixel(Canvas, X - 1, Y, ART_RGB(220, 236, 255));
            PutPixel(Canvas, X + 1, Y, ART_RGB(220, 236, 255));
            PutPixel(Canvas, X, Y - 1, ART_RGB(220, 236, 255));
            PutPixel(Canvas, X, Y + 1, ART_RGB(220, 236, 255));
        }
    }
}

internal void
DrawIceTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Ice = Ramp(ART_RGB(136, 186, 212), ART_RGB(158, 206, 228),
                          ART_RGB(180, 222, 240), ART_RGB(214, 238, 250));
    FillGround(Canvas, Ice, Seed, Variant, 0.f, 0.55f);
    // NOTE(zoubir): glossy streaks rising to the right, a crack, and air
    // trapped in the ice
    for(u32 Streak = 0; Streak < 2; Streak++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 1, Streak + 2 * Variant, 8, &X, &Y);
        i32 Length = 4 + (i32)DetailRoll(Detail, Streak, 4);
        for(i32 Step = 0; Step < Length; Step++)
        {
            PutPixel(Canvas, X + Step, Y - Step, ART_RGB(240, 250, 255));
            PutPixel(Canvas, X + Step + 1, Y - Step, Ice.C[3]);
        }
    }
    i32 X, Y;
    ScatterSpot(Detail + 2, Variant, 6, &X, &Y);
    PutCrack(Canvas, Detail, X, Y, 8, ART_RGB(112, 162, 192), ART_RGB(236, 248, 255));
    for(u32 Bubble = 0; Bubble < 3; Bubble++)
    {
        ScatterSpot(Detail + 3, Bubble + 3 * Variant, 2, &X, &Y);
        PutPixel(Canvas, X, Y, Ice.C[3]);
        PutPixel(Canvas, X + 1, Y + 1, Ice.C[0]);
    }
}
