/* Ground tiles: the full tiles of every natural ground kind, eight cells
   each (see the top of terrain_art.cpp). The base is soft, low-contrast
   noise that joins across tiles (ground_paint.cpp); the eye catches the
   details on top: tufts, pebbles, stones, cracks, flecks and embers,
   placed differently in each variant. Larger patches of light and colour
   come from the world tint the client multiplies in (client/ground/).
   Liquids are in liquid_tiles.cpp, hazards in terrain_hazard_art.cpp. */

internal void
DrawGrassTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Grass = Ramp(ART_RGB(62, 108, 44), ART_RGB(76, 126, 52),
                            ART_RGB(92, 144, 60), ART_RGB(120, 168, 74));
    FillGround(Canvas, Grass, Seed, Variant, 0.f, 0.55f);
    // NOTE(zoubir): the sward: short blades all over, a shaded root and a
    // lit tip, thicker where a noise says so, so the field reads as grass
    // and not as a painted green. Per pixel, so nothing lines up at a
    // tile's edge; blades never reach above the tile's top row
    for(i32 Y = 1; Y < TERRAIN_TILE_PIXELS; Y++)
    {
        for(i32 X = 0; X < TERRAIN_TILE_PIXELS; X++)
        {
            float Thick = GroundNoise(Seed + 21, Variant, X, Y, 8, 8);
            u32 Roll = HashLattice(Detail + 5, X, Y) >> 24;
            if ((float)Roll > 255.f * (0.55f + 0.35f * (1.f - Thick)))
            {
                BlendPixel(Canvas, X, Y, Grass.C[0], 0.6f);
                BlendPixel(Canvas, X, Y - 1, Roll & 1 ? Grass.C[3] : Grass.C[2], 0.7f);
            }
        }
    }
    // NOTE(zoubir): darker clumps of clover, then tufts of blades over them
    for(u32 Clump = 0; Clump < 4; Clump++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 1, Clump, 2, &X, &Y);
        PutPixel(Canvas, X, Y, Grass.C[0]);
        PutPixel(Canvas, X + 1, Y, Grass.C[0]);
        PutPixel(Canvas, X, Y - 1, Grass.C[1]);
        PutPixel(Canvas, X + 1, Y + 1, Grass.C[1]);
    }
    u32 Tufts = 5 + DetailRoll(Detail, 99, 4);
    for(u32 Tuft = 0; Tuft < Tufts; Tuft++)
    {
        i32 X, Y;
        ScatterSpot(Detail, Tuft, 4, &X, &Y);
        PutTuft(Canvas, HashLattice(Detail, (i32)Tuft, 3), X, Y, &Grass);
    }
    for(u32 Speck = 0; Speck < 5; Speck++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 2, Speck, 1, &X, &Y);
        PutPixel(Canvas, X, Y, Grass.C[3]);
    }
    // NOTE(zoubir): a flower on three variants in eight
    if (Variant == 1 || Variant == 4 || Variant == 6)
    {
        i32 X, Y;
        ScatterSpot(Detail + 3, 0, 4, &X, &Y);
        u32 Petal = Variant == 4 ? ART_RGB(236, 150, 196) :
            Variant == 6 ? ART_RGB(170, 190, 250) : ART_RGB(246, 238, 210);
        PutPixel(Canvas, X, Y - 1, Petal);
        PutPixel(Canvas, X - 1, Y, Petal);
        PutPixel(Canvas, X + 1, Y, Petal);
        PutPixel(Canvas, X, Y, ART_RGB(250, 214, 90));
        PutPixel(Canvas, X, Y + 1, Grass.C[0]);
        PutPixel(Canvas, X + 1, Y + 1, Grass.C[0]);
    }
}

