/* The web gateway, declared in web_gateway.h. Needs net/socket.cpp
   compiled in the same unit first. Its parts: websocket.cpp (handshake
   and frames), tcp.cpp (the browser side) and files.cpp (serving a
   folder for local testing). */

#include <ctype.h>
#include <stdlib.h>
#include "web_gateway.h"
#include "tcp.cpp"
#include "websocket.cpp"

// NOTE(zoubir): queues Size bytes for the browser. A WebSocket link
// stops at GATEWAY_OUT_LIMIT and the bytes are dropped (false); a file
// reply grows as far as it needs
internal bool32
GatewayQueue(gateway_link *Link, const u8 *Data, u32 Size, bool32 NoLimit)
{
    if (Link->OutSent == Link->OutUsed)
    {
        Link->OutSent = Link->OutUsed = 0;
    }
    if (!NoLimit && Link->OutUsed - Link->OutSent + Size > GATEWAY_OUT_LIMIT) return false;
    if (Link->OutUsed + Size > Link->OutCapacity)
    {
        // NOTE(zoubir): slide what is left to the front, then grow
        u32 Left = Link->OutUsed - Link->OutSent;
        if (Left) memmove(Link->Out, Link->Out + Link->OutSent, Left);
        Link->OutSent = 0;
        Link->OutUsed = Left;
        if (Left + Size > Link->OutCapacity)
        {
            u32 Capacity = Link->OutCapacity * 2;
            if (Capacity < Left + Size + 4096) Capacity = Left + Size + 4096;
            u8 *Grown = (u8 *)realloc(Link->Out, Capacity);
            if (!Grown) return false;
            Link->Out = Grown;
            Link->OutCapacity = Capacity;
        }
    }
    memcpy(Link->Out + Link->OutUsed, Data, Size);
    Link->OutUsed += Size;
    return true;
}

internal void
GatewayQueueText(gateway_link *Link, const char *Text)
{
    GatewayQueue(Link, (const u8 *)Text, (u32)strlen(Text), true);
}

internal void
GatewayCloseLink(web_gateway *Gateway, gateway_link *Link)
{
    if (Link->State == GatewayLink_Free) return;
    TcpClose(Link->Tcp);
    NetCloseSocket(&Link->Udp);
    free(Link->Out);
    memset(Link, 0, sizeof(*Link));
    Gateway->OpenLinks--;
}

#include "files.cpp"

// NOTE(zoubir): "host:port" or "a.b.c.d:port" to an address
internal bool32
GatewayResolve(const char *Text, net_address *Out)
{
    if (NetParseAddress(Text, Out)) return true;
    const char *Colon = strrchr(Text, ':');
    if (!Colon || Colon == Text || Colon - Text >= 128) return false;
    char Host[128];
    memcpy(Host, Text, (size_t)(Colon - Text));
    Host[Colon - Text] = 0;
    for (char *C = Host; *C; C++)
    {
        bool32 Allowed = (*C >= 'a' && *C <= 'z') || (*C >= 'A' && *C <= 'Z') ||
            (*C >= '0' && *C <= '9') || *C == '-' || *C == '.';
        if (!Allowed) return false;
    }
    int Port = atoi(Colon + 1);
    if (Port <= 0 || Port > 65535) return false;
    u32 Ip;
    if (!NetLookupHost(Host, &Ip)) return false;
    Out->Ip = Ip;
    Out->Port = (u16)Port;
    return true;
}

internal bool32
GatewayStart(web_gateway *Gateway, u16 Port, bool32 Loopback)
{
    memset(Gateway, 0, sizeof(*Gateway));
    if (!NetSocketsStartup()) return false;
    Gateway->Listener = TcpListen(Port, Loopback);
    if (Gateway->Listener == TCP_INVALID) return false;
    sockaddr_in Bound = {};
#if defined(_WIN32)
    int Length = sizeof(Bound);
#else
    socklen_t Length = sizeof(Bound);
#endif
    getsockname(TcpHandle(Gateway->Listener), (sockaddr *)&Bound, &Length);
    Gateway->Port = ntohs(Bound.sin_port);
    return true;
}

internal bool32
GatewayAllow(web_gateway *Gateway, const char *Server)
{
    net_address Address;
    if (Gateway->AllowedCount >= GATEWAY_MAX_ALLOWED || !GatewayResolve(Server, &Address)) return false;
    Gateway->Allowed[Gateway->AllowedCount++] = Address;
    return true;
}

internal void
GatewayServeFiles(web_gateway *Gateway, const char *Folder)
{
    snprintf(Gateway->Files, sizeof(Gateway->Files), "%s", Folder);
}

internal void
GatewayStop(web_gateway *Gateway)
{
    for (u32 Index = 0; Index < GATEWAY_MAX_LINKS; Index++)
    {
        GatewayCloseLink(Gateway, &Gateway->Links[Index]);
    }
    TcpClose(Gateway->Listener);
    Gateway->Listener = TCP_INVALID;
}

