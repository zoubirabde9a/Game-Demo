#if !defined(MATH_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define MATH_H
#include "intrinsics.h"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#define Absolute(A) ((A > 0) ? (A) : (-A))

struct v2
{
    union
    {
        float Data[2];
        struct
        {
            float X, Y;
        };
    };    
};

inline v2
V2(float X, float Y)
{
    v2 Result = {X, Y};
    return Result;
}

inline v2
V2(float Value)
{
    v2 Result = {Value, Value};
    return Result;
}


inline v2
V2(u32 X, u32 Y)
{
    v2 Result = {(float)X, (float)Y};
    return Result;
}

struct v3
{
    union
    {
        float Data[3];
        struct
        {
            float X, Y, Z;
        };
        struct
        {
            v2 XY;
            float Ignored0_;
        };
        struct
        {
            float Ignored1_;
            v2 YZ;
        };
    };    
};

inline v3
V3(float X, float Y, float Z)
{
    v3 Result = {X, Y, Z};
    return Result;
}

inline v3
V3(float Value)
{
    v3 Result = {Value, Value, Value};
    return Result;
}

inline v3
V3(u32 X, u32 Y, u32 Z)
{
    v3 Result = {(float)X, (float)Y, (float)Z};
    return Result;
}

struct v4
{
    union
    {
        float Data[4];
        struct
        {
            float X, Y, Z, W;
        };
    };
};

inline v4
V4(float X, float Y, float Z, float W)
{
    v4 Result = {X, Y, Z, W};
    return Result;
}

inline v2
operator*(float A, v2 B)
{
    v2 Result;

    Result.X = A * B.X;
    Result.Y = A * B.Y;
    
    return Result;
}

inline v2
operator*(v2 A, float B)
{
    v2 Result;

    Result.X = B * A.X;
    Result.Y = B * A.Y;
    
    return Result;
}

#if 0
inline v2
operator/(float A, v2 B)
{
    v2 Result;

    Result.X = A / B.X;
    Result.Y = A / B.Y;
    
    return Result;
}

inline v2
operator/(v2 A, float B)
{
    v2 Result;

    Result.X = B / A.X;
    Result.Y = B / A.Y;
    
    return Result;
}
#endif

inline v2
operator*(v2 A, v2 B)
{
    v2 Result;

    Result.X = B.X * A.X;
    Result.Y = B.Y * A.Y;
    
    return Result;
}

inline v2 &
operator*=(v2& A, float B)
{
    A = B * A;

    return A;
}

inline v2
operator-(v2 A)
{
    v2 Result;
    
    Result.X = -A.X;
    Result.Y = -A.Y;
    
    return Result;
}

inline v2
operator+(v2 A, v2 B)
{
    v2 Result;
    
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    
    return Result;
}

inline v2&
operator+=(v2& A, v2 B)
{
    A = A + B;
    
    return A;
}

inline v2
operator-(v2 A, v2 B)
{
    v2 Result;
    
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    
    return Result;
}

inline v2
operator-(v2 A, float B)
{
    v2 Result;
    
    Result.X = A.X - B;
    Result.Y = A.Y - B;
    
    return Result;
}

inline v2&
operator-=(v2& A, v2 B)
{
    A = A - B;
    
    return A;
}






inline v3
operator*(float A, v3 B)
{
    v3 Result;

    Result.X = A * B.X;
    Result.Y = A * B.Y;
    Result.Z = A * B.Z;
    
    return Result;
}

inline v3
operator*(v3 A, float B)
{
    v3 Result;

    Result.X = B * A.X;
    Result.Y = B * A.Y;
    Result.Z = B * A.Z;
    
    return Result;
}

inline v3 &
operator*=(v3& A, float B)
{
    A = B * A;

    return A;
}

inline v3
operator-(v3 A)
{
    v3 Result;
    
    Result.X = -A.X;
    Result.Y = -A.Y;
    Result.Z = -A.Z;
    
    return Result;
}

inline v3
operator+(v3 A, v3 B)
{
    v3 Result;
    
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    Result.Z = A.Z + B.Z;
    
    return Result;
}