internal void
DrawSoilTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    bool32 Wet = Kind == TerrainKind_Mud;
    color_ramp Soil = Wet ?
        Ramp(ART_RGB(56, 42, 30), ART_RGB(70, 53, 38),
             ART_RGB(84, 65, 46), ART_RGB(102, 82, 60)) :
        Ramp(ART_RGB(116, 86, 56), ART_RGB(134, 101, 67),
             ART_RGB(152, 118, 80), ART_RGB(174, 140, 98));
    FillGround(Canvas, Soil, Seed, Variant, 0.f, 0.55f);
    if (Wet)
    {
        // NOTE(zoubir): wheel ruts, and a puddle on half the variants with
        // the sky caught along its top
        for(u32 Rut = 0; Rut < 2; Rut++)
        {
            i32 X, Y;
            ScatterSpot(Detail + 1, Rut, 3, &X, &Y);
            for(i32 Step = 0; Step < 8; Step++)
            {
                PutPixel(Canvas, X + Step, Y + (Step > 4 ? 1 : 0), Soil.C[0]);
                PutPixel(Canvas, X + Step, Y + 1 + (Step > 4 ? 1 : 0), Soil.C[3]);
            }
        }
        if (Variant & 1)
        {
            i32 X, Y;
            ScatterSpot(Detail + 2, 0, 7, &X, &Y);
            i32 Half = 3 + (i32)DetailRoll(Detail, 7, 3);
            for(i32 DY = -1; DY <= 1; DY++)
            {
                i32 Reach = DY == 0 ? Half : Half - 2;
                for(i32 DX = -Reach; DX <= Reach; DX++)
                {
                    u32 Color = DY < 0 ? ART_RGB(96, 104, 110) : ART_RGB(42, 40, 40);
                    PutPixel(Canvas, X + DX, Y + DY, Color);
                }
            }
            PutPixel(Canvas, X - Half + 2, Y, ART_RGB(150, 160, 166));
            PutPixel(Canvas, X - Half + 3, Y, ART_RGB(120, 128, 134));
        }
        for(u32 Glint = 0; Glint < 4; Glint++)
        {
            i32 X, Y;
            ScatterSpot(Detail + 3, Glint, 2, &X, &Y);
            PutPixel(Canvas, X, Y, ART_RGB(118, 110, 96));
        }
        return;
    }
    // NOTE(zoubir): pebbles, a crack on some variants, a sprig of green
    u32 Pebbles = 3 + DetailRoll(Detail, 11, 4);
    for(u32 Pebble = 0; Pebble < Pebbles; Pebble++)
    {
        i32 X, Y;
        ScatterSpot(Detail, Pebble, 3, &X, &Y);
        i32 Width = 1 + (i32)DetailRoll(Detail, Pebble, 2);
        PutPebble(Canvas, X, Y, Width, 1 + (i32)DetailRoll(Detail + 1, Pebble, 2),
                  ART_RGB(204, 186, 156), ART_RGB(164, 144, 116), Soil.C[0]);
    }
    if ((Variant % 3) == 0)
    {
        i32 X, Y;
        ScatterSpot(Detail + 4, 0, 6, &X, &Y);
        PutCrack(Canvas, Detail, X, Y, 7, ART_RGB(96, 68, 44), Soil.C[3]);
    }
    if (Variant == 2 || Variant == 5)
    {
        i32 X, Y;
        ScatterSpot(Detail + 5, 0, 4, &X, &Y);
        color_ramp Sprig = Ramp(ART_RGB(70, 90, 40), ART_RGB(84, 112, 46),
                                ART_RGB(100, 130, 52), ART_RGB(128, 156, 66));
        PutTuft(Canvas, HashLattice(Detail, 5, 5), X, Y, &Sprig);
    }
}

