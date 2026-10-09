/* Web gateway tests (server/web_gateway/), run by net_tests.cpp: the
   WebSocket handshake key and frames, and a whole trip in one process: a
   test "browser" on TCP asks for a WebSocket, its message reaches a UDP
   "server" as one datagram, and the server's answer comes back as one
   frame, sent on to the address its allowed entry routes to. Also that a
   server not on the allowed list is refused, that an
   idle connection is closed, and the 426 a plain request to /net/ gets. */

#include "../server/web_gateway/web_gateway.cpp"

internal void
TestWebSocketAcceptKey()
{
    // NOTE(zoubir): the example in RFC 6455, section 1.3
    char Accept[32];
    WsAcceptKey("dGhlIHNhbXBsZSBub25jZQ==", Accept);
    Check(strcmp(Accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") == 0);

    ws_request Request;
    Check(WsParseRequest("GET /net/127.0.0.1:27015 HTTP/1.1\r\nHost: x\r\n"
                         "upgrade: WebSocket\r\nSec-WebSocket-Key: abc==\r\n\r\n", &Request));
    Check(strcmp(Request.Path, "/net/127.0.0.1:27015") == 0);
    Check(Request.Upgrade);
    Check(strcmp(Request.Key, "abc==") == 0);
}

// NOTE(zoubir): a frame as a browser sends it: masked
internal u32
TestClientFrame(u8 Opcode, const u8 *Payload, u32 Size, u8 *Out)
{
    u8 Mask[4] = {0x12, 0x34, 0x56, 0x78};
    u32 At = 0;
    Out[At++] = (u8)(0x80 | Opcode);
    if (Size < 126)
    {
        Out[At++] = (u8)(0x80 | Size);
    }
    else
    {
        Out[At++] = 0x80 | 126;
        Out[At++] = (u8)(Size >> 8);
        Out[At++] = (u8)Size;
    }
    memcpy(Out + At, Mask, 4);
    At += 4;
    for (u32 Index = 0; Index < Size; Index++) Out[At + Index] = Payload[Index] ^ Mask[Index & 3];
    return At + Size;
}

internal void
TestWebSocketFrames()
{
    static ws_frame Frame;
    u8 Payload[300];
    for (u32 Index = 0; Index < sizeof(Payload); Index++) Payload[Index] = (u8)(Index * 7);
    u8 Wire[400];
    u32 Size = TestClientFrame(2, Payload, sizeof(Payload), Wire);
    u32 Used = 0;
    Check(WsReadFrame(Wire, Size - 1, &Frame, &Used) == WsRead_NeedMore);
    Check(WsReadFrame(Wire, Size, &Frame, &Used) == WsRead_Frame);
    Check(Used == Size && Frame.Opcode == 2 && Frame.Size == sizeof(Payload));
    Check(memcmp(Frame.Payload, Payload, sizeof(Payload)) == 0);

    // NOTE(zoubir): a server's frame is not masked, and a browser's must be
    u32 ServerSize = WsWriteFrame(2, Payload, sizeof(Payload), Wire);
    Check(ServerSize == sizeof(Payload) + 4 && Wire[0] == 0x82 && Wire[1] == 126);
    Check(WsReadFrame(Wire, ServerSize, &Frame, &Used) == WsRead_Bad);

    u8 Huge[10] = {0x82, 0x80 | 127, 0, 0, 0, 0, 0, 1, 0, 0};
    Check(WsReadFrame(Huge, sizeof(Huge), &Frame, &Used) == WsRead_Bad);
    u8 Fragment[6] = {0x02, 0x80, 1, 2, 3, 4};
    Check(WsReadFrame(Fragment, sizeof(Fragment), &Frame, &Used) == WsRead_Bad);
}

// NOTE(zoubir): polls the gateway and reads what the test browser got,
// until Want bytes are in or a second has passed
internal u32
TestGatewayRead(web_gateway *Gateway, u64 Browser, u8 *Buffer, u32 Size, u32 Want, double *Now)
{
    u32 Got = 0;
    for (u32 Round = 0; Round < 100 && Got < Want; Round++)
    {
        GatewayPoll(Gateway, 10, *Now);
        *Now += 0.01;
        int Read = TcpRead(Browser, Buffer + Got, Size - 1 - Got);
        if (Read > 0) Got += (u32)Read;
        if (Read < 0) break;
    }
    Buffer[Got] = 0;
    return Got;
}

internal void
TestGatewayRelaysPackets()
{
    static web_gateway Gateway;
    Check(GatewayStart(&Gateway, 0, true));
    net_socket Server = NetOpenSocket(0);
    Check(Server.Open);
    // NOTE(zoubir): browsers ask for 10.9.9.9 and the packets go to this
    // machine, as a containerised gateway sends its host's public address
    // to the host's address on the container network
    char Allowed[64];
    char Route[96];
    snprintf(Allowed, sizeof(Allowed), "10.9.9.9:%u", NetSocketPort(&Server));
    snprintf(Route, sizeof(Route), "%s=127.0.0.1:%u", Allowed, NetSocketPort(&Server));
    Check(GatewayAllow(&Gateway, Route));

    double Now = 100.0;
    u64 Browser = TcpConnectLoopback(Gateway.Port);
    Check(Browser != TCP_INVALID);
    char Request[256];
    snprintf(Request, sizeof(Request),
             "GET /net/%s HTTP/1.1\r\nHost: test\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
             "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n", Allowed);
    // NOTE(zoubir): the first message right behind the request, as a
    // browser may send it
    u8 Wire[512];
    u32 Size = (u32)strlen(Request);
    memcpy(Wire, Request, Size);
    Size += TestClientFrame(2, (const u8 *)"hello", 5, Wire + Size);
    Check(TcpWrite(Browser, Wire, Size) == (int)Size);

    u8 Reply[1024];
    TestGatewayRead(&Gateway, Browser, Reply, sizeof(Reply), 129, &Now);
    Check(strstr((char *)Reply, "101 Switching Protocols") != 0);
    Check(strstr((char *)Reply, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != 0);

    u8 Packet[64];
    net_address From = {};
    u32 Got = 0;
    for (u32 Round = 0; Round < 100 && !Got; Round++)
    {
        GatewayPoll(&Gateway, 10, Now);
        Got = NetReceiveFrom(&Server, &From, Packet, sizeof(Packet));
    }
    Check(Got == 5 && memcmp(Packet, "hello", 5) == 0);
    Check(Gateway.ToServer == 1);

    // NOTE(zoubir): the server answers the gateway's address as it would any client
    Check(NetSendTo(&Server, From, (u8 *)"world!", 6));
    u32 Read = TestGatewayRead(&Gateway, Browser, Reply, sizeof(Reply), 8, &Now);
    Check(Read == 8 && Reply[0] == 0x82 && Reply[1] == 6 && memcmp(Reply + 2, "world!", 6) == 0);
    Check(Gateway.ToBrowser == 1);

    // NOTE(zoubir): nothing either way for longer than the idle limit
    Check(Gateway.OpenLinks == 1);
    Now += GATEWAY_IDLE_SECONDS + 1.0;
    GatewayPoll(&Gateway, 0, Now);
    Check(Gateway.OpenLinks == 0);

    TcpClose(Browser);
    NetCloseSocket(&Server);
    GatewayStop(&Gateway);
}

internal void
TestGatewayRefusesOtherServers()
{
    static web_gateway Gateway;
    Check(GatewayStart(&Gateway, 0, true));
    Check(GatewayAllow(&Gateway, "127.0.0.1:27015"));
    Check(!GatewayAllow(&Gateway, "not a server"));
    Check(!GatewayAllow(&Gateway, "10.0.0.2:27015=nowhere"));
    double Now = 0.0;
    u64 Browser = TcpConnectLoopback(Gateway.Port);
    const char *Request = "GET /net/10.0.0.1:27015 HTTP/1.1\r\nUpgrade: websocket\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
    Check(TcpWrite(Browser, (const u8 *)Request, (u32)strlen(Request)) > 0);
    u8 Reply[256];
    TestGatewayRead(&Gateway, Browser, Reply, sizeof(Reply), 12, &Now);
    Check(strncmp((char *)Reply, "HTTP/1.1 403", 12) == 0);
    TcpClose(Browser);

    // NOTE(zoubir): no WebSocket asked for: the answer deploy/install.sh
    // checks for
    Browser = TcpConnectLoopback(Gateway.Port);
    Request = "GET /net/ HTTP/1.1\r\nHost: game-demo-gateway\r\n\r\n";
    Check(TcpWrite(Browser, (const u8 *)Request, (u32)strlen(Request)) > 0);
    TestGatewayRead(&Gateway, Browser, Reply, sizeof(Reply), 12, &Now);
    Check(strncmp((char *)Reply, "HTTP/1.1 426", 12) == 0);
    TcpClose(Browser);
    GatewayStop(&Gateway);
}

internal void
TestWebGateway()
{
    TestWebSocketAcceptKey();
    TestWebSocketFrames();
    TestGatewayRelaysPackets();
    TestGatewayRefusesOtherServers();
}