// NOTE(zoubir): the opening request asks for a WebSocket to an allowed
// server: answer it and open the UDP side
internal void
GatewayOpenWebSocket(web_gateway *Gateway, gateway_link *Link, ws_request *Request)
{
    net_address Server = {};
    bool32 Allowed = false;
    if (GatewayResolve(Request->Path + 5, &Server))
    {
        for (u32 Index = 0; Index < Gateway->AllowedCount; Index++)
        {
            Allowed |= NetAddressEqual(Gateway->Allowed[Index], Server);
        }
    }
    if (!Allowed)
    {
        GatewayQueueText(Link, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        Link->State = GatewayLink_Closing;
        return;
    }
    Link->Udp = NetOpenSocket(0);
    if (!Link->Udp.Open)
    {
        GatewayQueueText(Link, "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        Link->State = GatewayLink_Closing;
        return;
    }
    char Accept[32];
    WsAcceptKey(Request->Key, Accept);
    char Reply[256];
    snprintf(Reply, sizeof(Reply),
             "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\n"
             "Connection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", Accept);
    GatewayQueueText(Link, Reply);
    Link->Server = Server;
    Link->State = GatewayLink_WebSocket;
}

// NOTE(zoubir): handles the request once its headers are all in; false
// while they are still coming
internal bool32
GatewayReadRequest(web_gateway *Gateway, gateway_link *Link)
{
    // NOTE(zoubir): reads stop one short of In's end, so this fits
    Link->In[Link->InUsed] = 0;
    char *End = strstr((char *)Link->In, "\r\n\r\n");
    if (!End) return false;
    u32 HeaderSize = (u32)(End + 4 - (char *)Link->In);
    ws_request Request;
    bool32 Parsed = WsParseRequest((char *)Link->In, &Request);
    if (Parsed && strncmp(Request.Path, "/net/", 5) == 0 && Request.Upgrade && Request.Key[0])
    {
        GatewayOpenWebSocket(Gateway, Link, &Request);
    }
    else if (Parsed && strncmp(Request.Path, "/net/", 5) == 0)
    {
        // NOTE(zoubir): a plain request here is how deploy/install.sh
        // checks the gateway answers behind Caddy
        GatewayQueueText(Link, "HTTP/1.1 426 Upgrade Required\r\nUpgrade: websocket\r\n"
                         "Content-Length: 0\r\nConnection: close\r\n\r\n");
        Link->State = GatewayLink_Closing;
    }
    else if (Parsed && Gateway->Files[0] && strcmp(Request.Method, "GET") == 0)
    {
        GatewaySendFile(Gateway, Link, Request.Path);
    }
    else
    {
        GatewayQueueText(Link, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
        Link->State = GatewayLink_Closing;
    }
    // NOTE(zoubir): a browser may send its first frame right behind the request
    memmove(Link->In, Link->In + HeaderSize, Link->InUsed - HeaderSize);
    Link->InUsed -= HeaderSize;
    return true;
}

internal void
GatewayReadFrames(web_gateway *Gateway, gateway_link *Link)
{
    local_persist ws_frame Frame;
    u32 At = 0;
    while (Link->State == GatewayLink_WebSocket)
    {
        u32 Used = 0;
        ws_read_result Result = WsReadFrame(Link->In + At, Link->InUsed - At, &Frame, &Used);
        if (Result == WsRead_NeedMore) break;
        if (Result == WsRead_Bad)
        {
            Link->State = GatewayLink_Closing;
            break;
        }
        At += Used;
        if (Frame.Opcode == 2)
        {
            if (Gateway->Now - Link->SecondStart >= 1.0)
            {
                Link->SecondStart = Gateway->Now;
                Link->MessagesThisSecond = 0;
            }
            if (++Link->MessagesThisSecond > GATEWAY_MAX_MESSAGES)
            {
                Link->State = GatewayLink_Closing;
                break;
            }
            if (Frame.Size && Frame.Size <= NET_MAX_PACKET_SIZE)
            {
                NetSendTo(&Link->Udp, Link->Server, Frame.Payload, Frame.Size);
                Gateway->ToServer++;
            }
        }
        else if (Frame.Opcode == 8 || Frame.Opcode == 9)
        {
            u8 Reply[WS_MAX_PAYLOAD + 4];
            u32 Size = WsWriteFrame(Frame.Opcode == 8 ? 8 : 10, Frame.Payload,
                                    (Frame.Opcode == 8 && Frame.Size > 2) ? 2 : Frame.Size, Reply);
            GatewayQueue(Link, Reply, Size, true);
            if (Frame.Opcode == 8) Link->State = GatewayLink_Closing;
        }
    }
    memmove(Link->In, Link->In + At, Link->InUsed - At);
    Link->InUsed -= At;
}

// NOTE(zoubir): everything the browser sent; false when it hung up
internal bool32
GatewayReadBrowser(web_gateway *Gateway, gateway_link *Link)
{
    for (;;)
    {
        if (Link->InUsed >= GATEWAY_IN_SIZE - 1) return false;
        int Got = TcpRead(Link->Tcp, Link->In + Link->InUsed, GATEWAY_IN_SIZE - 1 - Link->InUsed);
        if (Got < 0) return false;
        if (Got == 0) return true;
        Link->InUsed += (u32)Got;
        Link->LastHeard = Gateway->Now;
        if (Link->State == GatewayLink_Http && !GatewayReadRequest(Gateway, Link)) continue;
        if (Link->State == GatewayLink_WebSocket) GatewayReadFrames(Gateway, Link);
        if (Link->State == GatewayLink_Closing) Link->InUsed = 0;
    }
}

internal void
GatewayReadServer(web_gateway *Gateway, gateway_link *Link)
{
    u8 Packet[NET_MAX_PACKET_SIZE];
    u8 Frame[NET_MAX_PACKET_SIZE + 4];
    net_address From;
    u32 Size;
    while ((Size = NetReceiveFrom(&Link->Udp, &From, Packet, sizeof(Packet))) > 0)
    {
        if (!NetAddressEqual(From, Link->Server)) continue;
        Link->LastHeard = Gateway->Now;
        u32 FrameSize = WsWriteFrame(2, Packet, Size, Frame);
        if (GatewayQueue(Link, Frame, FrameSize, false)) Gateway->ToBrowser++;
    }
}

// NOTE(zoubir): false when the browser's connection failed
internal bool32
GatewayFlush(gateway_link *Link)
{
    while (Link->OutSent < Link->OutUsed)
    {
        int Sent = TcpWrite(Link->Tcp, Link->Out + Link->OutSent, Link->OutUsed - Link->OutSent);
        if (Sent < 0) return false;
        if (Sent == 0) break;
        Link->OutSent += (u32)Sent;
    }
    return true;
}

internal void
GatewayAccept(web_gateway *Gateway)
{
    for (;;)
    {
        u64 Tcp = TcpAccept(Gateway->Listener);
        if (Tcp == TCP_INVALID) return;
        gateway_link *Link = 0;
        for (u32 Index = 0; Index < GATEWAY_MAX_LINKS && !Link; Index++)
        {
            if (Gateway->Links[Index].State == GatewayLink_Free) Link = &Gateway->Links[Index];
        }
        if (!Link)
        {
            TcpClose(Tcp);
            continue;
        }
        memset(Link, 0, sizeof(*Link));
        Link->State = GatewayLink_Http;
        Link->Tcp = Tcp;
        Link->LastHeard = Gateway->Now;
        Link->SecondStart = Gateway->Now;
        Gateway->OpenLinks++;
    }
}

internal void
GatewayPoll(web_gateway *Gateway, int TimeoutMs, double Now)
{
    // NOTE(zoubir): entry 0 is the listener, then each link's TCP and UDP
    local_persist tcp_poll_entry Entries[1 + 2 * GATEWAY_MAX_LINKS];
    local_persist u32 Owners[1 + 2 * GATEWAY_MAX_LINKS];
    u32 Count = 0;
    Entries[Count] = {};
    Entries[Count].fd = TcpHandle(Gateway->Listener);
    Entries[Count].events = POLLIN;
    Owners[Count++] = 0;
    for (u32 Index = 0; Index < GATEWAY_MAX_LINKS; Index++)
    {
        gateway_link *Link = &Gateway->Links[Index];
        if (Link->State == GatewayLink_Free) continue;
        Entries[Count] = {};
        Entries[Count].fd = TcpHandle(Link->Tcp);
        Entries[Count].events = (short)(POLLIN | (Link->OutSent < Link->OutUsed ? POLLOUT : 0));
        Owners[Count++] = Index;
        if (Link->Udp.Open)
        {
            Entries[Count] = {};
            Entries[Count].fd = TcpHandle(Link->Udp.Handle);
            Entries[Count].events = POLLIN;
            Owners[Count++] = Index;
        }
    }
    TcpPoll(Entries, Count, TimeoutMs);
    Gateway->Now = Now;

    if (Entries[0].revents & POLLIN) GatewayAccept(Gateway);
    for (u32 Entry = 1; Entry < Count; Entry++)
    {
        gateway_link *Link = &Gateway->Links[Owners[Entry]];
        if (Link->State == GatewayLink_Free || !Entries[Entry].revents) continue;
        bool32 IsUdp = Link->Udp.Open && Entries[Entry].fd == TcpHandle(Link->Udp.Handle);
        if (IsUdp)
        {
            if (Link->State == GatewayLink_WebSocket) GatewayReadServer(Gateway, Link);
        }
        else if ((Entries[Entry].revents & (POLLIN | POLLERR | POLLHUP)) &&
                 !GatewayReadBrowser(Gateway, Link))
        {
            GatewayCloseLink(Gateway, Link);
        }
    }

    for (u32 Index = 0; Index < GATEWAY_MAX_LINKS; Index++)
    {
        gateway_link *Link = &Gateway->Links[Index];
        if (Link->State == GatewayLink_Free) continue;
        bool32 Failed = !GatewayFlush(Link);
        bool32 Done = Link->State == GatewayLink_Closing && Link->OutSent == Link->OutUsed;
        bool32 Idle = Now - Link->LastHeard > GATEWAY_IDLE_SECONDS;
        if (Failed || Done || Idle) GatewayCloseLink(Gateway, Link);
    }
}
