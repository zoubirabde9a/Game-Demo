/* Ground paint: the brushes the ground tiles are painted with. Tile noise
   that every variant of a kind shares along its border (so any variant
   joins any other without a seam), and small details (pebbles, stones,
   cracks, tufts, glows) placed away from the tile's edges, where a
   neighbour of another variant would cut them in half. Included by
   terrain_art.cpp before the tiles that use it. */

// NOTE(zoubir): value noise on cells of CellX x CellY pixels that repeats
// every PeriodX pixels across and PeriodY down (each a multiple of its
// cell). 0..255
inline u32
WrappedNoise(u32 Seed, i32 X, i32 Y, i32 CellX, i32 CellY, i32 PeriodX, i32 PeriodY)
{
    i32 CellsX = PeriodX / CellX;
    i32 CellsY = PeriodY / CellY;
    i32 CX = FloorDiv(X, CellX);
    i32 CY = FloorDiv(Y, CellY);
    i32 FX = (FloorMod(X, CellX) << 8) / CellX;
    i32 FY = (FloorMod(Y, CellY) << 8) / CellY;
    i32 A = (i32)(HashLattice(Seed, FloorMod(CX, CellsX), FloorMod(CY, CellsY)) >> 24);
    i32 B = (i32)(HashLattice(Seed, FloorMod(CX + 1, CellsX), FloorMod(CY, CellsY)) >> 24);
    i32 C = (i32)(HashLattice(Seed, FloorMod(CX, CellsX), FloorMod(CY + 1, CellsY)) >> 24);
    i32 D = (i32)(HashLattice(Seed, FloorMod(CX + 1, CellsX), FloorMod(CY + 1, CellsY)) >> 24);
    i32 Top = A + (((B - A) * FX) >> 8);
    i32 Bottom = C + (((D - C) * FX) >> 8);
    u32 Result = (u32)(Top + (((Bottom - Top) * FY) >> 8));
    return Result;
}

// NOTE(zoubir): noise that repeats every TERRAIN_TILE_PIXELS, so a tile's
// edges match the next tile's. 0..255
inline u32
WrappedTileNoise(u32 Seed, i32 X, i32 Y, i32 Cell)
{
    u32 Result = WrappedNoise(Seed, X, Y, Cell, Cell, TERRAIN_TILE_PIXELS,
                              TERRAIN_TILE_PIXELS);
    return Result;
}

// NOTE(zoubir): A mixed toward B by T (0..1), alpha kept from A
inline u32
MixArt(u32 A, u32 B, float T)
{
    T = ArtClamp01(T);
    u32 Result = A & 0xFF000000;
    for(u32 Shift = 0; Shift < 24; Shift += 8)
    {
        float From = (float)((A >> Shift) & 0xFF);
        float To = (float)((B >> Shift) & 0xFF);
        Result |= ((u32)(From + T * (To - From) + 0.5f) & 0xFF) << Shift;
    }
    return Result;
}

// NOTE(zoubir): mixes Color into the pixel already there
inline void
BlendPixel(sprite_canvas *Canvas, i32 X, i32 Y, u32 Color, float T)
{
    u32 Under = GetPixel(Canvas, X, Y);
    if (Under != ART_CLEAR)
    {
        PutPixel(Canvas, X, Y, MixArt(Under, Color, T));
    }
}

// NOTE(zoubir): one lattice value of a kind's tile noise. Points on the
// tile's left or top border are the same for every variant, and pulled
// toward the middle so the band they share does not repeat as a grid of
// blotches; the inside ones differ per variant
inline float
GroundLattice(u32 Seed, u32 Variant, i32 CX, i32 CY, i32 CellsX, i32 CellsY)
{
    CX = FloorMod(CX, CellsX);
    CY = FloorMod(CY, CellsY);
    bool32 Border = CX == 0 || CY == 0;
    u32 LatticeSeed = Border ? Seed : Seed + 7919u * (Variant + 1);
    float Result = (float)(HashLattice(LatticeSeed, CX, CY) >> 24) / 255.f;
    if (Border)
    {
        Result = 0.5f + 0.4f * (Result - 0.5f);
    }
    return Result;
}

inline float
SmoothFraction(i32 Offset, i32 Cell)
{
    float T = (float)Offset / (float)Cell;
    float Result = T * T * (3.f - 2.f * T);
    return Result;
}

