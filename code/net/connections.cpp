/* Server-side client table declared in connections.h. */

#include "connections.h"

internal net_client_slot *
NetFindClient(net_server_clients *Clients, net_address Address, u32 *IndexOut)
{
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        net_client_slot *Slot = &Clients->Slots[Index];
        if (Slot->Connected && NetAddressEqual(Slot->Address, Address))
        {
            *IndexOut = Index;
            return Slot;
        }
    }
    return 0;
}

internal void
NetServerStampHeader(net_client_slot *Slot, net_packet *Packet, u8 Type)
{
    Packet->Header.Type = Type;
    Packet->Header.Sequence = Slot->NextSequence++;
    Packet->Header.Ack = Slot->NewestReceived;
}

internal void
NetFillAccepted(net_client_slot *Slot, u32 SlotIndex, u32 ServerTick, u8 MapId,
                net_receive_result *Result)
{
    Result->HasReply = true;
    NetServerStampHeader(Slot, &Result->Reply, NetPacket_ConnectAccepted);
    Result->Reply.ConnectAccepted.ClientSalt = Slot->Salt;
    Result->Reply.ConnectAccepted.PlayerIndex = (u8)SlotIndex;
    Result->Reply.ConnectAccepted.ServerTick = ServerTick;
    Result->Reply.ConnectAccepted.MapId = MapId;
}

// Cookies change every 512 ticks (about 8.5 s at 60 Hz); the previous
// window still counts, so a handshake never straddles a change and fails.
#define NET_COOKIE_WINDOW_SHIFT 9

internal u32
NetCookie(u32 Secret, net_address Address, u32 Salt, u32 Window)
{
    u32 Hash = Secret ^ 0x9e3779b9u;
    u32 Parts[4] = {Address.Ip, Address.Port, Salt, Window};
    for (u32 Index = 0; Index < 4; ++Index)
    {
        Hash ^= Parts[Index];
        Hash *= 0x85ebca6bu;
        Hash ^= Hash >> 13;
        Hash *= 0xc2b2ae35u;
        Hash ^= Hash >> 16;
    }
    return Hash | 1; // never 0, which means "no cookie"
}

internal bool32
NetCookieValid(net_server_clients *Clients, net_address From, u32 Salt, u32 Cookie, u32 ServerTick)
{
    u32 Window = ServerTick >> NET_COOKIE_WINDOW_SHIFT;
    bool32 Result = Cookie != 0 &&
        (Cookie == NetCookie(Clients->Secret, From, Salt, Window) ||
         (Window > 0 && Cookie == NetCookie(Clients->Secret, From, Salt, Window - 1)));
    return Result;
}

