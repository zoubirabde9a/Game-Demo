/* Hazard terrain art: the full tiles of the ground kinds that do something
   to whoever stands on them (sim/terrain/terrain_kinds.cpp), drawn into
   the terrain atlas by DrawTerrainTile (terrain/ground_tiles.cpp) like
   the rest. Each has to read at a glance from its neighbours:

   pit      near-black depths; the client draws the far wall dropping
            into it and the lip around it (client/ground/pit_walls.cpp)
   spring   bright turquoise water welling up, bubbles rising and
            healing sparkles; animated like water
   bramble  a dense tangle of thorny stems, leaves and berries
   bog      dark muck under a scum of acid green, with bubbles
   rune     a violet glyph cut into a worn slab, glowing. Cells 0-3 are
            the quarters of one big glyph, so a 2 x 2 patch of runes is a
            single circle; cell 4 is the bare slab and 5-7 small glyphs
            for runes on their own (8-15 repeat the slab). The client picks which (and pulses
            the glow) in client/ground/ground_cells.cpp */

internal void
DrawPitTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Dark = Ramp(ART_RGB(5, 4, 9), ART_RGB(9, 7, 15),
                           ART_RGB(14, 11, 22), ART_RGB(22, 18, 34));
    FillGround(Canvas, Dark, Seed, Variant, -0.15f, 0.5f);
    // NOTE(zoubir): faint motes of dust hanging in the dark
    for(u32 Mote = 0; Mote < 2; Mote++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 1, Mote + 2 * Variant, 3, &X, &Y);
        PutPixel(Canvas, X, Y, ART_RGB(44, 38, 64));
    }
}

internal void
DrawSpringTile(sprite_canvas *Canvas, u32 Layout, u32 Frame)
{
    color_ramp Water = Ramp(ART_RGB(36, 150, 152), ART_RGB(56, 182, 174),
                            ART_RGB(86, 210, 194), ART_RGB(150, 240, 220));
    u32 Seed = 5500u;
    float Drift = (float)Frame / 4.f;
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            float Value = 0.6f + 0.4f * (GroundTexture(Seed, Layout, X, Y) - 0.5f);
            // NOTE(zoubir): light dancing on the water, a net of bright
            // lines that shifts each frame
            // NOTE(zoubir): brighter where the water wells up, swelling
            // and settling over the frames
            float Well = GroundNoise(Seed + 5, Layout, X, Y, 8, 8);
            Value += 0.25f * (Well - 0.5f) * (1.f + Sin(2.f * Pi32 * (Drift + Well)));
            PutPixel(Canvas, X, Y, ShadeRamp(&Water, Value, X, Y));
        }
    }
    // NOTE(zoubir): bubbles welling up: a dot, then a ring that widens
    // and fades, each out of step with the others
    for(u32 Bubble = 0; Bubble < 3; Bubble++)
    {
        i32 X, Y;
        ScatterSpot(Seed + 1 + Layout, Bubble, 5, &X, &Y);
        u32 Phase = (Frame + Bubble) % 4;
        if (Phase == 0)
        {
            PutPixel(Canvas, X, Y, ART_RGB(230, 255, 248));
        }
        else if (Phase < 3)
        {
            // NOTE(zoubir): a ring as a diamond, lit along its top
            i32 R = (i32)Phase;
            float Strength = Phase == 1 ? 0.8f : 0.45f;
            for(i32 Step = 0; Step <= R; Step++)
            {
                i32 Across = R - Step;
                BlendPixel(Canvas, X - Across, Y - Step, ART_RGB(230, 255, 248), Strength);
                BlendPixel(Canvas, X + Across, Y - Step, ART_RGB(230, 255, 248), Strength);
                BlendPixel(Canvas, X - Across, Y + Step, ART_RGB(230, 255, 248), Strength * 0.5f);
                BlendPixel(Canvas, X + Across, Y + Step, ART_RGB(230, 255, 248), Strength * 0.5f);
            }
        }
    }
    // NOTE(zoubir): healing sparkles, somewhere new each frame
    for(u32 Sparkle = 0; Sparkle < 2; Sparkle++)
    {
        i32 X, Y;
        ScatterSpot(Seed + 9 + Layout, Frame + 4 * Sparkle, 3, &X, &Y);
        PutGlow(Canvas, (float)X + 0.5f, (float)Y + 0.5f, 3.5f, ART_RGB(255, 255, 255), 0.55f);
        PutPixel(Canvas, X, Y, ART_RGB(255, 255, 255));
        PutPixel(Canvas, X - 1, Y, ART_RGB(210, 255, 240));
        PutPixel(Canvas, X + 1, Y, ART_RGB(210, 255, 240));
        PutPixel(Canvas, X, Y - 1, ART_RGB(210, 255, 240));
        PutPixel(Canvas, X, Y + 1, ART_RGB(210, 255, 240));
    }
}

