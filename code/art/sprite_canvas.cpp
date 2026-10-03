/* Sprite canvas: draws pixel-art sprite frames on the CPU, so monster art
   lives in code next to the monster instead of in binary files. Shapes are
   shaded as if lit from the upper left, with a 4-step color ramp and
   ordered dithering between steps, then outlined. Coordinates are pixels
   inside one frame, Y down. No OpenGL here: the server builds this too. */

// NOTE(zoubir): u32 colors are bytes R, G, B, A in memory
#define ART_RGB(R, G, B) ((u32)(R) | ((u32)(G) << 8) | ((u32)(B) << 16) | 0xFF000000)
#define ART_CLEAR 0

inline float
ArtClamp01(float Value)
{
    float Result = Value < 0.f ? 0.f : (Value > 1.f ? 1.f : Value);
    return Result;
}

inline v2
Lerp2(v2 A, float t, v2 B)
{
    v2 Result = A + t * (B - A);
    return Result;
}

struct color_ramp
{
    // NOTE(zoubir): darkest to lightest
    u32 C[4];
};

struct sprite_canvas
{
    u32 *Pixels;
    // NOTE(zoubir): pixels per row of the whole sheet
    u32 Stride;
    u32 Width;
    u32 Height;
    // NOTE(zoubir): shapes drawn after a SetMirror(true) are flipped around
    // the frame's center column, for bodies whose far side faces away
    bool32 Mirror;
};

inline color_ramp
Ramp(u32 C0, u32 C1, u32 C2, u32 C3)
{
    color_ramp Result = {{C0, C1, C2, C3}};
    return Result;
}

inline sprite_canvas
CanvasFrame(u32 *SheetPixels, u32 SheetWidth, u32 FrameSize,
            u32 Column, u32 Row)
{
    sprite_canvas Result = {};
    Result.Pixels = SheetPixels + Row * FrameSize * SheetWidth +
        Column * FrameSize;
    Result.Stride = SheetWidth;
    Result.Width = FrameSize;
    Result.Height = FrameSize;
    return Result;
}

inline u32
GetPixel(sprite_canvas *Canvas, i32 X, i32 Y)
{
    u32 Result = ART_CLEAR;
    if (X >= 0 && Y >= 0 && X < (i32)Canvas->Width && Y < (i32)Canvas->Height)
    {
        Result = Canvas->Pixels[Y * Canvas->Stride + X];
    }
    return Result;
}

// NOTE(zoubir): a shape's edge pixel drawn over another shape gets the
// darkest ramp step, so overlapping parts (arm over body) stay readable
inline void
PutShapePixel(sprite_canvas *Canvas, i32 X, i32 Y, u32 Color, bool32 IsEdge,
              color_ramp *Ramp);

inline void
PutPixel(sprite_canvas *Canvas, i32 X, i32 Y, u32 Color)
{
    if (Canvas->Mirror)
    {
        X = (i32)Canvas->Width - 1 - X;
    }
    if (X >= 0 && Y >= 0 && X < (i32)Canvas->Width && Y < (i32)Canvas->Height)
    {
        Canvas->Pixels[Y * Canvas->Stride + X] = Color;
    }
}

inline void
PutShapePixel(sprite_canvas *Canvas, i32 X, i32 Y, u32 Color, bool32 IsEdge,
              color_ramp *Ramp)
{
    i32 ReadX = Canvas->Mirror ? (i32)Canvas->Width - 1 - X : X;
    if (IsEdge && GetPixel(Canvas, ReadX, Y) != ART_CLEAR)
    {
        Color = Ramp->C[0];
    }
    PutPixel(Canvas, X, Y, Color);
}

global_variable float BayerMatrix4[16] =
{
     0.f/16.f,  8.f/16.f,  2.f/16.f, 10.f/16.f,
    12.f/16.f,  4.f/16.f, 14.f/16.f,  6.f/16.f,
     3.f/16.f, 11.f/16.f,  1.f/16.f,  9.f/16.f,
    15.f/16.f,  7.f/16.f, 13.f/16.f,  5.f/16.f,
};

