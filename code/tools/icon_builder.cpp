/* Draws the game's icon (a gold-hilted sword crossed over a fireball on a
   dark blue badge) and writes it as data/icon/game.ico, with one image
   drawn at each size Windows asks for: 16, 24, 32, 48, 64 and 256 px. The
   small sizes are drawn on their own, with thicker strokes and fewer
   details, not shrunk from the big one. code/platform/game.rc puts the
   .ico into win32_app.exe and launcher.exe.

   build.bat compiles it; run it by hand after changing the drawing, from
   build\, and commit the new .ico:
       icon_builder.exe ..\data\icon\game.ico [preview folder]
   With a preview folder it also writes icon_<size>.png there.

   Every shape is a signed distance (negative inside) in unit space, 0..1
   across the icon. Each pixel takes 8x8 samples, paints the layers back to
   front at each sample and averages them, which gives the edges their
   anti-aliasing. */

#include "../app_defs.h"
#include <math.h>
#pragma warning(disable: 4996) // fopen: a tool writing the one file it was handed

#pragma warning(push, 0)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image/stb_image_write.h"
#pragma warning(pop)

#define ICON_SAMPLES 8

struct icon_v2
{
    float X, Y;
};

struct icon_color
{
    float R, G, B, A;
};

// NOTE(zoubir): stroke widths and which details to draw, chosen per size
// so a 16 px icon still reads as a sword and a fireball
struct icon_style
{
    float Outline;
    float Border;
    float Margin;
    bool32 Details;
};

internal icon_v2 V2(float X, float Y) { icon_v2 Result = {X, Y}; return Result; }
internal icon_v2 Add(icon_v2 A, icon_v2 B) { return V2(A.X + B.X, A.Y + B.Y); }
internal icon_v2 Sub(icon_v2 A, icon_v2 B) { return V2(A.X - B.X, A.Y - B.Y); }
internal icon_v2 Scale(icon_v2 A, float S) { return V2(A.X * S, A.Y * S); }
internal float Dot(icon_v2 A, icon_v2 B) { return A.X * B.X + A.Y * B.Y; }
internal float Length(icon_v2 A) { return sqrtf(Dot(A, A)); }
internal float Clamp01(float V) { return V < 0.f ? 0.f : (V > 1.f ? 1.f : V); }
internal float MaxF(float A, float B) { return A > B ? A : B; }
internal float MinF(float A, float B) { return A < B ? A : B; }

internal icon_color
Rgb(float R, float G, float B)
{
    icon_color Result = {R / 255.f, G / 255.f, B / 255.f, 1.f};
    return Result;
}

internal icon_color
Mix(icon_color A, icon_color B, float T)
{
    icon_color Result = {A.R + (B.R - A.R) * T, A.G + (B.G - A.G) * T,
                         A.B + (B.B - A.B) * T, A.A + (B.A - A.A) * T};
    return Result;
}

// NOTE(zoubir): paints Color over Dest where Distance is inside, with
// Opacity (glows pass less than 1)
internal void
Paint(icon_color *Dest, float Distance, icon_color Color, float Opacity = 1.f)
{
    if (Distance <= 0.f)
    {
        float A = Color.A * Opacity;
        Dest->R += (Color.R - Dest->R) * A;
        Dest->G += (Color.G - Dest->G) * A;
        Dest->B += (Color.B - Dest->B) * A;
        Dest->A += (1.f - Dest->A) * A;
    }
}

internal float
CircleDistance(icon_v2 P, icon_v2 Center, float Radius)
{
    return Length(Sub(P, Center)) - Radius;
}

// NOTE(zoubir): a segment with rounded ends whose radius goes from RadiusA
// at A to RadiusB at B; close enough to exact for a flame's taper
internal float
TaperDistance(icon_v2 P, icon_v2 A, icon_v2 B, float RadiusA, float RadiusB)
{
    icon_v2 AB = Sub(B, A);
    float T = Clamp01(Dot(Sub(P, A), AB) / Dot(AB, AB));
    icon_v2 Closest = Add(A, Scale(AB, T));
    return Length(Sub(P, Closest)) - (RadiusA + (RadiusB - RadiusA) * T);
}

internal float
RoundedBoxDistance(icon_v2 P, float Min, float Max, float Radius)
{
    float Center = 0.5f * (Min + Max);
    float Half = 0.5f * (Max - Min) - Radius;
    float QX = fabsf(P.X - Center) - Half;
    float QY = fabsf(P.Y - Center) - Half;
    float Outside = Length(V2(MaxF(QX, 0.f), MaxF(QY, 0.f)));
    return Outside + MinF(MaxF(QX, QY), 0.f) - Radius;
}