internal void
DrawBrambleTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Floor = Ramp(ART_RGB(24, 32, 20), ART_RGB(32, 44, 26),
                            ART_RGB(42, 56, 32), ART_RGB(54, 70, 38));
    color_ramp Leaf = Ramp(ART_RGB(34, 60, 30), ART_RGB(48, 82, 38),
                           ART_RGB(66, 104, 46), ART_RGB(96, 132, 60));
    FillGround(Canvas, Floor, Seed, Variant, 0.f, 0.6f);
    // NOTE(zoubir): leaves under the stems
    for(u32 Index = 0; Index < 10; Index++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 1, Index, 2, &X, &Y);
        PutPixel(Canvas, X, Y, Leaf.C[3]);
        PutPixel(Canvas, X + 1, Y, Leaf.C[2]);
        PutPixel(Canvas, X, Y + 1, Leaf.C[1]);
        PutPixel(Canvas, X + 1, Y + 1, Leaf.C[1]);
        PutPixel(Canvas, X + 2, Y + 1, Leaf.C[0]);
    }
    // NOTE(zoubir): woody stems wandering in loops, lit along their top,
    // thorns pricking out on both sides; they turn back before the tile's
    // edge so a neighbour never cuts one off
    u32 Stem = ART_RGB(92, 58, 50);
    u32 StemLight = ART_RGB(132, 92, 70);
    u32 StemDark = ART_RGB(30, 22, 20);
    u32 Thorn = ART_RGB(206, 190, 150);
    for(u32 Index = 0; Index < 6; Index++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 2, Index, 4, &X, &Y);
        u32 H = HashLattice(Detail, (i32)Index, 41);
        i32 DX = (H & 1) ? 1 : -1;
        i32 DY = (H & 2) ? 1 : -1;
        for(i32 Step = 0; Step < 14; Step++)
        {
            if ((HashLattice(H, Step, 7) % 5) == 0)
            {
                DY = -DY;
            }
            if (X + DX < 2 || X + DX > 29)
            {
                DX = -DX;
            }
            if (Y + DY < 2 || Y + DY > 29)
            {
                DY = -DY;
            }
            X += DX;
            if ((Step % 3) == 2)
            {
                Y += DY;
            }
            PutPixel(Canvas, X, Y + 1, StemDark);
            PutPixel(Canvas, X, Y, (Step & 1) ? Stem : StemLight);
            if ((Step % 4) == 1)
            {
                PutPixel(Canvas, X, Y - 1, Thorn);
            }
            else if ((Step % 4) == 3)
            {
                PutPixel(Canvas, X, Y + 2, Thorn);
            }
        }
        // NOTE(zoubir): a leaf or a berry at the end of a stem
        if ((H >> 8) % 3 == 0)
        {
            PutPixel(Canvas, X, Y, ART_RGB(176, 26, 48));
            PutPixel(Canvas, X + 1, Y, ART_RGB(130, 16, 36));
            PutPixel(Canvas, X, Y - 1, ART_RGB(250, 120, 130));
        }
        else
        {
            PutPixel(Canvas, X, Y - 1, Leaf.C[3]);
            PutPixel(Canvas, X + 1, Y - 1, Leaf.C[2]);
            PutPixel(Canvas, X + 1, Y, Leaf.C[1]);
        }
    }
}

