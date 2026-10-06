/* Byte layout of the packets declared in protocol.h.
   Header: u32 protocol id, u8 type, u16 sequence, u16 ack, u32 token. Then the body. */

#include "protocol.h"
#include "serialize.cpp"

#define NET_POSITION_STEPS 8.0f // 1/8 unit
#define NET_VELOCITY_STEPS 4.0f // 1/4 unit per second

internal u32
NetReadU32At(u8 *Buffer)
{
    return (u32)Buffer[0] | ((u32)Buffer[1] << 8) | ((u32)Buffer[2] << 16) | ((u32)Buffer[3] << 24);
}

internal void
NetWriteU32At(u8 *Buffer, u32 Value)
{
    Buffer[0] = (u8)Value;
    Buffer[1] = (u8)(Value >> 8);
    Buffer[2] = (u8)(Value >> 16);
    Buffer[3] = (u8)(Value >> 24);
}

internal bool32
NetIsOtherVersion(u8 *Buffer, u32 Size)
{
    if (Size < 4) return false;
    u32 Id = NetReadU32At(Buffer);
    return Id != NET_PROTOCOL_ID &&
        (Id & NET_PROTOCOL_FAMILY_MASK) == (NET_PROTOCOL_ID & NET_PROTOCOL_FAMILY_MASK);
}

internal void
NetWriteVersionNotice(u8 *Buffer)
{
    NetWriteU32At(Buffer, NET_PROTOCOL_ID);
    NetWriteU32At(Buffer + 4, NET_VERSION_NOTICE_MAGIC);
}

internal bool32
NetReadVersionNotice(u8 *Buffer, u32 Size, u32 *ServerProtocol)
{
    if (Size != NET_VERSION_NOTICE_SIZE || NetReadU32At(Buffer + 4) != NET_VERSION_NOTICE_MAGIC)
    {
        return false;
    }
    *ServerProtocol = NetReadU32At(Buffer);
    return true;
}

internal bool32
NetSequenceNewer(u16 A, u16 B)
{
    u16 Distance = (u16)(A - B);
    return Distance != 0 && Distance < 0x8000;
}

#include "protocol/entities.cpp"

// Slot, kind and phase share a byte. Frozen bits past the snapshot's
// entities (left out to make it fit) read as clear.
internal bool32
NetSerializeRewind(net_stream *S, net_rewind *R, u16 EntityCount)
{
    u8 Head = (u8)((R->Slot & 7) | ((R->Kind & 3) << 3) | ((R->Phase & 3) << 5));
    NetU8(S, &Head);
    R->Slot = Head & 7;
    R->Kind = (Head >> 3) & 3;
    R->Phase = (Head >> 5) & 3;
    u8 Left = 0;
    if (S->Writing)
    {
        float Steps = R->PhaseLeft * 250.f + 0.5f;
        Left = (u8)(Steps <= 0.f ? 0 : (Steps >= 255.f ? 255 : Steps));
    }
    NetU8(S, &Left);
    R->PhaseLeft = (float)Left / 250.f;
    NetPosition(S, &R->X, S->OriginX);
    NetPosition(S, &R->Y, S->OriginY);
    u8 Radius = 0;
    if (S->Writing)
    {
        float Steps = R->Radius * 0.5f + 0.5f;
        Radius = (u8)(Steps <= 0.f ? 0 : (Steps >= 255.f ? 255 : Steps));
    }
    NetU8(S, &Radius);
    R->Radius = 2.f * (float)Radius;
    for (u32 Index = 0; Index < ArrayCount(R->Frozen); ++Index)
    {
        NetU8(S, &R->Frozen[Index]);
    }
    for (u32 Bit = EntityCount; Bit < 8 * ArrayCount(R->Frozen); ++Bit)
    {
        R->Frozen[Bit / 8] &= (u8)~(1u << (Bit % 8));
    }
    return R->Phase != 0;
}

// Returns false if the ability points at an entity the snapshot does not hold.
internal bool32
NetSerializeAbility(net_stream *S, net_ability_state *A, u16 EntityCount)
{
    NetU8(S, &A->EntityIndex);
    NetU8(S, &A->Phase);
    NetU8(S, &A->Ability);
    NetFixed16(S, &A->TimeLeft, 1000.0f); // milliseconds, up to 32 s
    NetUnitFloat(S, &A->AimX);
    NetUnitFloat(S, &A->AimY);
    NetU8(S, &A->PointCount);
    if (A->EntityIndex >= EntityCount || A->PointCount > NET_MAX_ABILITY_POINTS) return false;
    for (u32 Index = 0; Index < A->PointCount; ++Index)
    {
        NetPosition(S, &A->PointX[Index], S->OriginX);
        NetPosition(S, &A->PointY[Index], S->OriginY);
    }
    return true;
}