inline v3&
operator+=(v3& A, v3 B)
{
    A = A + B;
    
    return A;
}

inline v3
operator-(v3 A, v3 B)
{
    v3 Result;
    
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    Result.Z = A.Z - B.Z;
    
    return Result;
}

inline v3
operator-(v3 A, float B)
{
    v3 Result;
    
    Result.X = A.X - B;
    Result.Y = A.Y - B;
    Result.Z = A.Z - B;
    
    return Result;
}

inline v3&
operator-=(v3& A, v3 B)
{
    A = A - B;
    
    return A;
}

struct mat4
{
    float Data[16];

inline float
operator[](int Index)
{
    float Result;
    
    Result = Data[Index];
    
    return Result;
}
    
};

inline mat4
Multiply(mat4 A, mat4 B)
{
    mat4 Result =
    {
        A[0] * B[0] + A[1] * B[4] + A[2] * B[8] + A[3] * B[12],
        A[0] * B[1] + A[1] * B[5] + A[2] * B[9] + A[3] * B[13],
        A[0] * B[2] + A[1] * B[6] + A[2] * B[10] + A[3] * B[14],
        A[0] * B[3] + A[1] * B[7] + A[2] * B[11] + A[3] * B[15],

        A[4] * B[0] + A[5] * B[4] + A[6] * B[8] + A[7] * B[12],
        A[4] * B[1] + A[5] * B[5] + A[6] * B[9] + A[7] * B[13],
        A[4] * B[2] + A[5] * B[6] + A[6] * B[10] + A[7] * B[14],
        A[4] * B[3] + A[5] * B[7] + A[6] * B[11] + A[7] * B[15],
        
        A[8] * B[0] + A[9] * B[4] + A[10] * B[8] + A[11] * B[12],
        A[8] * B[1] + A[9] * B[5] + A[10] * B[9] + A[11] * B[13],
        A[8] * B[2] + A[9] * B[6] + A[10] * B[10] + A[11] * B[14],
        A[8] * B[3] + A[9] * B[7] + A[10] * B[11] + A[11] * B[15],

        A[12] * B[0] + A[13] * B[4] + A[14] * B[8] + A[15] * B[12],
        A[12] * B[1] + A[13] * B[5] + A[14] * B[9] + A[15] * B[13],
        A[12] * B[2] + A[13] * B[6] + A[14] * B[10] + A[15] * B[14],
        A[12] * B[3] + A[13] * B[7] + A[14] * B[11] + A[15] * B[15],

    };

    return Result;
}


inline mat4
operator*(mat4 A, mat4 B)
{
    mat4 Result;

    Result = Multiply(A, B);
    
    return Result;
}

inline mat4 &
operator*=(mat4& A, mat4 B)
{
    A = A * B;

    return A;
}


inline mat4
OrthoMatrix(float Left, float Right,
                 float Bottom, float Top,
                 float Near, float Far)
{
    mat4 Result =
        {
            2.f / (Right - Left), 0.f, 0.f, 0.f, 
            0.f, 2.f / (Top - Bottom), 0.f, 0.f, 
            0.f, 0.f, -2 / (Far - Near), 0.f, 
            -(Right + Left) / (Right - Left), -(Top + Bottom)/(Top - Bottom), (Far + Near)/(Far - Near), 1.f
        };

    return Result;
}

inline mat4
PerspectiveMatrix(float Aspect, float Fov, float Far, float Near)
{
    float TanHalfFov = Tan(Fov * 0.5f);
    mat4 Result =
        {
            1.f / (Aspect * TanHalfFov), 0.f, 0.f, 0.f, 
            0.f, 1.f / TanHalfFov, 0.f, 0.f, 
            0.f, 0.f, -(Far + Near) / (Far - Near), -1, 
            0.f, 0.f, - (2 * Far * Near) / (Far - Near), 0.f
        };

    return Result;
}