internal void
DrawBogTile(sprite_canvas *Canvas, u32 Seed, u32 Variant)
{
    u32 Detail = Seed + 977u * (Variant + 1);
    color_ramp Muck = Ramp(ART_RGB(36, 38, 22), ART_RGB(46, 50, 26),
                           ART_RGB(58, 64, 30), ART_RGB(74, 80, 36));
    color_ramp Scum = Ramp(ART_RGB(84, 118, 30), ART_RGB(112, 150, 36),
                           ART_RGB(146, 184, 50), ART_RGB(190, 220, 90));
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            float Texture = GroundTexture(Seed, Variant, X, Y);
            float Film = GroundNoise(Seed + 3, Variant, X, Y, 8, 8);
            u32 Color;
            if (Film > 0.64f)
            {
                Color = ShadeRamp(&Scum, 0.2f + (Film - 0.64f) * 3.f +
                                  0.4f * (Texture - 0.5f), X, Y);
            }
            else if (Film > 0.6f)
            {
                Color = Muck.C[0];
            }
            else
            {
                Color = ShadeRamp(&Muck, 0.5f + 0.9f * (Texture - 0.5f), X, Y);
            }
            PutPixel(Canvas, X, Y, Color);
        }
    }
    // NOTE(zoubir): bubbles of foul gas, lit on their top left
    u32 Bubbles = 2 + DetailRoll(Detail, Variant, 3);
    for(u32 Bubble = 0; Bubble < Bubbles; Bubble++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 1, Bubble + 4 * Variant, 3, &X, &Y);
        bool32 Big = DetailRoll(Detail + 2, Bubble + 4 * Variant, 3) == 0;
        if (Big)
        {
            PutPixel(Canvas, X, Y - 1, Scum.C[2]);
            PutPixel(Canvas, X - 1, Y, Scum.C[2]);
            PutPixel(Canvas, X + 1, Y, Scum.C[1]);
            PutPixel(Canvas, X, Y + 1, Scum.C[0]);
            PutPixel(Canvas, X, Y, Muck.C[1]);
            PutPixel(Canvas, X - 1, Y - 1, ART_RGB(228, 248, 160));
        }
        else
        {
            PutPixel(Canvas, X, Y, Scum.C[3]);
            PutPixel(Canvas, X + 1, Y + 1, Muck.C[0]);
        }
    }
}

// NOTE(zoubir): whether a point of a glyph is on one of its strokes.
// Big glyphs are 64 pixels across (four tiles' quarters), small ones 32;
// X, Y are from the glyph's middle
internal bool32
RuneStroke(float X, float Y, u32 Design)
{
    float Distance = SquareRoot(X * X + Y * Y);
    if (Design == 0)
    {
        float Outer = 22.f;
        float Inner = 16.5f;
        if (Absolute(Distance - Outer) < 1.f || Absolute(Distance - Inner) < 0.6f)
        {
            return true;
        }
        // NOTE(zoubir): eight ticks between the rings, and a three-armed
        // mark inside with a dot at its heart
        float Angle = ATan2(Y, X);
        float Eighth = Pi32 / 4.f;
        float Off = Angle - Eighth * floorf(Angle / Eighth + 0.5f);
        if (Distance > Inner && Distance < Outer && Absolute(Off) * Distance < 0.8f)
        {
            return true;
        }
        if (Distance < 3.f)
        {
            return Distance < 1.6f;
        }
        for(u32 Arm = 0; Arm < 3; Arm++)
        {
            float ArmAngle = -Pi32 / 2.f + (float)Arm * 2.f * Pi32 / 3.f;
            float AX = Cos(ArmAngle);
            float AY = Sin(ArmAngle);
            float Along = X * AX + Y * AY;
            float Across = Absolute(-X * AY + Y * AX);
            if (Along > 4.f && Along < 12.f && Across < 0.8f)
            {
                return true;
            }
            if (Absolute(Along - 12.f) < 0.8f && Across < 3.2f)
            {
                return true;
            }
        }
        return false;
    }
    if (Absolute(Distance - 11.f) < 0.75f)
    {
        return true;
    }
    float AX = Absolute(X);
    switch(Design)
    {
        case 1:
        {
            // NOTE(zoubir): a chevron over a bar
            return AX <= 5.f && (Absolute(Y - (AX - 3.f)) < 0.7f || (Absolute(Y - 4.f) < 0.7f));
        }
        case 2:
        {
            // NOTE(zoubir): an arrow pointing up
            return (AX < 0.7f && Y > -6.f && Y < 6.f) ||
                (AX <= 4.f && Absolute(Y - (AX - 6.f)) < 0.7f);
        }
        default:
        {
            // NOTE(zoubir): a diamond with a dot inside
            return Absolute(AX + Absolute(Y) - 6.f) < 0.7f || (X * X + Y * Y < 1.5f);
        }
    }
}

