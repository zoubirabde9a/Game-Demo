/* Burst heights on the wire (protocol.cpp, net_burst in protocol.h). */

// NOTE(zoubir): a class burst carries a small number (a radius, a bolt's
// length, Icicles spent) as whole NET_BURST_STEP steps added to its height
// (RangerBurstSpot and the like, sim/dungeon/role_kits/); a plain i16 in
// eighths stops at 4096 and cut every number to 1. Burst kinds stay under
// NET_BURST_NUMBER, so its bit on the kind byte says a number follows: one
// byte, or two from 128 up (to 32767). The height left over goes last.
#define NET_BURST_STEP 4096.f
#define NET_BURST_NUMBER 0x80u

internal bool32
NetBurstKindAndHeight(net_stream *S, u8 *Kind, float *Z)
{
    u32 Steps = 0;
    if (S->Writing)
    {
        // NOTE(zoubir): truncation is the floor here, as below 0 is 0
        float Whole = (*Z + 0.5f * NET_BURST_STEP) / NET_BURST_STEP;
        Steps = Whole > 32767.f ? 32767u : (Whole > 0.f ? (u32)Whole : 0u);
        if (*Kind & NET_BURST_NUMBER) return false;
    }
    u8 KindByte = (u8)(*Kind | (Steps ? NET_BURST_NUMBER : 0));
    u8 Low = 0;
    NetU8(S, &KindByte);
    *Kind = (u8)(KindByte & ~NET_BURST_NUMBER);
    if (KindByte & NET_BURST_NUMBER)
    {
        Low = (u8)((Steps & 0x7F) | (Steps > 0x7F ? 0x80 : 0));
        NetU8(S, &Low);
        u8 High = (u8)(Steps >> 7);
        if (Low & 0x80)
        {
            NetU8(S, &High);
        }
        else
        {
            High = 0;
        }
        Steps = (Low & 0x7Fu) | ((u32)High << 7);
    }
    else
    {
        Steps = 0;
    }
    float Left = *Z - NET_BURST_STEP * (float)Steps;
    float Lowest = -0.5f * NET_BURST_STEP;
    float Highest = 0.5f * NET_BURST_STEP - 1.f / NET_POSITION_STEPS;
    if (S->Writing)
    {
        Left = Left < Lowest ? Lowest : (Left > Highest ? Highest : Left);
    }
    NetFixed16(S, &Left, NET_POSITION_STEPS);
    *Z = NET_BURST_STEP * (float)Steps + Left;
    // NOTE(zoubir): only what a write makes is read, so whatever is read
    // writes back the same size: a number of 0 flagged, a two-byte number
    // under 128 or a height left over that rounds to another number is
    // refused
    bool32 Result = !(KindByte & NET_BURST_NUMBER) || (Steps > 0 && (Steps > 0x7F || !(Low & 0x80)));
    Result = Result && Left >= Lowest && Left <= Highest;
    return Result;
}
