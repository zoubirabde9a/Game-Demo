/* Liquid tiles: shallow water, deep water and lava. Each has four layouts
   (cells 0-3, 4-7, 8-11 and 12-15) of four animation frames; the client picks a
   layout per tile and plays the frames in step everywhere, so a frame
   joins its neighbours without a seam. Nothing moves across a tile's
   edge: glints and ripples flicker in place, and lava's veins pulse. A
   slow shimmer that rolls across the whole surface is added by the
   client's world tint (client/ground/). */

// NOTE(zoubir): a ripple: a short arc of light, lit fully on one frame,
// faintly on the frames either side and gone on the fourth
internal void
PutRipple(sprite_canvas *Canvas, i32 X, i32 Y, i32 Length, u32 Frame,
          u32 Index, u32 Light)
{
    u32 Phase = (Frame + Index) % 4;
    if (Phase == 2)
    {
        return;
    }
    float Strength = Phase == 0 ? 0.9f : 0.4f;
    for(i32 Step = 0; Step < Length; Step++)
    {
        i32 Lift = (Step > 0 && Step < Length - 1) ? 1 : 0;
        BlendPixel(Canvas, X + Step, Y - Lift, Light, Strength);
    }
}

internal void
DrawWaterTile(sprite_canvas *Canvas, bool32 Deep, u32 Layout, u32 Frame)
{
    color_ramp Water = Deep ?
        Ramp(ART_RGB(26, 52, 100), ART_RGB(32, 64, 118),
             ART_RGB(40, 78, 136), ART_RGB(62, 106, 162)) :
        Ramp(ART_RGB(66, 130, 168), ART_RGB(78, 146, 184),
             ART_RGB(94, 164, 200), ART_RGB(124, 188, 216));
    u32 Seed = Deep ? 4400u : 4300u;
    // NOTE(zoubir): the surface swells a little from frame to frame
    float Swell = 0.04f * Sin(2.f * Pi32 * (float)Frame / 4.f);
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            float Texture = GroundTexture(Seed, Layout, X, Y, true);
            float Wave = GroundNoise(Seed + 9, Layout, X, Y, 8, 4);
            float Value = 0.45f + 0.6f * (Texture - 0.5f) + Swell * (Wave - 0.5f) * 4.f;
            PutPixel(Canvas, X, Y, ShadeRamp(&Water, Value, X, Y));
        }
    }
    u32 Light = Deep ? ART_RGB(120, 170, 220) : ART_RGB(214, 240, 250);
    u32 Ripples = Deep ? 3 : 5;
    for(u32 Ripple = 0; Ripple < Ripples; Ripple++)
    {
        i32 X, Y;
        ScatterSpot(Seed + 1 + Layout, Ripple, 4, &X, &Y);
        PutRipple(Canvas, X, Y, 3 + (i32)DetailRoll(Seed + Layout, Ripple, 4),
                  Frame, Ripple, Light);
    }
    // NOTE(zoubir): a glint of sun that winks on one frame
    i32 X, Y;
    ScatterSpot(Seed + 7 + Layout, Frame, 3, &X, &Y);
    if (!Deep || Frame == 1)
    {
        PutPixel(Canvas, X, Y, ART_RGB(250, 254, 255));
    }
}

// NOTE(zoubir): a glowing molten surface, white-hot along the currents
// that run through it, with islands of black crust drifting on it, red
// where they meet the melt
internal void
DrawLavaTile(sprite_canvas *Canvas, u32 Layout, u32 Frame)
{
    color_ramp Molten = Ramp(ART_RGB(200, 52, 16), ART_RGB(236, 98, 22),
                             ART_RGB(252, 152, 36), ART_RGB(255, 226, 120));
    color_ramp Crust = Ramp(ART_RGB(36, 18, 16), ART_RGB(54, 26, 20),
                            ART_RGB(76, 34, 24), ART_RGB(120, 44, 24));
    u32 Seed = 4500u;
    for(i32 Y = 0; Y < 32; Y++)
    {
        for(i32 X = 0; X < 32; X++)
        {
            float Texture = GroundTexture(Seed, Layout, X, Y, true);
            float Current = Absolute(0.35f * GroundNoise(Seed + 7, Layout, X, Y, 16, 8) +
                                     0.65f * GroundNoise(Seed + 8, Layout, X, Y, 8, 4) - 0.5f);
            // NOTE(zoubir): the heat pulses along the currents, out of
            // step from place to place
            float Beat = GroundNoise(Seed + 11, Layout, X, Y, 16, 16);
            float Pulse = 0.1f * Sin(2.f * Pi32 * ((float)Frame / 4.f + Beat * 2.f));
            float Island = Texture - 0.64f;
            u32 Color;
            if (Island > 0.03f)
            {
                float Light = 0.2f + 3.f * (Island - 0.03f);
                Color = ShadeRamp(&Crust, Light, X, Y);
            }
            else if (Island > 0.f)
            {
                Color = Crust.C[3];
            }
            else
            {
                float Heat = 0.42f + 0.6f * (0.5f - Texture) + Pulse;
                if (Current < 0.05f)
                {
                    Heat += 0.45f;
                }
                Color = ShadeRamp(&Molten, Heat, X, Y);
            }
            PutPixel(Canvas, X, Y, Color);
        }
    }
    // NOTE(zoubir): bubbles of melt swelling and bursting, out of step
    for(u32 Bubble = 0; Bubble < 2; Bubble++)
    {
        i32 X, Y;
        ScatterSpot(Seed + 1 + Layout, Bubble, 3, &X, &Y);
        u32 Phase = (Frame + 2 * Bubble) % 4;
        if (Phase == 1)
        {
            PutPixel(Canvas, X, Y, Molten.C[3]);
            PutPixel(Canvas, X + 1, Y, Molten.C[2]);
        }
        else if (Phase == 2)
        {
            PutPixel(Canvas, X - 1, Y, Molten.C[3]);
            PutPixel(Canvas, X + 1, Y, Molten.C[3]);
            PutPixel(Canvas, X, Y - 1, Molten.C[3]);
            PutPixel(Canvas, X, Y, Molten.C[0]);
        }
    }
}

internal void
DrawLiquidTile(sprite_canvas *Canvas, terrain_kind Kind, u32 Cell)
{
    u32 Layout = Cell / TERRAIN_VARIANTS;
    u32 Frame = Cell % TERRAIN_VARIANTS;
    if (Kind == TerrainKind_Lava)
    {
        DrawLavaTile(Canvas, Layout, Frame);
    }
    else
    {
        DrawWaterTile(Canvas, Kind == TerrainKind_DeepWater, Layout, Frame);
    }
}
