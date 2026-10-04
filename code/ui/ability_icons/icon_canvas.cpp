/* Icon canvas: paints smooth, anti-aliased icons on the CPU for the
   ability bar. Shapes are filled by distance (each pixel's coverage is how
   far its centre is inside the shape), so edges stay clean at any icon
   size. Coordinates run 0..1 across the icon, Y down. Colours are floats
   0..1 with straight alpha; IconFinish turns the canvas into RGBA bytes
   with a soft drop shadow under the shapes.

   A shape takes an icon_paint: one colour, or a gradient between two
   colours along a line. */

struct icon_canvas
{
    v4 *Pixels;
    u32 Size;
};

struct icon_paint
{
    v4 A;
    v4 B;
    v2 From;
    v2 To;
};

// NOTE(zoubir): a colour from 0..255 channels
inline v4
IconColor(u32 R, u32 G, u32 B, u32 A = 255)
{
    v4 Result = V4(R / 255.f, G / 255.f, B / 255.f, A / 255.f);
    return Result;
}

inline icon_paint
Solid(v4 Color)
{
    icon_paint Result = {Color, Color, V2(0.f, 0.f), V2(0.f, 1.f)};
    return Result;
}

inline icon_paint
Gradient(v4 A, v4 B, v2 From, v2 To)
{
    icon_paint Result = {A, B, From, To};
    return Result;
}

inline float
IconClamp01(float Value)
{
    float Result = Value < 0.f ? 0.f : (Value > 1.f ? 1.f : Value);
    return Result;
}

inline v4
PaintAt(icon_paint *Paint, v2 P)
{
    v2 Line = Paint->To - Paint->From;
    float LengthSq = DotProduct(Line, Line);
    float t = LengthSq > 0.f ? IconClamp01(DotProduct(P - Paint->From, Line) / LengthSq) : 0.f;
    v4 Result = V4(Lerp(Paint->A.X, t, Paint->B.X), Lerp(Paint->A.Y, t, Paint->B.Y),
                   Lerp(Paint->A.Z, t, Paint->B.Z), Lerp(Paint->A.W, t, Paint->B.W));
    return Result;
}

// NOTE(zoubir): Color with Coverage over the pixel, straight alpha
inline void
BlendIconPixel(v4 *Pixel, v4 Color, float Coverage)
{
    float SourceA = Color.W * Coverage;
    if (SourceA <= 0.f)
    {
        return;
    }
    float OutA = SourceA + Pixel->W * (1.f - SourceA);
    float Keep = Pixel->W * (1.f - SourceA);
    Pixel->X = (Color.X * SourceA + Pixel->X * Keep) / OutA;
    Pixel->Y = (Color.Y * SourceA + Pixel->Y * Keep) / OutA;
    Pixel->Z = (Color.Z * SourceA + Pixel->Z * Keep) / OutA;
    Pixel->W = OutA;
}

// NOTE(zoubir): the pixel box (inclusive Min, exclusive Max) that covers
// the icon-space box, one pixel of margin for the soft edge
inline void
IconPixelBox(icon_canvas *Canvas, v2 Min, v2 Max, i32 *X0, i32 *Y0, i32 *X1, i32 *Y1)
{
    float Size = (float)Canvas->Size;
    *X0 = (i32)(Min.X * Size) - 1;
    *Y0 = (i32)(Min.Y * Size) - 1;
    *X1 = (i32)(Max.X * Size) + 2;
    *Y1 = (i32)(Max.Y * Size) + 2;
    if (*X0 < 0) *X0 = 0;
    if (*Y0 < 0) *Y0 = 0;
    if (*X1 > (i32)Canvas->Size) *X1 = (i32)Canvas->Size;
    if (*Y1 > (i32)Canvas->Size) *Y1 = (i32)Canvas->Size;
}

inline v2
IconPixelCentre(icon_canvas *Canvas, i32 X, i32 Y)
{
    float Size = (float)Canvas->Size;
    v2 Result = V2(((float)X + 0.5f) / Size, ((float)Y + 0.5f) / Size);
    return Result;
}

// NOTE(zoubir): coverage from a signed distance in icon units (negative
// inside), a one-pixel soft edge
inline float
IconCoverage(icon_canvas *Canvas, float Distance)
{
    float Result = IconClamp01(0.5f - Distance * (float)Canvas->Size);
    return Result;
}