// NOTE(zoubir): wind-laid ash with soot, bone-white flecks and the odd
// ember still glowing; basalt is the same plain cooled to black glass,
// split by cracks
internal void
DrawAshTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    bool32 IsAsh = Kind == TerrainKind_Ash;
    color_ramp Ground = IsAsh ?
        Ramp(ART_RGB(104, 98, 94), ART_RGB(118, 112, 106),
             ART_RGB(132, 126, 118), ART_RGB(150, 144, 134)) :
        Ramp(ART_RGB(40, 36, 44), ART_RGB(50, 46, 54),
             ART_RGB(61, 56, 65), ART_RGB(78, 71, 80));
    FillGround(Canvas, Ground, Seed, Variant, 0.f, 0.5f, IsAsh);
    if (IsAsh)
    {
        // NOTE(zoubir): fine ripples the wind combed into the ash
        PutWindRipples(Canvas, Seed, Variant, 3, 1, Ground.C[3], 0.45f,
                       ART_RGB(84, 78, 76), 0.40f);
        // NOTE(zoubir): stones half buried in it, lit on top with a shadow
        // under
        u32 Stones = 2 + DetailRoll(Detail, 17, 2);
        for(u32 Stone = 0; Stone < Stones; Stone++)
        {
            i32 X, Y;
            ScatterSpot(Detail + 7, Stone, 3, &X, &Y);
            i32 Width = 2 + (i32)DetailRoll(Detail + 8, Stone, 2);
            i32 Height = 1 + (i32)DetailRoll(Detail + 9, Stone, 2);
            PutPebble(Canvas, X, Y, Width, Height, ART_RGB(168, 160, 150), ART_RGB(112, 104, 98),
                      ART_RGB(80, 74, 72));
        }
        for(u32 Soot = 0; Soot < 4; Soot++)
        {
            i32 X, Y;
            ScatterSpot(Detail + 1, Soot, 2, &X, &Y);
            u32 Dark = ART_RGB(88, 82, 80);
            PutPixel(Canvas, X, Y, Dark);
            PutPixel(Canvas, X + 1, Y, Dark);
            if (DetailRoll(Detail, Soot, 2))
            {
                PutPixel(Canvas, X + 1, Y + 1, Dark);
            }
        }
        for(u32 Fleck = 0; Fleck < 5; Fleck++)
        {
            i32 X, Y;
            ScatterSpot(Detail + 2, Fleck, 1, &X, &Y);
            PutPixel(Canvas, X, Y, ART_RGB(170, 164, 154));
        }
        // NOTE(zoubir): a cinder, dark with a lit top
        if (Variant & 1)
        {
            i32 X, Y;
            ScatterSpot(Detail + 3, 0, 4, &X, &Y);
            PutPebble(Canvas, X, Y, 2, 1, ART_RGB(110, 100, 96), ART_RGB(58, 52, 52),
                      Ground.C[0]);
        }
        if (Variant == 5)
        {
            i32 X, Y;
            ScatterSpot(Detail + 4, 0, 5, &X, &Y);
            PutGlow(Canvas, (float)X + 0.5f, (float)Y + 0.5f, 3.5f, ART_RGB(220, 90, 40), 0.5f);
            PutPixel(Canvas, X, Y, ART_RGB(255, 196, 90));
            PutPixel(Canvas, X + 1, Y, ART_RGB(230, 100, 36));
            PutPixel(Canvas, X, Y + 1, ART_RGB(150, 50, 30));
        }
        return;
    }
    u32 Cracks = 1 + DetailRoll(Detail, 13, 2);
    for(u32 Crack = 0; Crack < Cracks; Crack++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 5, Crack, 5, &X, &Y);
        PutCrack(Canvas, HashLattice(Detail, (i32)Crack, 2), X, Y, 9,
                 ART_RGB(24, 20, 28), Ground.C[3]);
    }
    for(u32 Sheen = 0; Sheen < 3; Sheen++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 6, Sheen, 2, &X, &Y);
        PutPixel(Canvas, X, Y, ART_RGB(104, 94, 112));
    }
}

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
    // NOTE(zoubir): one or two big stones, then smaller ones around them
    u32 Stones = 5 + DetailRoll(Detail, 17, 3);
    for(u32 Index = 0; Index < Stones; Index++)
    {
        bool32 Big = Index < 1 + (Variant & 1);
        float RX = Big ? 4.5f + (float)DetailRoll(Detail, Index, 3) :
            1.5f + (float)DetailRoll(Detail, Index, 2);
        float RY = Big ? RX - 1.5f : RX - 0.5f;
        i32 Margin = (i32)RX + 3;
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
    i32 Row = 0;
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            bool32 Lower = SplitY[Variant] && Y >= SplitY[Variant];
            i32 Split = Lower ? SplitBottom[Variant] : SplitTop[Variant];
            i32 X0 = (Split && X >= Split) ? Split : 0;
            i32 X1 = (Split && X < Split) ? Split : 32;
            i32 Y0 = Lower ? SplitY[Variant] : 0;
            i32 Y1 = (SplitY[Variant] && !Lower) ? SplitY[Variant] : 32;
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

// NOTE(zoubir): the liquid and hazard kinds' tiles (liquid_tiles.cpp,
// terrain_hazard_art.cpp)
internal void DrawLiquidTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Cell);
internal void DrawHazardTerrainTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Cell);

// NOTE(zoubir): Cell 0..7 (see the top of terrain_art.cpp)
internal void
DrawTerrainTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Cell)
{
    u32 Seed = 1000u * (u32)Kind;
    switch(Kind)
    {
        case TerrainKind_Grass: DrawGrassTile(Canvas, Seed, Cell); break;
        case TerrainKind_Dirt:
        case TerrainKind_Mud: DrawSoilTile(Canvas, Kind, Seed, Cell); break;
        case TerrainKind_Ash:
        case TerrainKind_Basalt: DrawAshTile(Canvas, Kind, Seed, Cell); break;
        case TerrainKind_Rock:
        case TerrainKind_BasaltWall: DrawCragTile(Canvas, Kind, Seed, Cell); break;
        case TerrainKind_StoneFloor: DrawFlagstoneTile(Canvas, Seed, Cell); break;
        case TerrainKind_StoneWall: DrawStoneWallTile(Canvas, Seed, Cell); break;
        case TerrainKind_Snow: DrawSnowTile(Canvas, Seed, Cell); break;
        case TerrainKind_Ice: DrawIceTile(Canvas, Seed, Cell); break;
        case TerrainKind_ShallowWater:
        case TerrainKind_DeepWater:
        case TerrainKind_Lava: DrawLiquidTile(Canvas, Kind, Cell); break;
        default: DrawHazardTerrainTile(Canvas, Kind, Cell); break;
    }
}