// NOTE(zoubir): Light in [0, 1]; picks a ramp step with dithering so the
// boundary between two steps is a checker instead of a hard edge
inline u32
ShadeRamp(color_ramp *Ramp, float Light, i32 X, i32 Y)
{
    float Scaled = ArtClamp01(Light) * 3.f;
    i32 Step = (i32)Scaled;
    // NOTE(zoubir): only the middle of each step is dithered, so large
    // areas stay flat and read as clean pixel art
    float Fraction = (Scaled - (float)Step - 0.3f) / 0.4f;
    if (Fraction > BayerMatrix4[(Y & 3) * 4 + (X & 3)])
    {
        Step++;
    }
    if (Step > 3)
    {
        Step = 3;
    }
    return Ramp->C[Step];
}

// NOTE(zoubir): lit from up-left, a little from the viewer
inline float
LightFromNormal(float NX, float NY, float NZ)
{
    float LX = -0.45f;
    float LY = -0.6f;
    float LZ = 0.66f;
    float Diffuse = NX * LX + NY * LY + NZ * LZ;
    float Result = 0.18f + 0.82f * Maximum(0.f, Diffuse);
    return Result;
}

// NOTE(zoubir): a shaded ball, the main building block for bodies
internal void
FillBlob(sprite_canvas *Canvas, float CX, float CY, float RX, float RY,
         color_ramp Ramp, float LightBias = 0.f)
{
    i32 MinX = (i32)(CX - RX - 1.f);
    i32 MaxX = (i32)(CX + RX + 1.f);
    i32 MinY = (i32)(CY - RY - 1.f);
    i32 MaxY = (i32)(CY + RY + 1.f);
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
                float Light = LightFromNormal(NX, NY, NZ) + LightBias;
                float Rim = 1.f - 1.f / Minimum(RX, RY);
                PutShapePixel(Canvas, X, Y, ShadeRamp(&Ramp, Light, X, Y),
                              D > Rim * Rim, &Ramp);
            }
        }
    }
}

// NOTE(zoubir): a shaded tube from A to B with radius RA at A and RB at B,
// used for limbs, tails, horns and necks
internal void
FillLimb(sprite_canvas *Canvas, v2 A, v2 B, float RA, float RB,
         color_ramp Ramp, float LightBias = 0.f)
{
    float MaxR = Maximum(RA, RB);
    i32 MinX = (i32)(Minimum(A.X, B.X) - MaxR - 1.f);
    i32 MaxX = (i32)(Maximum(A.X, B.X) + MaxR + 1.f);
    i32 MinY = (i32)(Minimum(A.Y, B.Y) - MaxR - 1.f);
    i32 MaxY = (i32)(Maximum(A.Y, B.Y) + MaxR + 1.f);
    v2 AB = B - A;
    float LengthSqAB = Maximum(LengthSq(AB), 0.0001f);
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            v2 P = V2((float)X + 0.5f, (float)Y + 0.5f);
            float t = ArtClamp01(DotProduct(P - A, AB) / LengthSqAB);
            v2 Closest = A + t * AB;
            v2 Offset = P - Closest;
            float R = Lerp(RA, t, RB);
            float DistanceSq = LengthSq(Offset);
            if (DistanceSq <= R * R)
            {
                float NX = Offset.X / R;
                float NY = Offset.Y / R;
                float NZ = SquareRoot(Maximum(0.f, 1.f - NX * NX - NY * NY));
                float Light = LightFromNormal(NX, NY, NZ) + LightBias;
                PutShapePixel(Canvas, X, Y, ShadeRamp(&Ramp, Light, X, Y),
                              DistanceSq > (R - 1.f) * (R - 1.f), &Ramp);
            }
        }
    }
}

inline float
EdgeFunction(v2 A, v2 B, v2 P)
{
    float Result = (B.X - A.X) * (P.Y - A.Y) - (B.Y - A.Y) * (P.X - A.X);
    return Result;
}

