/* Talent ranks in a snapshot (protocol.cpp): the viewer's 30 ranks as
   one run of bits, 2 for each duel talent (at most 3) and 3 for each
   class talent from NET_TALENT_WIDE_FIRST on (at most 4), in
   NET_TALENT_BYTES bytes. */

internal void
NetSerializeTalentRanks(net_stream *S, u8 *Ranks)
{
    u8 Packed[NET_TALENT_BYTES] = {};
    u32 Bit = 0;
    for (u32 Index = 0; Index < NET_TALENT_COUNT; ++Index)
    {
        u32 Width = Index < NET_TALENT_WIDE_FIRST ? 2 : 3;
        u32 Rank = Ranks[Index] & ((1u << Width) - 1);
        for (u32 Part = 0; Part < Width; ++Part, ++Bit)
        {
            Packed[Bit / 8] |= (u8)(((Rank >> Part) & 1) << (Bit % 8));
        }
    }
    for (u32 Byte = 0; Byte < NET_TALENT_BYTES; ++Byte)
    {
        NetU8(S, &Packed[Byte]);
    }
    Bit = 0;
    for (u32 Index = 0; Index < NET_TALENT_COUNT; ++Index)
    {
        u32 Width = Index < NET_TALENT_WIDE_FIRST ? 2 : 3;
        u32 Rank = 0;
        for (u32 Part = 0; Part < Width; ++Part, ++Bit)
        {
            Rank |= (u32)((Packed[Bit / 8] >> (Bit % 8)) & 1) << Part;
        }
        Ranks[Index] = (u8)Rank;
    }
}