inline mat4
ScaleMatrix(float X, float Y, float Z)
{
    mat4 Result =
        {
            X, 0.f, 0.f, 0.f, 
            0.f, Y, 0.f, 0.f, 
            0.f, 0.f, Z, 0.f, 
            0.f, 0.f, 0.f, 1.f
        };

    return Result;
}

inline mat4
TranslationMatrix(float X, float Y, float Z)
{
    mat4 Result =
        {
            1.f, 0.f, 0.f, X, 
            0.f, 1.f, 0.f, Y, 
            0.f, 0.f, 1.f, Z, 
            0.f, 0.f, 0.f, 1.f
        };

    return Result;
}


inline float
SquareRoot(float A)
{
    float Result = sqrtf(A);
    return Result;
}

inline float
Square(float A)
{
    float Result = A*A;
    return Result;
}

inline float
Lerp(float A, float t, float B)
{
    float Result = A*(1.f - t) + B*t;
    return Result;
}

inline float
DotProduct(v2 A, v2 B)
{
    float Result = A.X * B.X + A.Y * B.Y;
    return Result;
}

inline float
LengthSq(v2 A)
{
    float Result = DotProduct(A, A);
    return Result;
}

inline float
Length(v2 A)
{
    float Result = SquareRoot(LengthSq(A));
    return Result;
}


//
// NOTE(zoubir): V3
//

inline float
DotProduct(v3 A, v3 B)
{
    float Result = A.X * B.X + A.Y * B.Y + A.Z * B.Z;
    return Result;
}

inline float
LengthSq(v3 A)
{
    float Result = DotProduct(A, A);
    return Result;
}

inline float
Length(v3 A)
{
    float Result = SquareRoot(LengthSq(A));
    return Result;
}

inline v3
Minimum3(v3 A, v3 B)
{
    v3 Result;
    
    Result.X = Minimum(A.X, B.X);
    Result.Y = Minimum(A.Y, B.Y);
    Result.Z = Minimum(A.Z, B.Z);

    return Result;
}

inline v3
Maximum3(v3 A, v3 B)
{
    v3 Result;
    
    Result.X = Maximum(A.X, B.X);
    Result.Y = Maximum(A.Y, B.Y);
    Result.Z = Maximum(A.Z, B.Z);

    return Result;
}

//
// NOTE(zoubir): Rectangle2
//

struct rectangle2
{
    v2 Min;
    v2 Max;
};

inline rectangle2
RectCenterHalfDims(v2 Center, v2 HalfDims)
{
    rectangle2 Result;

    Result.Min = Center - HalfDims;
    Result.Max = Center + HalfDims;

    return Result;
}

inline bool32
RectanglesIntersect(rectangle2 A, rectangle2 B)
{
    bool32 Result =
        (A.Min.X < B.Max.X) &&
        (B.Min.X < A.Max.X) &&
        (A.Min.Y < B.Max.Y) &&
        (B.Min.Y < A.Max.Y);
        
    return Result;
}

//
// NOTE(zoubir): Rectangle3
//

struct rectangle3
{
    v3 Min;
    v3 Max;
};

inline rectangle3
RectMinMax(v3 Min, v3 Max)
{
    rectangle3 Result;

    Result.Min = Min;
    Result.Max = Max;

    return(Result);
}

inline rectangle3
RectCenterHalfDims(v3 Center, v3 HalfDims)
{
    rectangle3 Result;

    Result.Min = Center - HalfDims;
    Result.Max = Center + HalfDims;

    return Result;
}

inline rectangle3
AddRadiusTo(rectangle3 Rect, v3 Radius)
{
    rectangle3 Result;

    Result.Min = Rect.Min + Radius;
    Result.Max = Rect.Max + Radius;
    
    return Result;
}

inline bool32
RectanglesIntersect(rectangle3 A, rectangle3 B)
{
    bool32 Result =
        (A.Min.X < B.Max.X) &&
        (B.Min.X < A.Max.X) &&
        (A.Min.Y < B.Max.Y) &&
        (B.Min.Y < A.Max.Y) &&
        (A.Min.Z < B.Max.Z) &&
        (B.Min.Z < A.Max.Z);
        
    return Result;
}

#endif
