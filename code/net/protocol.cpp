/* Byte layout of the packets declared in protocol.h.
   Header: u32 protocol id, u8 type, u16 sequence, u16 ack. Then the body. */

#include "protocol.h"
#include "serialize.cpp"

#define NET_POSITION_STEPS 8.0f // 1/8 unit
#define NET_VELOCITY_STEPS 4.0f // 1/4 unit per second

internal bool32
NetSequenceNewer(u16 A, u16 B)
{
    u16 Distance = (u16)(A - B);
    return Distance != 0 && Distance < 0x8000;
}

internal void
NetSerializeEntity(net_stream *S, net_entity_state *E)
{
    NetU16(S, &E->Id);
    NetU8(S, &E->Type);
    NetU8(S, &E->Facing);
    NetU8(S, &E->Animation);
    NetU8(S, &E->Variant);
    NetI16(S, &E->Health);
    NetFixed16(S, &E->X, NET_POSITION_STEPS);
    NetFixed16(S, &E->Y, NET_POSITION_STEPS);
    NetFixed16(S, &E->Z, NET_POSITION_STEPS);
    NetFixed16(S, &E->VelX, NET_VELOCITY_STEPS);
    NetFixed16(S, &E->VelY, NET_VELOCITY_STEPS);
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

    switch (P->Header.Type)
    {
        case NetPacket_ConnectRequest:
        {
            NetU32(S, &P->ConnectRequest.ClientSalt);
        } break;

        case NetPacket_ConnectAccepted:
        {
            NetU32(S, &P->ConnectAccepted.ClientSalt);
            NetU8(S, &P->ConnectAccepted.PlayerIndex);
            NetU32(S, &P->ConnectAccepted.ServerTick);
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