// NOTE(zoubir): flat-ish triangle for claws, teeth, ears, wing membranes.
// Shade runs from Light0 at A toward Light1 at the B-C edge
internal void
FillTriangle(sprite_canvas *Canvas, v2 A, v2 B, v2 C, color_ramp Ramp,
             float Light0, float Light1)
{
    i32 MinX = (i32)Minimum(A.X, Minimum(B.X, C.X));
    i32 MaxX = (i32)Maximum(A.X, Maximum(B.X, C.X)) + 1;
    i32 MinY = (i32)Minimum(A.Y, Minimum(B.Y, C.Y));
    i32 MaxY = (i32)Maximum(A.Y, Maximum(B.Y, C.Y)) + 1;
    float Area = EdgeFunction(A, B, C);
    if (Absolute(Area) < 0.0001f)
    {
        return;
    }
    for(i32 Y = MinY; Y <= MaxY; Y++)
    {
        for(i32 X = MinX; X <= MaxX; X++)
        {
            v2 P = V2((float)X + 0.5f, (float)Y + 0.5f);
            float W0 = EdgeFunction(B, C, P) / Area;
            float W1 = EdgeFunction(C, A, P) / Area;
            float W2 = EdgeFunction(A, B, P) / Area;
            if (W0 >= 0.f && W1 >= 0.f && W2 >= 0.f)
            {
                float Light = Lerp(Light1, W0, Light0);
                PutPixel(Canvas, X, Y, ShadeRamp(&Ramp, Light, X, Y));
            }
        }
    }
}

// NOTE(zoubir): solid disc with no shading, for eyes, glows and spots
internal void
FillDot(sprite_canvas *Canvas, float CX, float CY, float R, u32 Color)
{
    for(i32 Y = (i32)(CY - R - 1.f); Y <= (i32)(CY + R + 1.f); Y++)
    {
        for(i32 X = (i32)(CX - R - 1.f); X <= (i32)(CX + R + 1.f); X++)
        {
            float DX = (float)X + 0.5f - CX;
            float DY = (float)Y + 0.5f - CY;
            if (DX * DX + DY * DY <= R * R)
            {
                PutPixel(Canvas, X, Y, Color);
            }
        }
    }
}

// NOTE(zoubir): every empty pixel next to a drawn one becomes Color. Run
// it once per frame, after all shapes, so the sprite reads on any ground
internal void
OutlineFrame(sprite_canvas *Canvas, u32 Color)
{
    u32 Marker = 0x00FF00FF;
    for(i32 Y = 0; Y < (i32)Canvas->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Canvas->Width; X++)
        {
            u32 *Pixel = &Canvas->Pixels[Y * Canvas->Stride + X];
            if (*Pixel == ART_CLEAR)
            {
                u32 Up = GetPixel(Canvas, X, Y - 1);
                u32 Down = GetPixel(Canvas, X, Y + 1);
                u32 Left = GetPixel(Canvas, X - 1, Y);
                u32 Right = GetPixel(Canvas, X + 1, Y);
                bool32 Touches =
                    (Up != ART_CLEAR && Up != Marker) ||
                    (Down != ART_CLEAR && Down != Marker) ||
                    (Left != ART_CLEAR && Left != Marker) ||
                    (Right != ART_CLEAR && Right != Marker);
                if (Touches)
                {
                    *Pixel = Marker;
                }
            }
        }
    }
    for(i32 Y = 0; Y < (i32)Canvas->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Canvas->Width; X++)
        {
            u32 *Pixel = &Canvas->Pixels[Y * Canvas->Stride + X];
            if (*Pixel == Marker)
            {
                *Pixel = Color;
            }
        }
    }
}

// NOTE(zoubir): what a monster's draw function is asked for. Anim is the
// row (idle, move, windup, attack, recover); t runs 0..1 across the row
struct monster_pose
{
    animation_type Anim;
    u32 Frame;
    u32 FrameCount;
    float t;
    // NOTE(zoubir): Sin(2 Pi t) and Cos(2 Pi t), for loops
    float Wave;
    float Wave2;
};

// NOTE(zoubir): erases an ordered-dither share of the drawn pixels, for
// things fading in or out. Amount 0 keeps everything, 1 clears the frame
internal void
DissolveFrame(sprite_canvas *Canvas, float Amount)
{
    for(i32 Y = 0; Y < (i32)Canvas->Height; Y++)
    {
        for(i32 X = 0; X < (i32)Canvas->Width; X++)
        {
            if (BayerMatrix4[(Y & 3) * 4 + (X & 3)] < Amount)
            {
                Canvas->Pixels[Y * Canvas->Stride + X] = ART_CLEAR;
            }
        }
    }
}
