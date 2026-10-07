/* Stone tiles: the full tiles of rough and laid stone, eight cells each
   (see the top of terrain_art.cpp): crags of loose stones over gravel
   (grey, or black basalt with embers), flagstones in one of eight
   patterns, and the stone walls. Painted with the brushes in
   ground_paint.cpp; included by ground_tiles.cpp. */

// NOTE(zoubir): crags are rough high ground you can walk on: loose
// stones of every size over gravel, each lit from the top left and
// casting a little shadow. Basalt crags are black, with embers in the gaps
internal void
DrawCragTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    bool32 Basalt = Kind == TerrainKind_BasaltWall;
    color_ramp Gravel = Basalt ?
        Ramp(ART_RGB(30, 27, 33), ART_RGB(40, 36, 44),
             ART_RGB(50, 45, 54), ART_RGB(64, 58, 68)) :
        Ramp(ART_RGB(92, 88, 82), ART_RGB(106, 102, 95),
             ART_RGB(120, 115, 107), ART_RGB(138, 132, 122));
    color_ramp Stone = Basalt ?
        Ramp(ART_RGB(20, 18, 24), ART_RGB(44, 40, 50),
             ART_RGB(66, 60, 72), ART_RGB(96, 88, 100)) :
        Ramp(ART_RGB(64, 60, 58), ART_RGB(110, 105, 98),
             ART_RGB(140, 134, 125), ART_RGB(178, 172, 160));
    u32 Shadow = Basalt ? ART_RGB(8, 6, 10) : ART_RGB(40, 38, 40);
    FillGround(Canvas, Gravel, Seed, Variant, 0.f, 0.6f);
    for(u32 Grit = 0; Grit < 10; Grit++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 1, Grit, 1, &X, &Y);
        PutPixel(Canvas, X, Y, DetailRoll(Detail, Grit, 2) ? Gravel.C[0] : Gravel.C[3]);
    }
    if (Basalt && (Variant & 1))
    {
        i32 X, Y;
        ScatterSpot(Detail + 2, 0, 4, &X, &Y);
        PutGlow(Canvas, (float)X + 0.5f, (float)Y + 0.5f, 3.f, ART_RGB(200, 70, 30), 0.55f);
        PutPixel(Canvas, X, Y, ART_RGB(250, 150, 60));
    }
    // NOTE(zoubir): every stone keeps clear of its tile's edges (a
    // neighbour of another variant would cut it), so wide margins left an
    // empty band along every border and the stones fell into a grid. Big
    // stones only on two variants in eight, small ones up to the edge, and
    // loose grit filling the bands
    for(u32 Grain = 0; Grain < 8; Grain++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 3, Grain, 1, &X, &Y);
        PutPebble(Canvas, X, Y, 1 + (i32)DetailRoll(Detail + 4, Grain, 2), 1,
                  Stone.C[3], Stone.C[2], Shadow);
    }
    bool32 HasBig = Variant == 1 || Variant == 4;
    u32 Stones = 6 + DetailRoll(Detail, 17, 3);
    for(u32 Index = 0; Index < Stones; Index++)
    {
        bool32 Big = HasBig && Index == 0;
        float RX = Big ? 4.5f + (float)DetailRoll(Detail, Index, 3) :
            1.5f + (float)DetailRoll(Detail, Index, 2);
        float RY = Big ? RX - 1.5f : RX - 0.5f;
        i32 Margin = (i32)RX + (Big ? 3 : 1);
        i32 X, Y;
        ScatterSpot(Detail + 7 + Index, Index, Margin, &X, &Y);
        PutStone(Canvas, (float)X, (float)Y, RX, RY, Stone, Shadow);
    }
}

