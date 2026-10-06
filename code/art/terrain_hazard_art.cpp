/* Hazard terrain art: the full tiles of the ground kinds that do something
   to whoever stands on them (sim/terrain/terrain_kinds.cpp), drawn into
   the terrain atlas by DrawTerrainTile (terrain_art.cpp) like the rest.
   Each has to read at a glance from its neighbours: a pit is a black
   hole, a spring glows turquoise, bramble is a dark tangle, a bog is
   sickly green, a rune is a violet glyph that pulses. Springs and runes
   animate through the row's four columns. */

// NOTE(zoubir): a soft glowing dot, added over what is there
internal void
PutTileGlow(sprite_canvas *Canvas, i32 CX, i32 CY, i32 Radius, u32 Color)
{
    for(i32 Y = -Radius; Y <= Radius; Y++)
    {
        for(i32 X = -Radius; X <= Radius; X++)
        {
            if (X * X + Y * Y <= Radius * Radius)
            {
                PutTilePixel(Canvas, CX + X, CY + Y, Color);
            }
        }
    }
}

internal void
DrawHazardTerrainTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Column,
                      u32 Seed, float Phase)
{
    switch(Kind)
    {
        case TerrainKind_Pit:
        {
            // NOTE(zoubir): darkness with a faint cold swirl, so a pit
            // never reads as shadow or water
            color_ramp Dark = Ramp(ART_RGB(4, 3, 8), ART_RGB(10, 8, 16),
                                   ART_RGB(18, 14, 28), ART_RGB(34, 26, 52));
            FillTileGround(Canvas, Dark, Seed, -0.2f, 0.8f);
            for(u32 Mote = 0; Mote < 3; Mote++)
            {
                u32 H = HashLattice(Seed, (i32)Mote, 37);
                PutTilePixel(Canvas, (i32)(H % 32), (i32)((H >> 8) % 32),
                             ART_RGB(60, 50, 90));
            }
        } break;

        case TerrainKind_Spring:
        {
            color_ramp Water = Ramp(ART_RGB(20, 110, 120), ART_RGB(40, 160, 160),
                                    ART_RGB(80, 210, 196), ART_RGB(190, 255, 236));
            FillTileGround(Canvas, Water, 1500u, 0.05f, 0.9f);
            // NOTE(zoubir): bubbles rising and sparkles, moving with the frames
            i32 Rise = (i32)(Phase * 32.f * 4.f);
            for(u32 Bubble = 0; Bubble < 5; Bubble++)
            {
                u32 H = HashLattice(71u, (i32)Bubble, 3);
                i32 X = (i32)(H % 32);
                i32 Y = (i32)((H >> 8) % 32) - Rise;
                PutTilePixel(Canvas, X, Y, Water.C[3]);
                PutTilePixel(Canvas, X + 1, Y, Water.C[2]);
                PutTilePixel(Canvas, X, Y + 1, Water.C[2]);
            }
            u32 H = HashLattice(73u, (i32)Column, 5);
            PutTilePixel(Canvas, (i32)(H % 32), (i32)((H >> 8) % 32),
                         ART_RGB(255, 255, 255));
        } break;

        case TerrainKind_Bramble:
        {
            color_ramp Floor = Ramp(ART_RGB(30, 46, 22), ART_RGB(44, 64, 30),
                                    ART_RGB(60, 84, 38), ART_RGB(84, 110, 50));
            FillTileGround(Canvas, Floor, Seed, -0.1f, 1.2f);
            // NOTE(zoubir): woody stems with thorns along them, and berries
            for(u32 Stem = 0; Stem < 6; Stem++)
            {
                u32 H = HashLattice(Seed, (i32)Stem, 41);
                i32 X = (i32)(H % 32);
                i32 Y = (i32)((H >> 8) % 32);
                i32 DX = ((H >> 16) & 1) ? 1 : -1;
                for(i32 Step = 0; Step < 9; Step++)
                {
                    i32 PX = X + Step * DX;
                    i32 PY = Y + Step / 2;
                    PutTilePixel(Canvas, PX, PY, ART_RGB(78, 54, 34));
                    if ((Step % 3) == 1)
                    {
                        PutTilePixel(Canvas, PX, PY - 1, ART_RGB(170, 150, 110));
                    }
                }
                if ((H >> 20) % 3 == 0)
                {
                    PutTilePixel(Canvas, X + 2 * DX, Y + 2, ART_RGB(170, 30, 50));
                }
            }
        } break;

        case TerrainKind_Bog:
        {
            color_ramp Muck = Ramp(ART_RGB(40, 46, 22), ART_RGB(62, 72, 32),
                                   ART_RGB(88, 102, 42), ART_RGB(128, 146, 60));
            FillTileGround(Canvas, Muck, Seed, 0.f, 1.3f);
            // NOTE(zoubir): sickly bubbles and a scum of yellow-green
            for(u32 Bubble = 0; Bubble < 6; Bubble++)
            {
                u32 H = HashLattice(Seed, (i32)Bubble, 43);
                i32 X = (i32)(H % 32);
                i32 Y = (i32)((H >> 8) % 32);
                PutTilePixel(Canvas, X, Y, ART_RGB(170, 200, 80));
                PutTilePixel(Canvas, X + 1, Y, ART_RGB(120, 150, 50));
                PutTilePixel(Canvas, X, Y + 1, Muck.C[0]);
            }
        } break;

        case TerrainKind_Rune:
        {
            color_ramp Slab = Ramp(ART_RGB(60, 54, 70), ART_RGB(84, 76, 96),
                                   ART_RGB(106, 98, 120), ART_RGB(136, 128, 150));
            FillTileGround(Canvas, Slab, 1900u, 0.f, 0.8f);
            // NOTE(zoubir): a ring and a chevron glyph; the glow pulses
            float Pulse = 0.5f + 0.5f * Sin(2.f * Pi32 * Phase * 4.f);
            u32 Glow = ART_RGB(150 + (u32)(80.f * Pulse), 110 + (u32)(60.f * Pulse), 255);
            for(i32 Y = 0; Y < 32; Y++)
            {
                for(i32 X = 0; X < 32; X++)
                {
                    i32 DX = X - 16;
                    i32 DY = Y - 16;
                    i32 DistanceSq = DX * DX + DY * DY;
                    bool32 Ring = DistanceSq >= 100 && DistanceSq <= 132;
                    bool32 Chevron = Absolute(DX) <= 5 &&
                        (DY == Absolute(DX) - 3 || DY == Absolute(DX) + 1);
                    if (Ring || Chevron)
                    {
                        PutPixel(Canvas, X, Y, Glow);
                    }
                }
            }
            PutTileGlow(Canvas, 16, 16, 1, ART_RGB(250, 240, 255));
        } break;

        default:
        {
            InvalidCodePath;
        } break;
    }
}