// NOTE(zoubir): Points in either winding; the largest distance to any
// edge's line, which is exact inside and mitred outside
internal float
ConvexDistance(icon_v2 P, icon_v2 *Points, int Count)
{
    float Area = 0.f;
    for (int Index = 0; Index < Count; ++Index)
    {
        icon_v2 A = Points[Index];
        icon_v2 B = Points[(Index + 1) % Count];
        Area += A.X * B.Y - B.X * A.Y;
    }
    float Winding = Area > 0.f ? 1.f : -1.f;

    float Result = -1e9f;
    for (int Index = 0; Index < Count; ++Index)
    {
        icon_v2 A = Points[Index];
        icon_v2 B = Points[(Index + 1) % Count];
        icon_v2 Edge = Sub(B, A);
        icon_v2 Outward = Scale(V2(Edge.Y, -Edge.X), Winding / Length(Edge));
        Result = MaxF(Result, Dot(Sub(P, A), Outward));
    }
    return Result;
}

// The sword runs from the pommel (bottom left) to the tip (top right)
global_variable icon_v2 SwordDir = {0.70710678f, -0.70710678f};
global_variable icon_v2 SwordSide = {0.70710678f, 0.70710678f};
global_variable icon_v2 Pommel = {0.17f, 0.83f};
global_variable icon_v2 Tip = {0.85f, 0.15f};

// The fireball flies up and left; its tail streams under the sword
global_variable icon_v2 FireHead = {0.35f, 0.35f};

// NOTE(zoubir): the flame at one of its three layers: Shrink 0 is the red
// outside, larger values the hotter insides
internal float
FlameDistance(icon_v2 P, float Shrink)
{
    float HeadRadius = 0.20f * (1.f - 0.45f * Shrink);
    float Reach = 1.f - 0.55f * Shrink;
    icon_v2 Tails[3] = {V2(0.82f, 0.82f), V2(0.58f, 0.88f), V2(0.88f, 0.58f)};
    float TailRadius[3] = {0.035f, 0.02f, 0.02f};
    float Result = CircleDistance(P, FireHead, HeadRadius);
    for (int Index = 0; Index < 3; ++Index)
    {
        icon_v2 End = Add(FireHead, Scale(Sub(Tails[Index], FireHead), Reach));
        float Start = Index == 0 ? HeadRadius : HeadRadius * 0.6f;
        float Distance = TaperDistance(P, FireHead, End, Start,
                                       TailRadius[Index] * (1.f - 0.5f * Shrink));
        Result = MinF(Result, Distance);
    }
    return Result;
}

