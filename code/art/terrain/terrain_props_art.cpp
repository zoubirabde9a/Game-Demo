/* Terrain props: boulders, dead trees, logs, fences and crates, one
   48-pixel frame each, drawn by code like the ground. A new prop is one
   more case in DrawTerrainProp. Included by terrain_art.cpp. */

// NOTE(zoubir): props that are not the packed tree: one 48-pixel frame
// each, feet on the 7/8 line like monsters. Logs, fences and crates are
// the jumpables: low and wide, so a unit standing on one reads as on top
internal void
DrawTerrainProp(sprite_canvas *Canvas, terrain_prop Prop)
{
    switch(Prop)
    {
        case TerrainProp_Boulder:
        {
            color_ramp Stone = Ramp(ART_RGB(70, 68, 66), ART_RGB(108, 104, 98),
                                    ART_RGB(146, 140, 130), ART_RGB(188, 182, 170));
            color_ramp Moss = Ramp(ART_RGB(40, 70, 30), ART_RGB(60, 100, 40),
                                   ART_RGB(84, 130, 50), ART_RGB(120, 160, 70));
            FillBlob(Canvas, 24.f, 34.f, 13.f, 9.f, Stone);
            FillBlob(Canvas, 18.f, 37.f, 7.f, 5.f, Stone, -0.1f);
            FillBlob(Canvas, 31.f, 36.f, 6.f, 5.f, Stone, 0.05f);
            FillBlob(Canvas, 21.f, 27.f, 6.f, 2.5f, Moss, 0.1f);
            FillLimb(Canvas, V2(26.f, 30.f), V2(30.f, 36.f), 0.5f, 0.4f,
                     Ramp(Stone.C[0], Stone.C[0], Stone.C[0], Stone.C[0]));
            OutlineFrame(Canvas, ART_RGB(26, 24, 22));
        } break;

        case TerrainProp_DeadTree:
        {
            color_ramp Bark = Ramp(ART_RGB(40, 32, 28), ART_RGB(66, 54, 46),
                                   ART_RGB(94, 80, 68), ART_RGB(126, 110, 94));
            v2 Root = V2(24.f, 42.f);
            v2 Fork = V2(23.f, 22.f);
            FillLimb(Canvas, Root, Fork, 3.f, 2.f, Bark);
            FillLimb(Canvas, Root, Root + V2(-6.f, 1.f), 1.5f, 0.6f, Bark, -0.1f);
            FillLimb(Canvas, Root, Root + V2(6.f, 1.f), 1.5f, 0.6f, Bark, -0.1f);
            FillLimb(Canvas, Fork, V2(14.f, 10.f), 1.8f, 0.6f, Bark);
            FillLimb(Canvas, Fork, V2(32.f, 8.f), 1.8f, 0.6f, Bark);
            FillLimb(Canvas, V2(27.f, 15.f), V2(35.f, 16.f), 1.f, 0.4f, Bark);
            FillLimb(Canvas, V2(18.f, 15.f), V2(12.f, 18.f), 1.f, 0.4f, Bark);
            FillLimb(Canvas, V2(23.f, 22.f), V2(24.f, 6.f), 1.4f, 0.5f, Bark, 0.05f);
            OutlineFrame(Canvas, ART_RGB(16, 12, 10));
        } break;

        case TerrainProp_Log:
        {
            // NOTE(zoubir): a fallen trunk lying across the tile, its cut
            // end toward the viewer's right showing the rings
            color_ramp Bark = Ramp(ART_RGB(48, 34, 24), ART_RGB(78, 56, 38),
                                   ART_RGB(106, 78, 52), ART_RGB(136, 104, 72));
            color_ramp Wood = Ramp(ART_RGB(150, 112, 70), ART_RGB(184, 142, 92),
                                   ART_RGB(210, 172, 118), ART_RGB(232, 200, 150));
            color_ramp Moss = Ramp(ART_RGB(40, 70, 30), ART_RGB(60, 100, 40),
                                   ART_RGB(84, 130, 50), ART_RGB(120, 160, 70));
            FillLimb(Canvas, V2(8.f, 35.f), V2(38.f, 35.f), 6.5f, 6.f, Bark);
            FillLimb(Canvas, V2(19.f, 30.f), V2(15.f, 24.f), 1.6f, 1.f, Bark, 0.05f);
            // NOTE(zoubir): bark ridges along the trunk
            u32 Ridge = Bark.C[0];
            FillLimb(Canvas, V2(10.f, 33.f), V2(26.f, 33.f), 0.4f, 0.4f,
                     Ramp(Ridge, Ridge, Ridge, Ridge));
            FillLimb(Canvas, V2(14.f, 37.f), V2(34.f, 37.f), 0.4f, 0.4f,
                     Ramp(Ridge, Ridge, Ridge, Ridge));
            FillBlob(Canvas, 24.f, 29.5f, 6.f, 1.8f, Moss, 0.1f);
            FillBlob(Canvas, 39.f, 35.f, 4.f, 6.f, Wood, 0.1f);
            FillFlatEllipse(Canvas, 39.f, 35.f, 2.4f, 3.8f, Wood.C[1]);
            FillFlatEllipse(Canvas, 39.f, 35.f, 1.2f, 2.f, Wood.C[3]);
            FillDot(Canvas, 39.f, 35.f, 0.6f, Bark.C[1]);
            OutlineFrame(Canvas, ART_RGB(24, 16, 10));
        } break;

        case TerrainProp_Fence:
        {
            // NOTE(zoubir): two posts with pointed tops and two rails
            color_ramp Plank = Ramp(ART_RGB(70, 50, 32), ART_RGB(104, 76, 48),
                                    ART_RGB(136, 102, 66), ART_RGB(170, 132, 88));
            FillLimb(Canvas, V2(6.f, 26.f), V2(42.f, 26.f), 1.5f, 1.5f, Plank, 0.05f);
            FillLimb(Canvas, V2(6.f, 34.f), V2(42.f, 34.f), 1.5f, 1.5f, Plank, -0.05f);
            for(u32 Post = 0; Post < 2; Post++)
            {
                float X = Post ? 35.f : 13.f;
                FillLimb(Canvas, V2(X, 41.f), V2(X, 22.f), 2.2f, 2.f, Plank);
                FillTriangle(Canvas, V2(X - 2.f, 21.f), V2(X + 2.5f, 21.f),
                             V2(X, 17.f), Plank, 0.8f, 0.5f);
                FillDot(Canvas, X, 26.f, 0.6f, ART_RGB(60, 60, 64));
                FillDot(Canvas, X, 34.f, 0.6f, ART_RGB(60, 60, 64));
            }
            OutlineFrame(Canvas, ART_RGB(26, 18, 10));
        } break;

        case TerrainProp_Crate:
        {
            // NOTE(zoubir): a box seen from the front and above: a lit top,
            // a plank front with a cross brace, iron on the corners
            color_ramp Plank = Ramp(ART_RGB(92, 62, 34), ART_RGB(128, 90, 50),
                                    ART_RGB(162, 120, 70), ART_RGB(196, 154, 98));
            color_ramp Iron = Ramp(ART_RGB(40, 40, 46), ART_RGB(70, 70, 78),
                                   ART_RGB(104, 104, 112), ART_RGB(150, 150, 160));
            i32 Left = 11;
            i32 Right = 37;
            i32 Lid = 15;
            i32 Front = 23;
            i32 Bottom = 42;
            for(i32 Y = Lid; Y < Bottom; Y++)
            {
                for(i32 X = Left; X < Right; X++)
                {
                    bool32 Top = Y < Front;
                    // NOTE(zoubir): top planks run front to back, front
                    // planks across; a dark seam between planks
                    bool32 Seam = Top ? ((X - Left) % 6) == 5 : ((Y - Front) % 5) == 4;
                    float Light = Top ? 0.85f : 0.45f;
                    Light += 0.1f * (float)((HashLattice(91u, X / 6, Y / 5) >> 24) & 1);
                    u32 Color = Seam ? Plank.C[0] : ShadeRamp(&Plank, Light, X, Y);
                    PutPixel(Canvas, X, Y, Color);
                }
            }
            FillLimb(Canvas, V2((float)Left + 2.f, (float)Bottom - 2.f),
                     V2((float)Right - 2.f, (float)Front + 2.f), 1.3f, 1.3f, Plank, 0.15f);
            // NOTE(zoubir): the lid's front edge catches the light
            for(i32 X = Left; X < Right; X++)
            {
                PutPixel(Canvas, X, Front, Plank.C[3]);
            }
            // NOTE(zoubir): an iron bracket bent round each corner of the
            // front and of the lid, 6 pixels along each edge, with a rivet
            i32 CornerX[2] = {Left, Right - 1};
            i32 CornerY[3] = {Lid, Front, Bottom - 1};
            for(u32 CY = 0; CY < 3; CY++)
            {
                for(u32 CX = 0; CX < 2; CX++)
                {
                    i32 StepX = CX ? -1 : 1;
                    i32 StepY = CY == 2 ? -1 : 1;
                    for(i32 Along = 0; Along < 6; Along++)
                    {
                        for(i32 Across = 0; Across < 2; Across++)
                        {
                            i32 AX = CornerX[CX] + StepX * Along;
                            i32 AY = CornerY[CY] + StepY * Across;
                            i32 BX = CornerX[CX] + StepX * Across;
                            i32 BY = CornerY[CY] + StepY * Along;
                            float Light = Across == 0 ? 0.85f : 0.4f;
                            PutPixel(Canvas, AX, AY, ShadeRamp(&Iron, Light, AX, AY));
                            if (CY != 0 || Along < 3)
                            {
                                PutPixel(Canvas, BX, BY, ShadeRamp(&Iron, Light, BX, BY));
                            }
                        }
                    }
                    PutPixel(Canvas, CornerX[CX] + 2 * StepX, CornerY[CY] + 2 * StepY,
                             Iron.C[3]);
                }
            }
            OutlineFrame(Canvas, ART_RGB(24, 16, 10));
        } break;

        default:
        {
        } break;
    }
}