internal void
NetSerializeInput(net_stream *S, net_input *Input)
{
    NetU32(S, &Input->Tick);
    NetU32(S, &Input->Buttons);
    NetUnitFloat(S, &Input->AimX);
    NetUnitFloat(S, &Input->AimY);
}

// One function both writes and reads, so the two directions cannot drift apart.
internal bool32
NetSerializePacket(net_stream *S, net_packet *P)
{
    u32 ProtocolId = NET_PROTOCOL_ID;
    NetU32(S, &ProtocolId);
    if (ProtocolId != NET_PROTOCOL_ID) return false;

    NetU8(S, &P->Header.Type);
    NetU16(S, &P->Header.Sequence);
    NetU16(S, &P->Header.Ack);
    NetU32(S, &P->Header.Token);

    switch (P->Header.Type)
    {
        case NetPacket_ConnectRequest:
        {
            NetU32(S, &P->ConnectRequest.ClientSalt);
            NetU32(S, &P->ConnectRequest.ContentId);
            NetU32(S, &P->ConnectRequest.Cookie);
            NetName(S, P->ConnectRequest.Name, NET_NAME_SIZE);
        } break;

        case NetPacket_ConnectChallenge:
        {
            NetU32(S, &P->ConnectChallenge.ClientSalt);
            NetU32(S, &P->ConnectChallenge.Cookie);
        } break;

        case NetPacket_InfoRequest:
        {
            NetU32(S, &P->InfoRequest.Nonce);
            u8 Padding[NET_INFO_PADDING] = {};
            NetBytes(S, Padding, NET_INFO_PADDING);
        } break;

        case NetPacket_InfoReply:
        {
            net_info_reply *Info = &P->InfoReply;
            NetU32(S, &Info->Nonce);
            NetU32(S, &Info->ContentId);
            NetU8(S, &Info->MapId);
            NetU8(S, &Info->PlayerCount);
            NetU8(S, &Info->MaxPlayers);
            NetName(S, Info->ServerName, NET_SERVER_NAME_SIZE);
            NetU8(S, &Info->NameCount);
            if (Info->NameCount > NET_MAX_SNAPSHOT_SCORES) return false;
            for (u32 Index = 0; Index < Info->NameCount; ++Index)
            {
                NetName(S, Info->Names[Index], NET_NAME_SIZE);
            }
        } break;

        case NetPacket_ConnectAccepted:
        {
            NetU32(S, &P->ConnectAccepted.ClientSalt);
            NetU8(S, &P->ConnectAccepted.PlayerIndex);
            NetU32(S, &P->ConnectAccepted.ServerTick);
            NetU8(S, &P->ConnectAccepted.MapId);
            NetName(S, P->ConnectAccepted.ServerName, NET_SERVER_NAME_SIZE);
        } break;

        case NetPacket_ConnectDenied:
        {
            NetU32(S, &P->ConnectDenied.ClientSalt);
            NetU8(S, &P->ConnectDenied.Reason);
        } break;

        case NetPacket_Disconnect:
        {
        } break;

        case NetPacket_Input:
        {
            NetU8(S, &P->Input.Count);
            if (P->Input.Count == 0 || P->Input.Count > NET_MAX_INPUTS_PER_PACKET) return false;
            for (u32 Index = 0; Index < P->Input.Count; ++Index)
            {
                NetSerializeInput(S, &P->Input.Inputs[Index]);
            }
        } break;

        case NetPacket_Snapshot:
        {
            NetU32(S, &P->Snapshot.Tick);
            NetU32(S, &P->Snapshot.InputTick);
            NetU8(S, &P->Snapshot.InputBuffered);
            NetF32(S, &P->Snapshot.OriginX);
            NetF32(S, &P->Snapshot.OriginY);
            if (!(P->Snapshot.OriginX == P->Snapshot.OriginX) ||
                !(P->Snapshot.OriginY == P->Snapshot.OriginY)) return false;
            S->OriginX = P->Snapshot.OriginX;
            S->OriginY = P->Snapshot.OriginY;
            NetU16(S, &P->Snapshot.Count);
            if (P->Snapshot.Count > NET_MAX_SNAPSHOT_ENTITIES) return false;
            for (u32 Index = 0; Index < P->Snapshot.Count; ++Index)
            {
                NetSerializeEntity(S, &P->Snapshot.Entities[Index]);
            }
            NetU8(S, &P->Snapshot.AbilityCount);
            if (P->Snapshot.AbilityCount > NET_MAX_SNAPSHOT_ABILITIES) return false;
            for (u32 Index = 0; Index < P->Snapshot.AbilityCount; ++Index)
            {
                if (!NetSerializeAbility(S, &P->Snapshot.Abilities[Index], P->Snapshot.Count)) return false;
            }
            NetU8(S, &P->Snapshot.ScoreCount);
            if (P->Snapshot.ScoreCount > NET_MAX_SNAPSHOT_SCORES) return false;
            for (u32 Index = 0; Index < P->Snapshot.ScoreCount; ++Index)
            {
                net_score *Score = &P->Snapshot.Scores[Index];
                NetU8(S, &Score->Slot);
                NetU16(S, &Score->Kills);
                NetU16(S, &Score->Deaths);
                NetU16(S, &Score->MonsterKills);
                // NOTE(zoubir): the level in 7 bits, the ward in the top one
                u8 LevelAndWard = (u8)((Score->Level & 0x7f) | ((Score->Ward & 1) << 7));
                NetU8(S, &LevelAndWard);
                Score->Level = LevelAndWard & 0x7f;
                Score->Ward = LevelAndWard >> 7;
                if (Score->Slot >= NET_MAX_SNAPSHOT_SCORES) return false;
            }
            NetU8(S, &P->Snapshot.NameSlot);
            if (P->Snapshot.NameSlot != NET_NO_NAME_SLOT)
            {
                if (P->Snapshot.NameSlot >= NET_MAX_SNAPSHOT_SCORES) return false;
                NetName(S, P->Snapshot.Name, NET_NAME_SIZE);
            }
            NetU8(S, &P->Snapshot.FacingCount);
            if (P->Snapshot.FacingCount > NET_MAX_SNAPSHOT_FACINGS) return false;
            for (u32 Index = 0; Index < P->Snapshot.FacingCount; ++Index)
            {
                net_facing *Facing = &P->Snapshot.Facings[Index];
                NetU8(S, &Facing->EntityIndex);
                NetU8(S, &Facing->Angle);
                if (Facing->EntityIndex >= P->Snapshot.Count) return false;
            }
            NetU8(S, &P->Snapshot.SoundCount);
            if (P->Snapshot.SoundCount > NET_MAX_SNAPSHOT_SOUNDS) return false;
            for (u32 Index = 0; Index < P->Snapshot.SoundCount; ++Index)
            {
                NetU8(S, &P->Snapshot.Sounds[Index]);
            }
            for (u32 Index = 0; Index < NET_COOLDOWN_COUNT; ++Index)
            {
                NetU8(S, &P->Snapshot.Cooldowns[Index]);
            }
            NetU16(S, &P->Snapshot.Xp);
            for (u32 Index = 0; Index < NET_TALENT_COUNT; Index += 4)
            {
                u8 Packed = 0;
                for (u32 Part = 0; Part < 4 && Index + Part < NET_TALENT_COUNT; ++Part)
                {
                    Packed |= (u8)((P->Snapshot.TalentRanks[Index + Part] & 3) << (2 * Part));
                }
                NetU8(S, &Packed);
                for (u32 Part = 0; Part < 4 && Index + Part < NET_TALENT_COUNT; ++Part)
                {
                    P->Snapshot.TalentRanks[Index + Part] = (Packed >> (2 * Part)) & 3;
                }
            }
            NetU8(S, &P->Snapshot.Stagger);
            NetU8(S, &P->Snapshot.HasOwnBody);
            if (P->Snapshot.HasOwnBody > 1) return false;
            if (P->Snapshot.HasOwnBody)
            {
                for (u32 Axis = 0; Axis < 3; ++Axis)
                {
                    NetF32(S, &P->Snapshot.OwnPosition[Axis]);
                    NetF32(S, &P->Snapshot.OwnVelocity[Axis]);
                }
                // A mask of the status clocks that run, then each of those.
                u16 Running = 0;
                for (u32 Index = 0; Index < NET_STATUS_COUNT; ++Index)
                {
                    if (P->Snapshot.OwnStatus[Index] != 0.f) Running |= (u16)(1 << Index);
                }
                NetU16(S, &Running);
                for (u32 Index = 0; Index < NET_STATUS_COUNT; ++Index)
                {
                    if (Running & (1 << Index))
                    {
                        NetF32(S, &P->Snapshot.OwnStatus[Index]);
                    }
                    else
                    {
                        P->Snapshot.OwnStatus[Index] = 0.f;
                    }
                }
            }
            NetU8(S, &P->Snapshot.KillCount);
            if (P->Snapshot.KillCount > NET_MAX_SNAPSHOT_KILLS) return false;
            for (u32 Index = 0; Index < P->Snapshot.KillCount; ++Index)
            {
                net_kill *Kill = &P->Snapshot.Kills[Index];
                NetU8(S, &Kill->Killer);
                NetU8(S, &Kill->Victim);
                NetU8(S, &Kill->KillerMonster);
                if (Kill->Victim >= NET_MAX_SNAPSHOT_SCORES) return false;
            }
            NetU8(S, &P->Snapshot.BurstCount);
            if (P->Snapshot.BurstCount > NET_MAX_SNAPSHOT_BURSTS) return false;
            for (u32 Index = 0; Index < P->Snapshot.BurstCount; ++Index)
            {
                net_burst *Burst = &P->Snapshot.Bursts[Index];
                NetU8(S, &Burst->Kind);
                NetU8(S, &Burst->Slot);
                NetU8(S, &Burst->Angle);
                NetPosition(S, &Burst->X, S->OriginX);
                NetPosition(S, &Burst->Y, S->OriginY);
                NetFixed16(S, &Burst->Z, NET_POSITION_STEPS);
            }
            NetU8(S, &P->Snapshot.RewindCount);
            if (P->Snapshot.RewindCount > NET_MAX_SNAPSHOT_REWINDS) return false;
            for (u32 Index = 0; Index < P->Snapshot.RewindCount; ++Index)
            {
                if (!NetSerializeRewind(S, &P->Snapshot.Rewinds[Index], P->Snapshot.Count)) return false;
            }
            NetU8(S, &P->Snapshot.CastCount);
            if (P->Snapshot.CastCount > NET_MAX_SNAPSHOT_CASTS) return false;
            for (u32 Index = 0; Index < P->Snapshot.CastCount; ++Index)
            {
                net_player_cast *Cast = &P->Snapshot.Casts[Index];
                NetU8(S, &Cast->EntityIndex);
                NetU8(S, &Cast->Spell);
                NetU8(S, &Cast->Done);
                if (Cast->EntityIndex >= P->Snapshot.Count) return false;
            }
        } break;

        default: return false;
    }

    return !S->Failed;
}