// NOTE(zoubir): smooth value noise on CellX x CellY cells, wrapping
// every tile, seamless between variants. 0..1
inline float
GroundNoise(u32 Seed, u32 Variant, i32 X, i32 Y, i32 CellX, i32 CellY)
{
    i32 Tile = TERRAIN_TILE_PIXELS;
    i32 CellsX = Tile / CellX;
    i32 CellsY = Tile / CellY;
    i32 CX = FloorDiv(X, CellX);
    i32 CY = FloorDiv(Y, CellY);
    float FX = SmoothFraction(FloorMod(X, CellX), CellX);
    float FY = SmoothFraction(FloorMod(Y, CellY), CellY);
    float A = GroundLattice(Seed, Variant, CX, CY, CellsX, CellsY);
    float B = GroundLattice(Seed, Variant, CX + 1, CY, CellsX, CellsY);
    float C = GroundLattice(Seed, Variant, CX, CY + 1, CellsX, CellsY);
    float D = GroundLattice(Seed, Variant, CX + 1, CY + 1, CellsX, CellsY);
    float Top = A + (B - A) * FX;
    float Bottom = C + (D - C) * FX;
    float Result = Top + (Bottom - Top) * FY;
    return Result;
}

// NOTE(zoubir): three octaves, large shapes first. Drift stretches the
// cells sideways, for wind-laid ash and snow and slow water. 0..1
inline float
GroundTexture(u32 Seed, u32 Variant, i32 X, i32 Y, bool32 Drift = false)
{
    i32 Tall = Drift ? 2 : 1;
    float Result = 0.36f * GroundNoise(Seed, Variant, X, Y, 16, 16 / Tall) +
        0.4f * GroundNoise(Seed + 1, Variant, X, Y, 8, 8 / Tall) +
        0.24f * GroundNoise(Seed + 2, Variant, X, Y, 4, 4 / Tall);
    return Result;
}

// NOTE(zoubir): fills the tile from a ramp by GroundTexture, dithered
// between steps. Contrast under 1 keeps most of it on the middle steps
internal void
FillGround(sprite_canvas *Canvas, color_ramp Ramp, u32 Seed, u32 Variant,
           float Bias, float Contrast, bool32 Drift = false)
{
    for(i32 Y = 0; Y < TERRAIN_TILE_PIXELS; Y++)
    {
        for(i32 X = 0; X < TERRAIN_TILE_PIXELS; X++)
        {
            float Texture = GroundTexture(Seed, Variant, X, Y, Drift);
            float Value = 0.5f + Contrast * 1.6f * (Texture - 0.5f) + Bias;
            PutPixel(Canvas, X, Y, ShadeRamp(&Ramp, Value, X, Y));
        }
    }
}

// NOTE(zoubir): ripples the wind combed into soft ground (ash, snow):
// crests mixed toward Crest, troughs toward Trough, faded in and out by a
// second noise so they do not stripe a whole plain. WavesDown and
// WavesAcross are whole numbers of waves a tile, and the waves bend by tile
// noise whose border every variant shares, so they run on into any
// neighbour without a seam
internal void
PutWindRipples(sprite_canvas *Canvas, u32 Seed, u32 Variant, i32 WavesDown,
               i32 WavesAcross, u32 Crest, float CrestStrength, u32 Trough,
               float TroughStrength)
{
    for(i32 Y = 0; Y < TERRAIN_TILE_PIXELS; Y++)
    {
        for(i32 X = 0; X < TERRAIN_TILE_PIXELS; X++)
        {
            float Bend = 1.6f * GroundNoise(Seed + 11, Variant, X, Y, 8, 8);
            float Phase = 6.2832f * ((float)(WavesDown * Y + WavesAcross * X) /
                                     (float)TERRAIN_TILE_PIXELS + Bend);
            float Wave = Sin(Phase);
            float Fade = GroundNoise(Seed + 12, Variant, X, Y, 16, 16);
            if (Wave > 0.55f)
            {
                BlendPixel(Canvas, X, Y, Crest, CrestStrength * Fade);
            }
            else if (Wave < -0.75f)
            {
                BlendPixel(Canvas, X, Y, Trough, TroughStrength * Fade);
            }
        }
    }
}

// NOTE(zoubir): the Index-th detail spot of a tile, at least Margin
// pixels in from every edge
inline void
ScatterSpot(u32 Seed, u32 Index, i32 Margin, i32 *X, i32 *Y)
{
    u32 H = HashLattice(Seed, (i32)Index, 977);
    i32 Span = TERRAIN_TILE_PIXELS - 2 * Margin;
    *X = Margin + (i32)(H % (u32)Span);
    *Y = Margin + (i32)((H >> 12) % (u32)Span);
}

// NOTE(zoubir): a roll 0..Range-1 for detail Index
inline u32
DetailRoll(u32 Seed, u32 Index, u32 Range)
{
    u32 Result = (HashLattice(Seed + 5u, (i32)Index, 433) >> 8) % Range;
    return Result;
}

