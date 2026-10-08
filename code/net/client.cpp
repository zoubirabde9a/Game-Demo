/* Client connection, declared in client.h. Needs protocol.cpp and
   socket.cpp compiled in the same unit. */

#include "client.h"

internal bool32
NetResolveServer(const char *Text, net_address *Out)
{
    if (NetParseAddress(Text, Out)) return true;
    // NOTE: "host" or "host:port"; the port is the text after the last colon
    char Host[128];
    u32 Length = 0;
    u32 Colon = 0;
    for (; Text[Length] && Length + 1 < sizeof(Host); ++Length)
    {
        Host[Length] = Text[Length];
        if (Text[Length] == ':') Colon = Length + 1;
    }
    if (Text[Length]) return false;
    Host[Length] = 0;
    u32 Port = NET_DEFAULT_PORT;
    if (Colon)
    {
        Host[Colon - 1] = 0;
        Port = 0;
        for (u32 Index = Colon; Index < Length; ++Index)
        {
            char C = Host[Index];
            if (C < '0' || C > '9') return false;
            Port = Port * 10 + (u32)(C - '0');
            if (Port > 65535) return false;
        }
        if (Port == 0) return false;
    }
    if (!Host[0]) return false;
    for (u32 Index = 0; Host[Index]; ++Index)
    {
        char C = Host[Index];
        bool32 Allowed = (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') ||
            (C >= '0' && C <= '9') || C == '-' || C == '.';
        if (!Allowed) return false;
    }
    u32 Ip;
    if (!NetLookupHost(Host, &Ip)) return false;
    Out->Ip = Ip;
    Out->Port = (u16)Port;
    return true;
}

internal void
NetClientSend(net_client *Client, net_packet *Packet, u8 Type)
{
    Packet->Header.Type = Type;
    Packet->Header.Sequence = Client->NextSequence++;
    Packet->Header.Ack = Client->NewestReceived;
    Packet->Header.Token = Client->Salt;
    u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(Packet, Buffer, sizeof(Buffer));
    if (Size) NetSendTo(&Client->Socket, Client->Server, Buffer, Size);
}

#include "client_chat.cpp"

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
                for (u32 Index = 0; Index < NET_SERVER_NAME_SIZE; ++Index)
                {
                    Client->ServerName[Index] = Packet->ConnectAccepted.ServerName[Index];
                }
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

        case NetPacket_ChatLines:
        {
            if (Client->State == NetClient_Connected) NetClientChatHandle(Client, &Packet->ChatLines);
        } break;

        case NetPacket_Disconnect:
        {
            if (Client->State == NetClient_Connected) NetClientEnd(Client, NetEnd_ServerClosed);
        } break;

        default: break;
    }
}

internal void
NetClientPoll(net_client *Client, float Dt)
{
    if (Client->State == NetClient_Disconnected) return;

    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From;
    u32 Size;
    while (Client->State != NetClient_Disconnected &&
           (Size = NetReceiveFrom(&Client->Socket, &From, Buffer, sizeof(Buffer))) != 0)
    {
        if (!NetAddressEqual(From, Client->Server)) continue;
        u32 ServerProtocol;
        if (Client->State == NetClient_Connecting &&
            NetReadVersionNotice(Buffer, Size, &ServerProtocol))
        {
            NetClientEnd(Client, NetEnd_WrongVersion);
            break;
        }
        net_packet Packet;
        // NOTE: a packet without our token is not from our server
        if (NetReadPacket(Buffer, Size, &Packet) && Packet.Header.Token == Client->Salt)
        {
            NetClientHandle(Client, &Packet);
        }
    }

    if (Client->State == NetClient_Connecting)
    {
        net_packet Out = {};
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
        }
        else
        {
            NetClientChatPoll(Client, Dt);
        }
    }
}

internal u32
NetClientQueueInput(net_client *Client, u32 Buttons, float AimX, float AimY,
                    u16 Target, u8 Role)
{
    if (Client->State != NetClient_Connected) return 0;
    // Shift the history down and put this input first.
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
    Newest->Target = Target;
    Newest->Role = Role;
    Client->RecentInputCount = Keep + 1;
    Client->InputsUnsent++;
    return Newest->Tick;
}

internal void
NetClientFlushInputs(net_client *Client)
{
    if (Client->State != NetClient_Connected || !Client->InputsUnsent) return;
    net_packet Out = {};
    Out.Input.Count = (u8)Client->RecentInputCount;
    for (u32 Index = 0; Index < Client->RecentInputCount; ++Index)
    {
        Out.Input.Inputs[Index] = Client->RecentInputs[Index];
    }
    NetClientSend(Client, &Out, NetPacket_Input);
    Client->InputsUnsent = 0;
}

internal void
NetClientUpdate(net_client *Client, float Dt, u32 Buttons, float AimX, float AimY)
{
    NetClientPoll(Client, Dt);
    NetClientQueueInput(Client, Buttons, AimX, AimY);
    NetClientFlushInputs(Client);
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