internal icon_color
SampleIcon(icon_v2 P, icon_style *Style)
{
    icon_color Result = {0, 0, 0, 0};

    // The badge: deep blue, lighter at the top, ringed in gold
    float Badge = RoundedBoxDistance(P, Style->Margin, 1.f - Style->Margin, 0.2f);
    icon_color Top = Rgb(44, 60, 100);
    icon_color Bottom = Rgb(14, 19, 34);
    Paint(&Result, Badge, Mix(Top, Bottom, Clamp01((P.Y - Style->Margin) * 1.1f)));
    float Glow = Clamp01(1.f - Length(Sub(P, FireHead)) / 0.48f);
    Paint(&Result, Badge, Rgb(255, 120, 30), (Style->Details ? 0.55f : 0.3f) * Glow * Glow);
    float GoldShade = Clamp01(P.Y);
    icon_color Gold = Mix(Rgb(250, 214, 80), Rgb(170, 120, 26), GoldShade);
    Paint(&Result, MaxF(Badge, -(Badge + Style->Border)), Gold);

    // The fireball, red outside to white-hot in the middle
    Paint(&Result, FlameDistance(P, 0.f), Rgb(212, 52, 26));
    Paint(&Result, FlameDistance(P, 0.45f), Rgb(248, 134, 32));
    Paint(&Result, FlameDistance(P, 0.85f), Rgb(255, 218, 84));
    if (Style->Details)
    {
        Paint(&Result, CircleDistance(P, V2(0.31f, 0.31f), 0.055f), Rgb(255, 250, 220));
    }

    // The sword's shapes; each is painted once outlined, then filled
    icon_v2 GuardCenter = Add(Pommel, Scale(SwordDir, 0.17f));
    icon_v2 BladeBase = Add(GuardCenter, Scale(SwordDir, 0.02f));
    icon_v2 Shoulder = Sub(Tip, Scale(SwordDir, 0.14f));
    float BladeHalf = Style->Details ? 0.06f : 0.07f;
    icon_v2 Blade[5] =
    {
        Sub(BladeBase, Scale(SwordSide, BladeHalf)),
        Sub(Shoulder, Scale(SwordSide, BladeHalf * 0.85f)),
        Tip,
        Add(Shoulder, Scale(SwordSide, BladeHalf * 0.85f)),
        Add(BladeBase, Scale(SwordSide, BladeHalf)),
    };
    float BladeDistance = ConvexDistance(P, Blade, 5);
    float GuardHalf = Style->Details ? 0.12f : 0.14f;
    float GuardDistance = TaperDistance(P, Sub(GuardCenter, Scale(SwordSide, GuardHalf)),
                                        Add(GuardCenter, Scale(SwordSide, GuardHalf)),
                                        0.04f, 0.04f);
    float GripDistance = TaperDistance(P, Pommel, GuardCenter, 0.035f, 0.035f);
    float PommelDistance = CircleDistance(P, Pommel, 0.055f);

    float Sword = MinF(MinF(BladeDistance, GuardDistance), MinF(GripDistance, PommelDistance));
    Paint(&Result, Sword - Style->Outline, Rgb(10, 12, 20));

    // Steel: the upper-left half catches the light, a ridge runs down the middle
    float Across = Dot(Sub(P, BladeBase), SwordSide);
    float Along = Dot(Sub(P, BladeBase), SwordDir);
    icon_color Steel = Across < 0.f ? Rgb(236, 242, 250) : Rgb(150, 162, 184);
    if (Style->Details)
    {
        Steel = Mix(Steel, Rgb(255, 255, 255), 0.25f * Clamp01(1.f - Along * 1.6f) * (Across < 0.f));
    }
    Paint(&Result, BladeDistance, Steel);
    if (Style->Details)
    {
        float Ridge = MaxF(fabsf(Across) - 0.006f, BladeDistance + 0.02f);
        Paint(&Result, Ridge, Rgb(196, 206, 222));
    }

    Paint(&Result, GripDistance, Rgb(98, 58, 34));
    if (Style->Details)
    {
        // Leather wrapping: dark bands across the grip
        float Band = fabsf(fmodf(Dot(Sub(P, Pommel), SwordDir) + 1.f, 0.035f) - 0.0175f) - 0.006f;
        Paint(&Result, MaxF(GripDistance, Band), Rgb(60, 34, 20));
    }
    icon_color HiltGold = Across < 0.f ? Rgb(255, 222, 92) : Rgb(204, 150, 34);
    Paint(&Result, GuardDistance, HiltGold);
    Paint(&Result, PommelDistance, Mix(Rgb(255, 222, 92), Rgb(196, 140, 30),
                                       Clamp01(0.5f + 6.f * Dot(Sub(P, Pommel), V2(0.6f, 0.8f)))));
    if (Style->Details)
    {
        Paint(&Result, CircleDistance(P, Pommel, 0.022f), Rgb(208, 52, 40));
        Paint(&Result, CircleDistance(P, GuardCenter, 0.022f), Rgb(208, 52, 40));
    }
    return Result;
}

// NOTE(zoubir): Out is Size*Size pixels, RGBA, top row first
internal void
DrawIcon(int Size, u8 *Out)
{
    icon_style Style = {};
    float Pixel = 1.f / (float)Size;
    Style.Outline = MaxF(Pixel, 0.022f);
    Style.Border = MaxF(Pixel, 0.035f);
    Style.Margin = Size >= 48 ? 0.03f : 0.f;
    Style.Details = Size >= 48;

    for (int Y = 0; Y < Size; ++Y)
    {
        for (int X = 0; X < Size; ++X)
        {
            float R = 0, G = 0, B = 0, A = 0;
            for (int SY = 0; SY < ICON_SAMPLES; ++SY)
            {
                for (int SX = 0; SX < ICON_SAMPLES; ++SX)
                {
                    icon_v2 P = V2(((float)X + ((float)SX + 0.5f) / ICON_SAMPLES) * Pixel,
                                   ((float)Y + ((float)SY + 0.5f) / ICON_SAMPLES) * Pixel);
                    icon_color C = SampleIcon(P, &Style);
                    R += C.R * C.A;
                    G += C.G * C.A;
                    B += C.B * C.A;
                    A += C.A;
                }
            }
            u8 *Dest = Out + 4 * (Y * Size + X);
            float Inverse = A > 0.f ? 1.f / A : 0.f;
            Dest[0] = (u8)(Clamp01(R * Inverse) * 255.f + 0.5f);
            Dest[1] = (u8)(Clamp01(G * Inverse) * 255.f + 0.5f);
            Dest[2] = (u8)(Clamp01(B * Inverse) * 255.f + 0.5f);
            Dest[3] = (u8)(Clamp01(A / (ICON_SAMPLES * ICON_SAMPLES)) * 255.f + 0.5f);
        }
    }
}

struct icon_buffer
{
    u8 *Data;
    u32 Used;
    u32 Size;
};