internal void
IconCircle(icon_canvas *Canvas, v2 Centre, float Radius, icon_paint Paint)
{
    i32 X0, Y0, X1, Y1;
    IconPixelBox(Canvas, Centre - V2(Radius, Radius), Centre + V2(Radius, Radius),
                 &X0, &Y0, &X1, &Y1);
    for(i32 Y = Y0; Y < Y1; Y++)
    {
        for(i32 X = X0; X < X1; X++)
        {
            v2 P = IconPixelCentre(Canvas, X, Y);
            float Distance = Length(P - Centre) - Radius;
            BlendIconPixel(&Canvas->Pixels[Y * Canvas->Size + X], PaintAt(&Paint, P),
                           IconCoverage(Canvas, Distance));
        }
    }
}

// NOTE(zoubir): a ring of Width centred on Radius; with Start < End
// (radians, 0 = +X, growing clockwise since Y is down) only that arc
internal void
IconArc(icon_canvas *Canvas, v2 Centre, float Radius, float Width,
        icon_paint Paint, float Start = 0.f, float End = 0.f)
{
    float Outer = Radius + 0.5f * Width;
    i32 X0, Y0, X1, Y1;
    IconPixelBox(Canvas, Centre - V2(Outer, Outer), Centre + V2(Outer, Outer),
                 &X0, &Y0, &X1, &Y1);
    bool32 Partial = Start < End;
    v2 StartPoint = Centre + Radius * V2(Cos(Start), Sin(Start));
    v2 EndPoint = Centre + Radius * V2(Cos(End), Sin(End));
    for(i32 Y = Y0; Y < Y1; Y++)
    {
        for(i32 X = X0; X < X1; X++)
        {
            v2 P = IconPixelCentre(Canvas, X, Y);
            v2 D = P - Centre;
            float Distance = Absolute(Length(D) - Radius) - 0.5f * Width;
            if (Partial)
            {
                float Angle = ATan2(D.Y, D.X);
                while (Angle < Start) Angle += 2.f * Pi32;
                while (Angle > Start + 2.f * Pi32) Angle -= 2.f * Pi32;
                if (Angle > End)
                {
                    // NOTE(zoubir): outside the arc: round caps at its ends
                    float ToStart = Length(P - StartPoint);
                    float ToEnd = Length(P - EndPoint);
                    Distance = Minimum(ToStart, ToEnd) - 0.5f * Width;
                }
            }
            BlendIconPixel(&Canvas->Pixels[Y * Canvas->Size + X], PaintAt(&Paint, P),
                           IconCoverage(Canvas, Distance));
        }
    }
}

// NOTE(zoubir): a thick line from A to B with round ends
internal void
IconCapsule(icon_canvas *Canvas, v2 A, v2 B, float Radius, icon_paint Paint)
{
    v2 Min = V2(Minimum(A.X, B.X) - Radius, Minimum(A.Y, B.Y) - Radius);
    v2 Max = V2(Maximum(A.X, B.X) + Radius, Maximum(A.Y, B.Y) + Radius);
    i32 X0, Y0, X1, Y1;
    IconPixelBox(Canvas, Min, Max, &X0, &Y0, &X1, &Y1);
    v2 AB = B - A;
    float LengthSq = DotProduct(AB, AB);
    for(i32 Y = Y0; Y < Y1; Y++)
    {
        for(i32 X = X0; X < X1; X++)
        {
            v2 P = IconPixelCentre(Canvas, X, Y);
            float t = LengthSq > 0.f ? IconClamp01(DotProduct(P - A, AB) / LengthSq) : 0.f;
            float Distance = Length(P - (A + t * AB)) - Radius;
            BlendIconPixel(&Canvas->Pixels[Y * Canvas->Size + X], PaintAt(&Paint, P),
                           IconCoverage(Canvas, Distance));
        }
    }
}

// NOTE(zoubir): a convex polygon, points in either winding order
internal void
IconPolygon(icon_canvas *Canvas, v2 *Points, u32 Count, icon_paint Paint)
{
    v2 Min = Points[0];
    v2 Max = Points[0];
    float Area = 0.f;
    for(u32 Index = 0; Index < Count; Index++)
    {
        v2 P = Points[Index];
        v2 Q = Points[(Index + 1) % Count];
        Min = V2(Minimum(Min.X, P.X), Minimum(Min.Y, P.Y));
        Max = V2(Maximum(Max.X, P.X), Maximum(Max.Y, P.Y));
        Area += P.X * Q.Y - Q.X * P.Y;
    }
    float Winding = Area >= 0.f ? 1.f : -1.f;
    i32 X0, Y0, X1, Y1;
    IconPixelBox(Canvas, Min, Max, &X0, &Y0, &X1, &Y1);
    for(i32 Y = Y0; Y < Y1; Y++)
    {
        for(i32 X = X0; X < X1; X++)
        {
            v2 P = IconPixelCentre(Canvas, X, Y);
            // NOTE(zoubir): inside a convex shape, the distance is the
            // largest signed distance to an edge line
            float Distance = -1000.f;
            for(u32 Index = 0; Index < Count; Index++)
            {
                v2 A = Points[Index];
                v2 B = Points[(Index + 1) % Count];
                v2 Edge = B - A;
                float EdgeLength = Length(Edge);
                if (EdgeLength <= 0.f)
                {
                    continue;
                }
                v2 Normal = V2(Edge.Y, -Edge.X) * (Winding / EdgeLength);
                Distance = Maximum(Distance, DotProduct(P - A, Normal));
            }
            BlendIconPixel(&Canvas->Pixels[Y * Canvas->Size + X], PaintAt(&Paint, P),
                           IconCoverage(Canvas, Distance));
        }
    }
}