// NOTE(zoubir): a worn slab, and the glyph cut into it: dark where the
// cut's edge shades it, glowing violet in the groove, with a haze round it
internal void
DrawRuneTile(sprite_canvas *Canvas, u32 Seed, u32 Cell)
{
    u32 Detail = Seed + 977u * (Cell + 1);
    color_ramp Slab = Ramp(ART_RGB(62, 56, 74), ART_RGB(74, 68, 88),
                           ART_RGB(86, 80, 100), ART_RGB(104, 96, 118));
    FillGround(Canvas, Slab, Seed, Cell == 4 ? 0 : 1, 0.f, 0.6f);
    for(u32 Chip = 0; Chip < 4; Chip++)
    {
        i32 X, Y;
        ScatterSpot(Detail + 1, Chip + 4 * Cell, 2, &X, &Y);
        PutPixel(Canvas, X, Y, Slab.C[3]);
        PutPixel(Canvas, X + 1, Y + 1, Slab.C[0]);
    }
    if (Cell == 4 || Cell > 7)
    {
        return;
    }
    // NOTE(zoubir): where this tile sits in its glyph
    bool32 Big = Cell < 4;
    u32 Design = Big ? 0 : Cell - 4;
    float OriginX = Big ? (float)(32 * (i32)(Cell & 1)) - 32.f : -16.f;
    float OriginY = Big ? (float)(32 * (i32)(Cell >> 1)) - 32.f : -16.f;
    u32 Haze = ART_RGB(120, 70, 220);
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            float GX = OriginX + (float)X + 0.5f;
            float GY = OriginY + (float)Y + 0.5f;
            if (RuneStroke(GX, GY, Design))
            {
                bool32 Core = RuneStroke(GX - 1.f, GY, Design) &&
                    RuneStroke(GX + 1.f, GY, Design);
                bool32 Lit = !RuneStroke(GX - 1.f, GY - 1.f, Design);
                PutPixel(Canvas, X, Y, Core ? ART_RGB(246, 232, 255) :
                         Lit ? ART_RGB(214, 180, 255) : ART_RGB(176, 128, 250));
                continue;
            }
            // NOTE(zoubir): the haze, stronger nearer a stroke; the cut's
            // upper-left lip shades the groove's edge
            float Near = 0.f;
            for(i32 OY = -2; OY <= 2; OY++)
            {
                for(i32 OX = -2; OX <= 2; OX++)
                {
                    if (RuneStroke(GX + (float)OX, GY + (float)OY, Design))
                    {
                        float Reach = 1.f - 0.3f * (float)(Absolute(OX) + Absolute(OY));
                        Near = Maximum(Near, Reach);
                    }
                }
            }
            if (RuneStroke(GX + 1.f, GY + 1.f, Design))
            {
                PutPixel(Canvas, X, Y, ART_RGB(36, 26, 52));
            }
            else if (Near > 0.f)
            {
                BlendPixel(Canvas, X, Y, Haze, 0.55f * Near);
            }
        }
    }
}

internal void
DrawHazardTerrainTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Cell)
{
    u32 Seed = 1000u * (u32)Kind;
    switch(Kind)
    {
        case TerrainKind_Pit: DrawPitTile(Canvas, Seed, Cell); break;
        case TerrainKind_Spring:
        {
            DrawSpringTile(Canvas, Cell / TERRAIN_VARIANTS, Cell % TERRAIN_VARIANTS);
        } break;
        case TerrainKind_Bramble: DrawBrambleTile(Canvas, Seed, Cell); break;
        case TerrainKind_Bog: DrawBogTile(Canvas, Seed, Cell); break;
        case TerrainKind_Rune: DrawRuneTile(Canvas, Seed, Cell); break;
        default:
        {
            InvalidCodePath;
        } break;
    }
}
