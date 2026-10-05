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

// NOTE: the type byte keeps the type in its low 5 bits; the top three say
// whether a fresh hit, height and velocity follow. Most things stand on
// the ground, many stand still and few were just hit, so those 9 bytes
// are usually left out.
#define NET_ENTITY_TYPE_MASK 0x1f
#define NET_ENTITY_HAS_HIT 0x20
#define NET_ENTITY_HAS_Z 0x40
#define NET_ENTITY_MOVING 0x80

// Whether a value is still nonzero once quantized to Steps per unit.
inline bool32
NetNonZero(float Value, float Steps)
{
    float Scaled = Value * Steps;
    return Scaled >= 0.5f || Scaled <= -0.5f;
}

internal void
NetSerializeEntity(net_stream *S, net_entity_state *E)
{
    NetU16(S, &E->Id);
    u8 TypeAndFlags = 0;
    if (S->Writing)
    {
        if (E->Type > NET_ENTITY_TYPE_MASK) S->Failed = true;
        TypeAndFlags = (u8)(E->Type & NET_ENTITY_TYPE_MASK);
        // A unit thrown up this tick can still be at height 0; its speed
        // goes with the height, so the flag counts either.
        if (NetNonZero(E->Z, NET_POSITION_STEPS) || NetNonZero(E->VelZ, NET_VELOCITY_STEPS))
        {
            TypeAndFlags |= NET_ENTITY_HAS_Z;
        }
        if (NetNonZero(E->VelX, NET_VELOCITY_STEPS) || NetNonZero(E->VelY, NET_VELOCITY_STEPS))
        {
            TypeAndFlags |= NET_ENTITY_MOVING;
        }
        if (E->Hit)
        {
            TypeAndFlags |= NET_ENTITY_HAS_HIT;
        }
    }
    NetU8(S, &TypeAndFlags);
    E->Type = TypeAndFlags & NET_ENTITY_TYPE_MASK;
    // Small fields are packed; out-of-range values are cut to their bits.
    // Status has a fourth bit, which rides in Look's top bit.
    u8 Look = (u8)((E->Facing & 3) | ((E->Animation & 15) << 2) | ((E->Flash & 1) << 6) |
                   (((E->Status >> 3) & 1) << 7));
    NetU8(S, &Look);
    E->Facing = Look & 3;
    E->Animation = (Look >> 2) & 15;
    E->Flash = (Look >> 6) & 1;
    NetU8(S, &E->Variant);
    u8 Extra = (u8)((E->Affix & 7) | ((E->Status & 7) << 3) | ((E->Ability & 3) << 6));
    NetU8(S, &Extra);
    E->Affix = Extra & 7;
    E->Status = (u8)(((Extra >> 3) & 7) | (((Look >> 7) & 1) << 3));
    E->Ability = (Extra >> 6) & 3;
    NetI16(S, &E->Health);
    NetFixed16(S, &E->X, NET_POSITION_STEPS);
    NetFixed16(S, &E->Y, NET_POSITION_STEPS);
    if (TypeAndFlags & NET_ENTITY_HAS_Z)
    {
        NetFixed16(S, &E->Z, NET_POSITION_STEPS);
        NetFixed16(S, &E->VelZ, NET_VELOCITY_STEPS);
    }
    else
    {
        E->Z = E->VelZ = 0.f;
    }
    if (TypeAndFlags & NET_ENTITY_MOVING)
    {
        NetFixed16(S, &E->VelX, NET_VELOCITY_STEPS);
        NetFixed16(S, &E->VelY, NET_VELOCITY_STEPS);
    }
    else
    {
        E->VelX = E->VelY = 0.f;
    }
    E->Hit = (TypeAndFlags & NET_ENTITY_HAS_HIT) ? 1 : 0;
    if (E->Hit)
    {
        NetU8(S, &E->HitStop);
        NetU8(S, &E->HitAngle);
        u8 By = (u8)((E->HitBy & 15) | ((E->HitThrown & 1) << 7));
        NetU8(S, &By);
        E->HitBy = By & 15;
        E->HitThrown = (By >> 7) & 1;
    }
    else
    {
        E->HitStop = E->HitAngle = E->HitBy = E->HitThrown = 0;
    }
}

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
    NetFixed16(S, &R->X, NET_POSITION_STEPS);
    NetFixed16(S, &R->Y, NET_POSITION_STEPS);
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
        NetFixed16(S, &A->PointX[Index], NET_POSITION_STEPS);
        NetFixed16(S, &A->PointY[Index], NET_POSITION_STEPS);
    }
    return true;
}

internal void
NetSerializeInput(net_stream *S, net_input *Input)
{
    NetU32(S, &Input->Tick);
    NetU16(S, &Input->Buttons);
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
            NetU8(S, &P->Snapshot.Stagger);
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
                NetFixed16(S, &Burst->X, NET_POSITION_STEPS);
                NetFixed16(S, &Burst->Y, NET_POSITION_STEPS);
                NetFixed16(S, &Burst->Z, NET_POSITION_STEPS);
            }
            NetU8(S, &P->Snapshot.RewindCount);
            if (P->Snapshot.RewindCount > NET_MAX_SNAPSHOT_REWINDS) return false;
            for (u32 Index = 0; Index < P->Snapshot.RewindCount; ++Index)
            {
                if (!NetSerializeRewind(S, &P->Snapshot.Rewinds[Index], P->Snapshot.Count)) return false;
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

internal u32
NetWriteSnapshotFitting(net_packet *Packet, u8 *Buffer, u32 BufferSize, u32 *Dropped)
{
    net_snapshot *Snapshot = &Packet->Snapshot;
    u32 Start = Snapshot->Count;
    u32 Size = NetWritePacket(Packet, Buffer, BufferSize);
    while (Size == 0 && Snapshot->Count > 1)
    {
        Snapshot->Count--;
        u32 Kept = 0;
        for (u32 Index = 0; Index < Snapshot->AbilityCount; ++Index)
        {
            if (Snapshot->Abilities[Index].EntityIndex < Snapshot->Count)
            {
                Snapshot->Abilities[Kept++] = Snapshot->Abilities[Index];
            }
        }
        Snapshot->AbilityCount = (u8)Kept;
        Kept = 0;
        for (u32 Index = 0; Index < Snapshot->FacingCount; ++Index)
        {
            if (Snapshot->Facings[Index].EntityIndex < Snapshot->Count)
            {
                Snapshot->Facings[Kept++] = Snapshot->Facings[Index];
            }
        }
        Snapshot->FacingCount = (u8)Kept;
        Size = NetWritePacket(Packet, Buffer, BufferSize);
    }
    if (Dropped) *Dropped = Start - Snapshot->Count;
    return Size;
}
