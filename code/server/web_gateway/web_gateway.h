#if !defined(WEB_GATEWAY_H)
#define WEB_GATEWAY_H
/* Web gateway: lets the browser game (web/) play on the dedicated server,
   which speaks only UDP. A browser opens a WebSocket to
   /net/<server>:<port>; the gateway opens one UDP socket for it, sends
   every binary message on to that server as one datagram, and sends each
   datagram the server answers back as one message. The server sees an
   ordinary client at the gateway's address, so nothing in it changes.
   Only servers on the allowed list (GatewayAllow) can be reached, so the
   gateway is no open relay. A WebSocket idle for GATEWAY_IDLE_SECONDS, or
   one sending more than GATEWAY_MAX_MESSAGES a second, is closed; when
   the browser falls behind, server packets are dropped, as UDP would.

   For trying the browser game on one machine it can also serve a folder
   of files (GatewayServeFiles, files.cpp): build\web_gateway.exe --files
   web\build, then open http://127.0.0.1:27080/. On the live server Caddy
   serves the page and passes /net/ on to the gateway (deploy/README.md).

   GatewayStart, GatewayAllow, then GatewayPoll in a loop, which waits up
   to the timeout for something to do. web_gateway_main.cpp is the
   program; tests/web_gateway_tests.cpp drives it in one process. */

#define GATEWAY_DEFAULT_PORT 27080
#define GATEWAY_MAX_LINKS 128
#define GATEWAY_MAX_ALLOWED 8
#define GATEWAY_IN_SIZE 16384
#define GATEWAY_OUT_LIMIT (256 * 1024) // queued for one browser before packets drop
#define GATEWAY_IDLE_SECONDS 30.0
#define GATEWAY_MAX_MESSAGES 200       // a second, from one browser

enum gateway_link_state
{
    GatewayLink_Free,
    GatewayLink_Http,      // reading the opening request
    GatewayLink_WebSocket,
    GatewayLink_Closing,   // sending what is queued, then closing
};

struct gateway_link
{
    gateway_link_state State;
    u64 Tcp;
    net_socket Udp;
    net_address Server;
    double LastHeard;      // from the browser or the server
    double SecondStart;
    u32 MessagesThisSecond;
    u32 InUsed;
    u8 In[GATEWAY_IN_SIZE];
    u8 *Out;               // malloc'd; [OutSent, OutUsed) is still to send
    u32 OutUsed;
    u32 OutSent;
    u32 OutCapacity;
};

struct web_gateway
{
    u64 Listener;
    u16 Port;
    u32 AllowedCount;
    net_address Allowed[GATEWAY_MAX_ALLOWED];
    char Files[256];       // a folder to serve over HTTP, "" for none
    double Now;
    u32 OpenLinks;
    u64 ToServer;          // packets passed on since start
    u64 ToBrowser;
    gateway_link Links[GATEWAY_MAX_LINKS];
};

// Listens on Port (0: any free one, read back from Gateway->Port).
// Loopback true accepts connections from this machine only.
internal bool32 GatewayStart(web_gateway *Gateway, u16 Port, bool32 Loopback);

// Allows "host:port" or "a.b.c.d:port"; a host name is looked up now.
internal bool32 GatewayAllow(web_gateway *Gateway, const char *Server);

// Serves the files in Folder to plain HTTP requests.
internal void GatewayServeFiles(web_gateway *Gateway, const char *Folder);

// Waits up to TimeoutMs for the sockets, then handles all that is ready.
// Now is seconds on any steady clock.
internal void GatewayPoll(web_gateway *Gateway, int TimeoutMs, double Now);

internal void GatewayStop(web_gateway *Gateway);

#endif
