/* The web gateway program: lets browser players reach game servers that
   speak only UDP (server/web_gateway/web_gateway.h says how).
   Usage: web_gateway [--port N] [--allow host:port]... [--files folder]
                      [--local]
     --port    the TCP port browsers connect to (default 27080)
     --allow   a game server browsers may reach; repeat for more. Without
               one, 127.0.0.1:27015 (a server on this machine).
               host:port=host2:port2 lets browsers ask for the first and
               sends to the second (deploy/install.sh: the public address
               browsers know, and the host as seen from the container)
     --files   also serve this folder over HTTP, to try the browser game
               on one machine: web_gateway --files web\build
     --local   accept connections from this machine only
   Every minute it prints how many browsers are connected and how many
   packets went each way. Ctrl+C or SIGTERM stops it. Built by
   build_server.sh / .bat. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../net/protocol.cpp"
#include "../net/socket.cpp"
#include "platform/clock.cpp"
#include "platform/stop_signal.cpp"
#include "web_gateway/web_gateway.cpp"

int
main(int ArgCount, char **Args)
{
    setvbuf(stdout, 0, _IONBF, 0);
    static web_gateway Gateway;
    u16 Port = GATEWAY_DEFAULT_PORT;
    bool32 Loopback = false;
    const char *Files = 0;
    const char *Allowed[GATEWAY_MAX_ALLOWED];
    u32 AllowedCount = 0;
    for (int Arg = 1; Arg < ArgCount; Arg++)
    {
        bool32 HasValue = Arg + 1 < ArgCount;
        if (strcmp(Args[Arg], "--port") == 0 && HasValue) Port = (u16)atoi(Args[++Arg]);
        else if (strcmp(Args[Arg], "--files") == 0 && HasValue) Files = Args[++Arg];
        else if (strcmp(Args[Arg], "--local") == 0) Loopback = true;
        else if (strcmp(Args[Arg], "--allow") == 0 && HasValue && AllowedCount < GATEWAY_MAX_ALLOWED)
            Allowed[AllowedCount++] = Args[++Arg];
        else
        {
            fprintf(stderr, "usage: web_gateway [--port N] [--allow host:port]... [--files folder] [--local]\n");
            return 1;
        }
    }
    if (!AllowedCount) Allowed[AllowedCount++] = "127.0.0.1:27015";

    if (!GatewayStart(&Gateway, Port, Loopback))
    {
        fprintf(stderr, "web_gateway: cannot listen on TCP port %u\n", Port);
        return 1;
    }
    for (u32 Index = 0; Index < AllowedCount; Index++)
    {
        if (!GatewayAllow(&Gateway, Allowed[Index]))
        {
            fprintf(stderr, "web_gateway: cannot resolve %s\n", Allowed[Index]);
            return 1;
        }
        printf("web_gateway: browsers may reach %s\n", Allowed[Index]);
    }
    if (Files)
    {
        GatewayServeFiles(&Gateway, Files);
        printf("web_gateway: serving %s at http://127.0.0.1:%u/\n", Files, Gateway.Port);
    }
    printf("web_gateway: listening on TCP port %u\n", Gateway.Port);

    StopSignalInstall();
    double NextReport = ClockSeconds() + 60.0;
    while (!GlobalStopRequested)
    {
        GatewayPoll(&Gateway, 250, ClockSeconds());
        if (ClockSeconds() >= NextReport)
        {
            NextReport += 60.0;
            printf("web_gateway: %u connected, %llu packets to servers, %llu to browsers\n",
                   Gateway.OpenLinks, (unsigned long long)Gateway.ToServer,
                   (unsigned long long)Gateway.ToBrowser);
        }
    }
    GatewayStop(&Gateway);
    NetSocketsShutdown();
    return 0;
}
