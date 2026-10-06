/* Bounds-checked little-endian byte stream used by protocol.cpp.
   The same calls write when Writing is set and read otherwise. Running
   past the end sets Failed, and every read from then on returns zero. */

struct net_stream
{
    u8 *Data;
    u32 Size;
    u32 At;
    bool32 Writing;
    bool32 Failed;
    // A snapshot's positions on the ground are sent from this point (the
    // viewer), see NetPosition in protocol.cpp
    float OriginX, OriginY;
};

internal void
NetBytes(net_stream *S, u8 *Bytes, u32 Count)
{
    if (S->Failed || S->Size - S->At < Count)
    {
        S->Failed = true;
        if (!S->Writing)
        {
            for (u32 Index = 0; Index < Count; ++Index) Bytes[Index] = 0;
        }
        return;
    }

    for (u32 Index = 0; Index < Count; ++Index)
    {
        if (S->Writing) S->Data[S->At + Index] = Bytes[Index];
        else Bytes[Index] = S->Data[S->At + Index];
    }
    S->At += Count;
}

// Integers go through explicit shifts so the layout does not depend on the CPU.
internal void NetU8(net_stream *S, u8 *Value) { NetBytes(S, Value, 1); }

internal void
NetU16(net_stream *S, u16 *Value)
{
    u8 B[2] = {(u8)*Value, (u8)(*Value >> 8)};
    NetBytes(S, B, 2);
    *Value = (u16)(B[0] | (B[1] << 8));
}

internal void
NetU32(net_stream *S, u32 *Value)
{
    u8 B[4] = {(u8)*Value, (u8)(*Value >> 8), (u8)(*Value >> 16), (u8)(*Value >> 24)};
    NetBytes(S, B, 4);
    *Value = (u32)B[0] | ((u32)B[1] << 8) | ((u32)B[2] << 16) | ((u32)B[3] << 24);
}

internal void NetI16(net_stream *S, i16 *Value) { NetU16(S, (u16 *)Value); }

// A short player name: length byte, then that many bytes, no terminator.
// Name holds Capacity bytes including the terminator. Reading keeps only
// printable ASCII (anything else becomes '?'), since it came off the net.
internal void
NetName(net_stream *S, char *Name, u32 Capacity)
{
    u8 Length = 0;
    if (S->Writing)
    {
        while (Length + 1u < Capacity && Name[Length]) ++Length;
    }
    NetU8(S, &Length);
    if (Length + 1u > Capacity)
    {
        S->Failed = true;
        Length = 0;
    }
    NetBytes(S, (u8 *)Name, Length);
    if (!S->Writing)
    {
        for (u32 Index = 0; Index < Length; ++Index)
        {
            if (Name[Index] < 32 || Name[Index] > 126) Name[Index] = '?';
        }
        Name[Length] = 0;
    }
}

internal void
NetF32(net_stream *S, float *Value)
{
    union { float F; u32 U; } Bits;
    Bits.F = *Value;
    NetU32(S, &Bits.U);
    *Value = Bits.F;
}

// A float sent as an i16 count of 1/UnitsPerStep steps, rounded to nearest.
// Values beyond what fits are clamped; NaN becomes 0.
internal void
NetFixed16(net_stream *S, float *Value, float StepsPerUnit)
{
    float Steps = *Value * StepsPerUnit;
    if (!(Steps == Steps)) Steps = 0;
    if (Steps > 32767.0f) Steps = 32767.0f;
    if (Steps < -32767.0f) Steps = -32767.0f;
    i16 Quantized = (i16)(Steps + (Steps < 0 ? -0.5f : 0.5f));
    NetI16(S, &Quantized);
    *Value = (float)Quantized / StepsPerUnit;
}

// A float in -1..1 sent as an i16. Out-of-range values are clamped first.
internal void
NetUnitFloat(net_stream *S, float *Value)
{
    float V = *Value;
    if (!(V >= -1.0f)) V = -1.0f; // also catches NaN
    if (V > 1.0f) V = 1.0f;
    i16 Quantized = (i16)(V * 32767.0f + (V < 0 ? -0.5f : 0.5f));
    NetI16(S, &Quantized);
    if (Quantized < -32767) Quantized = -32767;
    *Value = (float)Quantized / 32767.0f;
}