inline void
IconTriangle(icon_canvas *Canvas, v2 A, v2 B, v2 C, icon_paint Paint)
{
    v2 Points[3] = {A, B, C};
    IconPolygon(Canvas, Points, 3, Paint);
}

// NOTE(zoubir): a soft light that fades from Color at the centre to
// nothing at Radius, painted under what comes after it
internal void
IconGlow(icon_canvas *Canvas, v2 Centre, float Radius, v4 Color)
{
    i32 X0, Y0, X1, Y1;
    IconPixelBox(Canvas, Centre - V2(Radius, Radius), Centre + V2(Radius, Radius),
                 &X0, &Y0, &X1, &Y1);
    for(i32 Y = Y0; Y < Y1; Y++)
    {
        for(i32 X = X0; X < X1; X++)
        {
            v2 P = IconPixelCentre(Canvas, X, Y);
            float Fade = 1.f - IconClamp01(Length(P - Centre) / Radius);
            BlendIconPixel(&Canvas->Pixels[Y * Canvas->Size + X], Color, Fade * Fade);
        }
    }
}

// NOTE(zoubir): a four-pointed sparkle: two thin diamonds crossed
internal void
IconSparkle(icon_canvas *Canvas, v2 Centre, float Radius, icon_paint Paint)
{
    float Thin = 0.22f * Radius;
    v2 Tall[4] = {Centre + V2(0.f, -Radius), Centre + V2(Thin, 0.f),
                  Centre + V2(0.f, Radius), Centre + V2(-Thin, 0.f)};
    v2 Wide[4] = {Centre + V2(-Radius, 0.f), Centre + V2(0.f, -Thin),
                  Centre + V2(Radius, 0.f), Centre + V2(0.f, Thin)};
    IconPolygon(Canvas, Tall, 4, Paint);
    IconPolygon(Canvas, Wide, 4, Paint);
}

// NOTE(zoubir): the canvas as RGBA bytes at Out (rows of Stride pixels),
// over a soft shadow cast down and to the right so icons read on any slot
internal void
IconFinish(icon_canvas *Canvas, u32 *Out, u32 Stride)
{
    i32 Size = (i32)Canvas->Size;
    i32 ShiftX = Size / 32;
    i32 ShiftY = Size / 20;
    for(i32 Y = 0; Y < Size; Y++)
    {
        for(i32 X = 0; X < Size; X++)
        {
            // NOTE(zoubir): the shadow is the shapes' alpha, shifted and
            // averaged over a 3x3 box to soften it
            float Shadow = 0.f;
            for(i32 DY = -1; DY <= 1; DY++)
            {
                for(i32 DX = -1; DX <= 1; DX++)
                {
                    i32 SX = X - ShiftX + DX;
                    i32 SY = Y - ShiftY + DY;
                    if (SX >= 0 && SY >= 0 && SX < Size && SY < Size)
                    {
                        Shadow += Canvas->Pixels[SY * Size + SX].W;
                    }
                }
            }
            v4 Pixel = V4(0.f, 0.f, 0.f, 0.55f * Shadow / 9.f);
            BlendIconPixel(&Pixel, Canvas->Pixels[Y * Size + X], 1.f);
            u32 R = (u32)(IconClamp01(Pixel.X) * 255.f + 0.5f);
            u32 G = (u32)(IconClamp01(Pixel.Y) * 255.f + 0.5f);
            u32 B = (u32)(IconClamp01(Pixel.Z) * 255.f + 0.5f);
            u32 A = (u32)(IconClamp01(Pixel.W) * 255.f + 0.5f);
            Out[Y * Stride + X] = R | (G << 8) | (B << 16) | (A << 24);
        }
    }
}
