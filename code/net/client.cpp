/* Client connection, declared in client.h. Needs protocol.cpp and
   socket.cpp compiled in the same unit. */

#include "client.h"

internal void
NetClientSend(net_client *Client, net_packet *Packet, u8 Type)
{
    Packet->Header.Type = Type;
    Packet->Header.Sequence = Client->NextSequence++;
    Packet->Header.Ack = Client->NewestReceived;
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(Packet, Buffer, sizeof(Buffer));
    if (Size) NetSendTo(&Client->Socket, Client->Server, Buffer, Size);
}

internal void
NetClientEnd(net_client *Client, net_client_end Reason)
{
    NetCloseSocket(&Client->Socket);
    Client->State = NetClient_Disconnected;
    Client->EndReason = Reason;
}

internal bool32
NetClientConnect(net_client *Client, net_address Server, u32 Salt, u32 ContentId,
                 const char *Name)
{
    *Client = {};
    for (u32 Index = 0; Name && Name[Index] && Index + 1 < NET_NAME_SIZE; ++Index)
    {
        Client->Name[Index] = Name[Index];
    }
    Client->Socket = NetOpenSocket(0);
    if (!Client->Socket.Open) return false;
    Client->Server = Server;
    Client->Salt = Salt;
    Client->ContentId = ContentId;
    Client->State = NetClient_Connecting;
    return true;
}

internal void
NetClientHandle(net_client *Client, net_packet *Packet)
{
    Client->SecondsSinceHeard = 0;
    if (NetSequenceNewer(Packet->Header.Sequence, Client->NewestReceived))
    {
        Client->NewestReceived = Packet->Header.Sequence;
    }

    switch (Packet->Header.Type)
    {
        case NetPacket_ConnectAccepted:
        {
            if (Client->State == NetClient_Connecting && Packet->ConnectAccepted.ClientSalt == Client->Salt)
            {
                Client->State = NetClient_Connected;
                Client->PlayerIndex = Packet->ConnectAccepted.PlayerIndex;
                Client->MapId = Packet->ConnectAccepted.MapId;
                Client->InputTick = Packet->ConnectAccepted.ServerTick;
            }
        } break;

        case NetPacket_ConnectChallenge:
        {
            if (Client->State == NetClient_Connecting &&
                Packet->ConnectChallenge.ClientSalt == Client->Salt)
            {
                Client->Cookie = Packet->ConnectChallenge.Cookie;
                Client->RetryTimer = 0; // answer on this update
            }
        } break;

        case NetPacket_ConnectDenied:
        {
            if (Client->State == NetClient_Connecting && Packet->ConnectDenied.ClientSalt == Client->Salt)
            {
                NetClientEnd(Client, Packet->ConnectDenied.Reason == NetDeny_WrongVersion ?
                             NetEnd_WrongVersion : NetEnd_ServerFull);
            }
        } break;

        case NetPacket_Snapshot:
        {
            if (Client->State == NetClient_Connected &&
                (!Client->HasSnapshot || Packet->Snapshot.Tick > Client->Snapshot.Tick))
            {
                Client->Snapshot = Packet->Snapshot;
                Client->HasSnapshot = true;
            }
        } break;

        case NetPacket_Disconnect:
        {
            if (Client->State == NetClient_Connected) NetClientEnd(Client, NetEnd_ServerClosed);
        } break;

        default: break;
    }
}

internal void
NetClientUpdate(net_client *Client, float Dt, u16 Buttons, float AimX, float AimY)
{
    if (Client->State == NetClient_Disconnected) return;

    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From;
    u32 Size;
    while (Client->State != NetClient_Disconnected &&
           (Size = NetReceiveFrom(&Client->Socket, &From, Buffer, sizeof(Buffer))) != 0)
    {
        if (!NetAddressEqual(From, Client->Server)) continue;
        net_packet Packet;
        if (NetReadPacket(Buffer, Size, &Packet)) NetClientHandle(Client, &Packet);
    }

    net_packet Out = {};
    if (Client->State == NetClient_Connecting)
    {
        Client->SecondsConnecting += Dt;
        Client->RetryTimer -= Dt;
        if (Client->SecondsConnecting >= NET_CONNECT_GIVE_UP)
        {
            NetClientEnd(Client, NetEnd_NoAnswer);
        }
        else if (Client->RetryTimer <= 0)
        {
            Client->RetryTimer = NET_CONNECT_RETRY;
            Out.ConnectRequest.ClientSalt = Client->Salt;
            Out.ConnectRequest.ContentId = Client->ContentId;
            Out.ConnectRequest.Cookie = Client->Cookie;
            for (u32 Index = 0; Index < NET_NAME_SIZE; ++Index)
            {
                Out.ConnectRequest.Name[Index] = Client->Name[Index];
            }
            NetClientSend(Client, &Out, NetPacket_ConnectRequest);
        }
    }
    else if (Client->State == NetClient_Connected)
    {
        Client->SecondsSinceHeard += Dt;
        if (Client->SecondsSinceHeard >= NET_CLIENT_TIMEOUT)
        {
            NetClientEnd(Client, NetEnd_LostConnection);
            return;
        }

        // Shift the history down and put this frame's input first.
        u32 Keep = Client->RecentInputCount < NET_MAX_INPUTS_PER_PACKET ?
            Client->RecentInputCount : NET_MAX_INPUTS_PER_PACKET - 1;
        for (u32 Index = Keep; Index > 0; --Index)
        {
            Client->RecentInputs[Index] = Client->RecentInputs[Index - 1];
        }
        net_input *Newest = &Client->RecentInputs[0];
        Newest->Tick = ++Client->InputTick;
        Newest->Buttons = Buttons;
        Newest->AimX = AimX;
        Newest->AimY = AimY;
        Client->RecentInputCount = Keep + 1;

        Out.Input.Count = (u8)Client->RecentInputCount;
        for (u32 Index = 0; Index < Client->RecentInputCount; ++Index)
        {
            Out.Input.Inputs[Index] = Client->RecentInputs[Index];
        }
        NetClientSend(Client, &Out, NetPacket_Input);
    }
}

internal void
NetClientDisconnect(net_client *Client)
{
    if (Client->State == NetClient_Connected)
    {
        net_packet Bye = {};
        NetClientSend(Client, &Bye, NetPacket_Disconnect);
    }
    if (Client->State != NetClient_Disconnected) NetClientEnd(Client, NetEnd_LeftByChoice);
}
