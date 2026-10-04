/* Server browser: asks every server in the list (server_list.cpp) who is
   playing, without joining, and times the answer. The connect screen
   shows the result: the server's own name, its map, players, ping, and
   whether it runs this build. Runs only while something calls
   UpdateServerBrowser, so it sends nothing during play.

   One info request per server every SERVER_BROWSER_INTERVAL seconds,
   from its own socket. A server that has not answered for
   SERVER_BROWSER_SILENT seconds shows as not answering. The browser
   build has no UDP and reports nothing. */

#define SERVER_BROWSER_INTERVAL 2.f
#define SERVER_BROWSER_SILENT 5.f

enum server_reach
{
    ServerReach_Unknown,      // no answer yet, still asking
    ServerReach_Up,           // answered and runs this build
    ServerReach_OtherVersion, // answered but runs another build
    ServerReach_Silent,       // no answer for a while
    ServerReach_BadAddress,   // the address does not resolve
};

struct server_status
{
    server_reach Reach;
    float PingMs;
    u8 Players;
    u8 MaxPlayers;
    u8 MapId;
    char Name[NET_SERVER_NAME_SIZE]; // what the server calls itself, "" until it answers
#if !COMPILER_EMSCRIPTEN
    // Implementation detail below; read the fields above.
    bool32 Resolved;
    net_address Address;
    u32 Nonce;
    float SentAt;
    float HeardAt;
#endif
};

struct server_browser
{
    server_status Servers[SERVER_LIST_COUNT];
#if !COMPILER_EMSCRIPTEN
    net_socket Socket;
    float Clock;
    float NextRound;
    u32 NextNonce;
#endif
};

#if !COMPILER_EMSCRIPTEN

internal void
ServerBrowserReceive(server_browser *Browser)
{
    u8 Buffer[NET_MAX_PACKET_SIZE];
    net_address From;
    u32 Size;
    while ((Size = NetReceiveFrom(&Browser->Socket, &From, Buffer, sizeof(Buffer))) != 0)
    {
        for(u32 Index = 0; Index < SERVER_LIST_COUNT; Index++)
        {
            server_status *Status = &Browser->Servers[Index];
            if (!Status->Resolved || !NetAddressEqual(From, Status->Address))
            {
                continue;
            }
            u32 ServerProtocol;
            net_packet Packet;
            if (NetReadVersionNotice(Buffer, Size, &ServerProtocol))
            {
                Status->Reach = ServerReach_OtherVersion;
                Status->HeardAt = Browser->Clock;
            }
            else if (NetReadPacket(Buffer, Size, &Packet) &&
                     Packet.Header.Type == NetPacket_InfoReply &&
                     Packet.InfoReply.Nonce == Status->Nonce)
            {
                net_info_reply *Info = &Packet.InfoReply;
                Status->Reach = (Info->ContentId == SimContentId()) ?
                    ServerReach_Up : ServerReach_OtherVersion;
                Status->PingMs = 1000.f * (Browser->Clock - Status->SentAt);
                Status->Players = Info->PlayerCount;
                Status->MaxPlayers = Info->MaxPlayers;
                Status->MapId = Info->MapId;
                CopyString(Status->Name, sizeof(Status->Name), Info->ServerName);
                Status->HeardAt = Browser->Clock;
            }
        }
    }
}

internal void
UpdateServerBrowser(server_browser *Browser, float DeltaTime)
{
    if (!Browser->Socket.Open)
    {
        if (!NetSocketsStartup())
        {
            return;
        }
        Browser->Socket = NetOpenSocket(0);
        Browser->NextNonce = (u32)time(0) ^ (u32)(size_t)Browser;
        // NOTE(zoubir): a DNS name is looked up once, here, as the screen
        // opens; looking it up blocks until the name server answers
        for(u32 Index = 0; Index < SERVER_LIST_COUNT; Index++)
        {
            server_status *Status = &Browser->Servers[Index];
            Status->Resolved = NetResolveServer(ServerList[Index].Address, &Status->Address);
            Status->Reach = Status->Resolved ? ServerReach_Unknown : ServerReach_BadAddress;
        }
    }
    Browser->Clock += DeltaTime;
    ServerBrowserReceive(Browser);
    if (Browser->Clock >= Browser->NextRound)
    {
        Browser->NextRound = Browser->Clock + SERVER_BROWSER_INTERVAL;
        for(u32 Index = 0; Index < SERVER_LIST_COUNT; Index++)
        {
            server_status *Status = &Browser->Servers[Index];
            if (!Status->Resolved)
            {
                continue;
            }
            if (Status->Reach != ServerReach_Unknown &&
                Browser->Clock - Status->HeardAt > SERVER_BROWSER_SILENT)
            {
                Status->Reach = ServerReach_Silent;
            }
            net_packet Request = {};
            Request.Header.Type = NetPacket_InfoRequest;
            Status->Nonce = ++Browser->NextNonce;
            Request.Header.Token = Status->Nonce;
            Request.InfoRequest.Nonce = Status->Nonce;
            u8 Buffer[NET_MAX_PACKET_SIZE];
            u32 Size = NetWritePacket(&Request, Buffer, sizeof(Buffer));
            if (Size && NetSendTo(&Browser->Socket, Status->Address, Buffer, Size))
            {
                Status->SentAt = Browser->Clock;
            }
        }
        // NOTE(zoubir): a server never heard from is silent after a while too
        for(u32 Index = 0; Index < SERVER_LIST_COUNT; Index++)
        {
            server_status *Status = &Browser->Servers[Index];
            if (Status->Resolved && Status->Reach == ServerReach_Unknown &&
                Browser->Clock > SERVER_BROWSER_SILENT)
            {
                Status->Reach = ServerReach_Silent;
            }
        }
    }
}

// NOTE(zoubir): stops asking and frees the socket; the next update starts over
internal void
StopServerBrowser(server_browser *Browser)
{
    NetCloseSocket(&Browser->Socket);
    *Browser = {};
}

#else

internal void UpdateServerBrowser(server_browser *Browser, float DeltaTime) {}
internal void StopServerBrowser(server_browser *Browser) {}

#endif