internal void
Put(icon_buffer *Buffer, const void *Data, u32 Count)
{
    Assert(Buffer->Used + Count <= Buffer->Size);
    memcpy(Buffer->Data + Buffer->Used, Data, Count);
    Buffer->Used += Count;
}

internal void Put8(icon_buffer *Buffer, u8 V) { Put(Buffer, &V, 1); }
internal void Put16(icon_buffer *Buffer, u16 V) { u8 B[2] = {(u8)V, (u8)(V >> 8)}; Put(Buffer, B, 2); }
internal void
Put32(icon_buffer *Buffer, u32 V)
{
    u8 B[4] = {(u8)V, (u8)(V >> 8), (u8)(V >> 16), (u8)(V >> 24)};
    Put(Buffer, B, 4);
}

internal void
WritePngToBuffer(void *Context, void *Data, int Count)
{
    Put((icon_buffer *)Context, Data, (u32)Count);
}

// NOTE(zoubir): the classic icon image: a BITMAPINFOHEADER with double
// height, the colours bottom row first as BGRA, then an all-clear AND mask
internal void
PutBitmapImage(icon_buffer *Buffer, int Size, u8 *Pixels)
{
    u32 MaskRow = ((Size + 31) / 32) * 4;
    Put32(Buffer, 40);
    Put32(Buffer, Size);
    Put32(Buffer, Size * 2);
    Put16(Buffer, 1);
    Put16(Buffer, 32);
    Put32(Buffer, 0);
    Put32(Buffer, Size * Size * 4 + MaskRow * Size);
    Put32(Buffer, 0);
    Put32(Buffer, 0);
    Put32(Buffer, 0);
    Put32(Buffer, 0);
    for (int Y = Size - 1; Y >= 0; --Y)
    {
        for (int X = 0; X < Size; ++X)
        {
            u8 *P = Pixels + 4 * (Y * Size + X);
            u8 Bgra[4] = {P[2], P[1], P[0], P[3]};
            Put(Buffer, Bgra, 4);
        }
    }
    for (u32 Index = 0; Index < MaskRow * Size; ++Index)
    {
        Put8(Buffer, 0);
    }
}

int
main(int ArgCount, char **Args)
{
    if (ArgCount < 2)
    {
        fprintf(stderr, "usage: icon_builder <out.ico> [preview folder]\n");
        return 1;
    }
    int Sizes[] = {16, 24, 32, 48, 64, 256};
    const int Count = (int)ArrayCount(Sizes);

    icon_buffer Images[Count] = {};
    for (int Index = 0; Index < Count; ++Index)
    {
        int Size = Sizes[Index];
        u8 *Pixels = (u8 *)calloc(Size * Size, 4);
        DrawIcon(Size, Pixels);
        Images[Index].Size = Size * Size * 8 + 1024;
        Images[Index].Data = (u8 *)calloc(Images[Index].Size, 1);
        if (Size == 256)
        {
            stbi_write_png_to_func(WritePngToBuffer, &Images[Index], Size, Size, 4, Pixels, Size * 4);
        }
        else
        {
            PutBitmapImage(&Images[Index], Size, Pixels);
        }
        if (ArgCount > 2)
        {
            char Path[512];
            snprintf(Path, sizeof(Path), "%s/icon_%d.png", Args[2], Size);
            stbi_write_png(Path, Size, Size, 4, Pixels, Size * 4);
        }
        free(Pixels);
    }

    // ICONDIR, then one 16-byte entry per image, then the images
    icon_buffer File = {};
    File.Size = 6 + 16 * Count;
    for (int Index = 0; Index < Count; ++Index)
    {
        File.Size += Images[Index].Used;
    }
    File.Data = (u8 *)calloc(File.Size, 1);
    Put16(&File, 0);
    Put16(&File, 1);
    Put16(&File, (u16)Count);
    u32 Offset = 6 + 16 * Count;
    for (int Index = 0; Index < Count; ++Index)
    {
        Put8(&File, (u8)(Sizes[Index] & 255));
        Put8(&File, (u8)(Sizes[Index] & 255));
        Put8(&File, 0);
        Put8(&File, 0);
        Put16(&File, 1);
        Put16(&File, 32);
        Put32(&File, Images[Index].Used);
        Put32(&File, Offset);
        Offset += Images[Index].Used;
    }
    for (int Index = 0; Index < Count; ++Index)
    {
        Put(&File, Images[Index].Data, Images[Index].Used);
    }

    FILE *Out = fopen(Args[1], "wb");
    if (!Out || fwrite(File.Data, 1, File.Used, Out) != File.Used)
    {
        fprintf(stderr, "icon_builder: could not write %s\n", Args[1]);
        return 1;
    }
    fclose(Out);
    printf("icon_builder: wrote %s (%u bytes)\n", Args[1], File.Used);
    return 0;
}