// NOTE(zoubir): a pebble Width x Height, lit on its top-left pixel, with
// a shadow pixel cast down-right
internal void
PutPebble(sprite_canvas *Canvas, i32 X, i32 Y, i32 Width, i32 Height,
          u32 Light, u32 Body, u32 Shadow)
{
    for(i32 DY = 0; DY < Height; DY++)
    {
        for(i32 DX = 0; DX < Width; DX++)
        {
            PutPixel(Canvas, X + DX, Y + DY, (DX == 0 && DY == 0) ? Light : Body);
        }
    }
    for(i32 DX = 1; DX <= Width; DX++)
    {
        PutPixel(Canvas, X + DX, Y + Height, Shadow);
    }
}

// NOTE(zoubir): a stone seen from above: a shaded ellipse lit from the top
// left, outlined dark along its lower half, over a soft shadow cast down
// and right
internal void
PutStone(sprite_canvas *Canvas, float CX, float CY, float RX, float RY,
         color_ramp Ramp, u32 Shadow)
{
    i32 MinX = (i32)(CX - RX - 2.f);
    i32 MaxX = (i32)(CX + RX + 2.f);
    i32 MinY = (i32)(CY - RY - 2.f);
    i32 MaxY = (i32)(CY + RY + 2.f);
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            float SX = ((float)X + 0.5f - CX - 1.f) / (RX + 0.5f);
            float SY = ((float)Y + 0.5f - CY - 1.f) / (RY + 0.5f);
            if (SX * SX + SY * SY <= 1.f)
            {
                BlendPixel(Canvas, X, Y, Shadow, 0.55f);
            }
        }
    }
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            float NX = ((float)X + 0.5f - CX) / RX;
            float NY = ((float)Y + 0.5f - CY) / RY;
            float D = NX * NX + NY * NY;
            if (D <= 1.f)
            {
                float NZ = SquareRoot(1.f - D);
                float Light = LightFromNormal(NX, NY, NZ);
                bool32 Edge = D > 0.62f && (NY > 0.15f || NX > 0.55f);
                PutPixel(Canvas, X, Y, Edge ? Ramp.C[0] : ShadeRamp(&Ramp, Light, X, Y));
            }
        }
    }
}

// NOTE(zoubir): a crack wandering Length pixels from X, Y: dark, with a
// lit pixel on its lower side where the far lip catches the light
internal void
PutCrack(sprite_canvas *Canvas, u32 Seed, i32 X, i32 Y, i32 Length,
         u32 Dark, u32 Light)
{
    i32 DX = (Seed & 1) ? 1 : -1;
    for(i32 Step = 0; Step < Length; Step++)
    {
        u32 Turn = HashLattice(Seed, Step, 61) % 5;
        if (Turn == 0)
        {
            Y--;
        }
        else if (Turn == 1)
        {
            Y++;
        }
        else
        {
            X += DX;
        }
        PutPixel(Canvas, X, Y, Dark);
        if ((Step & 1) == 0)
        {
            PutPixel(Canvas, X, Y + 1, Light);
        }
    }
}

// NOTE(zoubir): a tuft of grass blades rooted along row Y: each blade's
// tip lit, its foot in shade
internal void
PutTuft(sprite_canvas *Canvas, u32 Seed, i32 X, i32 Y, color_ramp *Ramp)
{
    u32 Blades = 2 + (Seed % 3);
    for(u32 Blade = 0; Blade < Blades; Blade++)
    {
        i32 BX = X + 2 * (i32)Blade - (i32)Blades + 1;
        i32 Height = 2 + (i32)((Seed >> (4 + 2 * Blade)) % 3);
        i32 Lean = ((Seed >> (12 + Blade)) & 1) ? 1 : 0;
        for(i32 Step = 0; Step < Height; Step++)
        {
            i32 PX = BX + (Step == Height - 1 ? Lean : 0);
            PutPixel(Canvas, PX, Y - Step, Step == Height - 1 ? Ramp->C[3] : Ramp->C[2]);
        }
        PutPixel(Canvas, BX + 1, Y + 1, Ramp->C[0]);
    }
}

// NOTE(zoubir): a soft round glow of Color, strongest in the middle,
// mixed into the ground
internal void
PutGlow(sprite_canvas *Canvas, float CX, float CY, float Radius, u32 Color,
        float Strength)
{
    i32 MinX = (i32)(CX - Radius - 1.f);
    i32 MaxX = (i32)(CX + Radius + 1.f);
    i32 MinY = (i32)(CY - Radius - 1.f);
    i32 MaxY = (i32)(CY + Radius + 1.f);
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            float DX = (float)X + 0.5f - CX;
            float DY = (float)Y + 0.5f - CY;
            float Falloff = 1.f - SquareRoot(DX * DX + DY * DY) / Radius;
            if (Falloff > 0.f)
            {
                BlendPixel(Canvas, X, Y, Color, Strength * Falloff);
            }
        }
    }
}