// NOTE(zoubir): flagstones laid in each tile in one of eight patterns;
// every stone its own shade, bevelled light along its top and left,
// darker along its bottom and right, mortar between
internal void
DrawFlagstoneTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Flag = Ramp(ART_RGB(116, 112, 106), ART_RGB(132, 128, 120),
                           ART_RGB(148, 143, 134), ART_RGB(166, 160, 150));
    u32 Mortar = ART_RGB(78, 74, 70);
    // NOTE(zoubir): where the joints fall: SplitX splits the top half (and
    // the bottom one when SplitBottomX is set), SplitY the whole tile
    i32 SplitY[8] = {16, 0, 16, 16, 12, 20, 0, 16};
    i32 SplitTop[8] = {16, 12, 0, 20, 16, 0, 0, 10};
    i32 SplitBottom[8] = {16, 0, 12, 0, 22, 14, 20, 0};
    // NOTE(zoubir): a kind has TERRAIN_CELLS (16) cells and there are eight
    // layouts; cells past the eighth read past the tables and laid random
    // stones that changed from run to run. They reuse a layout, with their
    // own shades and wear
    u32 Layout = Variant % ArrayCount(SplitY);
    i32 Row = 0;
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            bool32 Lower = SplitY[Layout] && Y >= SplitY[Layout];
            i32 Split = Lower ? SplitBottom[Layout] : SplitTop[Layout];
            i32 X0 = (Split && X >= Split) ? Split : 0;
            i32 X1 = (Split && X < Split) ? Split : 32;
            i32 Y0 = Lower ? SplitY[Layout] : 0;
            i32 Y1 = (SplitY[Layout] && !Lower) ? SplitY[Layout] : 32;
            Row = Lower ? 1 : 0;
            bool32 Joint = X == X0 || Y == Y0;
            bool32 Corner = (X == X0 + 1 || X == X1 - 1) && (Y == Y0 + 1 || Y == Y1 - 1);
            if (Joint || Corner)
            {
                PutPixel(Canvas, X, Y, Mortar);
                continue;
            }
            float Shade = (float)(HashLattice(Detail, X0, Row) >> 24) / 255.f;
            float Value = 0.42f + 0.3f * (Shade - 0.5f) +
                0.5f * (GroundTexture(Seed, Variant, X, Y) - 0.5f);
            if (X == X0 + 1 || Y == Y0 + 1)
            {
                Value += 0.3f;
            }
            else if (X == X1 - 1 || Y == Y1 - 1)
            {
                Value -= 0.3f;
            }
            PutPixel(Canvas, X, Y, ShadeRamp(&Flag, Value, X, Y));
        }
    }
    // NOTE(zoubir): wear: a chip or crack, and moss in a joint
    i32 X, Y;
    ScatterSpot(Detail + 1, Variant, 5, &X, &Y);
    if (Variant & 1)
    {
        PutCrack(Canvas, Detail, X, Y, 5, ART_RGB(92, 88, 82), Flag.C[3]);
    }
    if (Variant == 2 || Variant == 7)
    {
        PutPixel(Canvas, 0, Y, ART_RGB(78, 104, 52));
        PutPixel(Canvas, 0, Y + 1, ART_RGB(94, 124, 60));
        PutPixel(Canvas, 1, Y + 1, ART_RGB(70, 94, 48));
    }
}

internal void
DrawStoneWallTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Stone = Ramp(ART_RGB(54, 50, 56), ART_RGB(78, 74, 80),
                            ART_RGB(104, 100, 104), ART_RGB(136, 132, 134));
    FillGround(Canvas, Stone, Seed, Variant, 0.05f, 0.7f);
    // NOTE(zoubir): courses of bricks, offset every other row
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            i32 Course = Y / 8;
            i32 Offset = (Course % 2) ? 8 : 0;
            if ((Y % 8) == 0 || ((X + Offset) % 16) == 0)
            {
                PutPixel(Canvas, X, Y, Stone.C[0]);
            }
            else if ((Y % 8) == 1)
            {
                PutPixel(Canvas, X, Y, Stone.C[3]);
            }
        }
    }
}