internal net_receive_result
NetServerReceive(net_server_clients *Clients, net_address From, net_packet *Packet, u32 ServerTick)
{
    net_receive_result Result = {};
    u32 SlotIndex = 0;
    net_client_slot *Slot = NetFindClient(Clients, From, &SlotIndex);

    if (Packet->Header.Type == NetPacket_ConnectRequest)
    {
        u32 Salt = Packet->ConnectRequest.ClientSalt;
        u32 ContentId = Packet->ConnectRequest.ContentId;
        if (ContentId != 0 && Clients->ContentId != 0 && ContentId != Clients->ContentId)
        {
            // A different build would misread every snapshot; refuse it
            // before it takes a slot.
            Result.Event = NetReceive_Denied;
            Result.HasReply = true;
            Result.Reply.Header.Type = NetPacket_ConnectDenied;
            Result.Reply.ConnectDenied.ClientSalt = Salt;
            Result.Reply.ConnectDenied.Reason = NetDeny_WrongVersion;
            return Result;
        }

        // A stranger, or a known address with a new salt, must first show
        // it receives at From.
        if (Clients->RequireCookie && !(Slot && Slot->Salt == Salt) &&
            !NetCookieValid(Clients, From, Salt, Packet->ConnectRequest.Cookie, ServerTick))
        {
            Result.Event = NetReceive_Ignored;
            Result.HasReply = true;
            Result.Reply.Header.Type = NetPacket_ConnectChallenge;
            Result.Reply.ConnectChallenge.ClientSalt = Salt;
            Result.Reply.ConnectChallenge.Cookie =
                NetCookie(Clients->Secret, From, Salt, ServerTick >> NET_COOKIE_WINDOW_SHIFT);
            return Result;
        }

        if (Slot && Slot->Salt == Salt)
        {
            Slot->SecondsSinceHeard = 0;
            Result.Event = NetReceive_Rejoined;
            Result.SlotIndex = SlotIndex;
            NetFillAccepted(Slot, SlotIndex, ServerTick, Clients->MapId, &Result);
            return Result;
        }

        if (!Slot)
        {
            for (SlotIndex = 0; SlotIndex < NET_MAX_CLIENTS; ++SlotIndex)
            {
                if (!Clients->Slots[SlotIndex].Connected) break;
            }
        }
        // else: same address with a new salt means the client restarted.
        // It keeps its slot, and the Joined event below tells the game to
        // start that slot over.

        if (SlotIndex == NET_MAX_CLIENTS)
        {
            Result.Event = NetReceive_Denied;
            Result.HasReply = true;
            Result.Reply.Header.Type = NetPacket_ConnectDenied;
            Result.Reply.ConnectDenied.ClientSalt = Salt;
            Result.Reply.ConnectDenied.Reason = NetDeny_ServerFull;
            return Result;
        }

        Slot = &Clients->Slots[SlotIndex];
        *Slot = {};
        Slot->Connected = true;
        Slot->Address = From;
        Slot->Salt = Salt;
        Slot->NewestReceived = Packet->Header.Sequence;
        Result.Event = NetReceive_Joined;
        Result.SlotIndex = SlotIndex;
        for (u32 Index = 0; Index < NET_NAME_SIZE; ++Index)
        {
            Result.Name[Index] = Packet->ConnectRequest.Name[Index];
        }
        NetFillAccepted(Slot, SlotIndex, ServerTick, Clients->MapId, &Result);
        return Result;
    }

    if (!Slot) return Result;

    Slot->SecondsSinceHeard = 0;
    Result.SlotIndex = SlotIndex;
    if (NetSequenceNewer(Packet->Header.Sequence, Slot->NewestReceived))
    {
        Slot->NewestReceived = Packet->Header.Sequence;
    }

    if (Packet->Header.Type == NetPacket_Disconnect)
    {
        *Slot = {};
        Result.Event = NetReceive_Left;
    }
    else if (Packet->Header.Type == NetPacket_Input)
    {
        // Inputs arrive newest first and overlap with earlier packets.
        // Walk oldest to newest and keep only ticks we have not applied.
        net_input_batch *Batch = &Packet->Input;
        for (i32 Index = (i32)Batch->Count - 1; Index >= 0; --Index)
        {
            net_input *Input = &Batch->Inputs[Index];
            if (!Slot->HasInput || Input->Tick > Slot->NewestInputTick)
            {
                Result.NewInputs[Result.NewInputCount++] = *Input;
                Slot->NewestInputTick = Input->Tick;
                Slot->HasInput = true;
            }
        }
        Result.Event = Result.NewInputCount ? NetReceive_Inputs : NetReceive_Ignored;
    }

    return Result;
}

internal u32
NetServerAdvance(net_server_clients *Clients, float Dt)
{
    u32 TimedOut = 0;
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        net_client_slot *Slot = &Clients->Slots[Index];
        if (!Slot->Connected) continue;
        Slot->SecondsSinceHeard += Dt;
        if (Slot->SecondsSinceHeard >= NET_CLIENT_TIMEOUT)
        {
            *Slot = {};
            TimedOut |= 1u << Index;
        }
    }
    return TimedOut;
}
