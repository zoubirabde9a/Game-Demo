/* Checks that a game server answers. Usage: probe [address:port]
   Connects like a real client, waits for the first snapshot, then says
   goodbye. Exit codes: 0 joined and got a snapshot, 1 bad usage or no
   network, 2 no answer, 3 server full, 4 connection lost.
   The deploy script runs it after every install, and it works as a
   health check from anywhere. Built by build_server.sh / .bat. */

#include <stdio.h>
#include "../net/protocol.cpp"
#include "../net/socket.cpp"
#include "../net/client.cpp"
#include "platform/clock.cpp"

int
main(int ArgCount, char **Args)
{
    const char *Text = ArgCount > 1 ? Args[1] : "127.0.0.1:27015";
    net_address Server;
    if (!NetParseAddress(Text, &Server))
    {
        fprintf(stderr, "usage: probe [a.b.c.d:port]\n");
        return 1;
    }
    if (!NetSocketsStartup())
    {
        fprintf(stderr, "could not start networking\n");
        return 1;
    }

    static net_client Client;
    u32 Salt = (u32)(ClockSeconds() * 1000.0) ^ 0x9e3779b9u;
    if (!NetClientConnect(&Client, Server, Salt, 0))
    {
        fprintf(stderr, "could not open a socket\n");
        return 1;
    }

    // Same rate as the game, until a snapshot arrives or the client gives up.
    double Start = ClockSeconds();
    while (Client.State != NetClient_Disconnected && !Client.HasSnapshot)
    {
        NetClientUpdate(&Client, 1.0f / 60.0f, 0, 0, 0);
        ClockSleep(1.0 / 60.0);
    }

    int Result = 0;
    if (Client.HasSnapshot)
    {
        printf("ok: %s answered in %.0f ms, slot %u, %u entities in view\n", Text,
               (ClockSeconds() - Start) * 1000.0, Client.PlayerIndex, Client.Snapshot.Count);
        NetClientDisconnect(&Client);
    }
    else
    {
        switch (Client.EndReason)
        {
            case NetEnd_ServerFull: printf("full: %s refused the probe\n", Text); Result = 3; break;
            case NetEnd_NoAnswer: printf("down: no answer from %s\n", Text); Result = 2; break;
            default: printf("lost: connection to %s dropped\n", Text); Result = 4; break;
        }
    }
    NetSocketsShutdown();
    return Result;
}