internal u32
NetWritePacket(net_packet *Packet, u8 *Buffer, u32 BufferSize)
{
    net_stream S = {Buffer, BufferSize, 0, true, false};
    if (!NetSerializePacket(&S, Packet)) return 0;
    return S.At;
}

internal bool32
NetReadPacket(u8 *Buffer, u32 Size, net_packet *Packet)
{
    net_stream S = {Buffer, Size, 0, false, false};
    *Packet = {};
    if (!NetSerializePacket(&S, Packet)) return false;
    return S.At == Size; // trailing bytes mean a corrupt or foreign packet
}

// Keeps the entries of a list that points into the snapshot's entities
// (EntityIndex) whose entity is still in it.
#define NetKeepSentEntries(Snapshot, List, ListCount)                        {                                                                           u32 Kept = 0;                                                           for (u32 Index = 0; Index < (Snapshot)->ListCount; ++Index)             {                                                                           if ((Snapshot)->List[Index].EntityIndex < (Snapshot)->Count)             {                                                                           (Snapshot)->List[Kept++] = (Snapshot)->List[Index];                 }                                                                   }                                                                       (Snapshot)->ListCount = (u8)Kept;                                   }

internal u32
NetWriteSnapshotFitting(net_packet *Packet, u8 *Buffer, u32 BufferSize, u32 *Dropped)
{
    net_snapshot *Snapshot = &Packet->Snapshot;
    u32 Start = Snapshot->Count;
    u32 Size = NetWritePacket(Packet, Buffer, BufferSize);
    while (Size == 0 && Snapshot->Count > 1)
    {
        Snapshot->Count--;
        NetKeepSentEntries(Snapshot, Abilities, AbilityCount);
        NetKeepSentEntries(Snapshot, Facings, FacingCount);
        NetKeepSentEntries(Snapshot, Casts, CastCount);
        Size = NetWritePacket(Packet, Buffer, BufferSize);
    }
    if (Dropped) *Dropped = Start - Snapshot->Count;
    return Size;
}
