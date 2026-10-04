/* Entry point of the dedicated server.
   Usage: server [port] [--map <name>] [--bots <count>] [--name <server name>]
   Runs ServerTick SERVER_TICK_RATE times a second until Ctrl+C or a
   service stop, then tells every player it is closing and exits 0. The
   map is any map's name or its last word ("keep", "wilds", "ashen
   wastes"); players who join get it in the connect reply. --bots keeps
   that many bot players in the slots no human uses (server/bots.cpp).
   --name is what players see in the server list and HUD, up to 23
   characters; without it clients use the name in their own server list.
   Build with build_server.bat (Windows) or build_server.sh (Linux). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.cpp"
#include "platform/clock.cpp"
#include "platform/stop_signal.cpp"

int
main(int ArgCount, char **Args)
{
    u16 Port = SERVER_DEFAULT_PORT;
    u32 MapId = MapId_Arena;
    u32 Bots = 0;
    const char *Name = 0;
    for (int Arg = 1; Arg < ArgCount; ++Arg)
    {
        if (strcmp(Args[Arg], "--map") == 0 && Arg + 1 < ArgCount)
        {
            map_id Found = FindMapByName(Args[++Arg], MapId_Count);
            if (Found == MapId_Count)
            {
                fprintf(stderr, "unknown map \"%s\"; maps:", Args[Arg]);
                for (u32 Index = 0; Index < MapId_Count; ++Index)
                {
                    fprintf(stderr, " \"%s\"", GetMapDef((map_id)Index)->Name);
                }
                fprintf(stderr, "\n");
                return 1;
            }
            MapId = Found;
            continue;
        }
        if (strcmp(Args[Arg], "--name") == 0 && Arg + 1 < ArgCount)
        {
            Name = Args[++Arg];
            continue;
        }
        if (strcmp(Args[Arg], "--bots") == 0 && Arg + 1 < ArgCount)
        {
            Bots = (u32)atoi(Args[++Arg]);
            if (Bots > MAX_PLAYERS) Bots = MAX_PLAYERS;
            continue;
        }
        int Parsed = atoi(Args[Arg]);
        if (Parsed <= 0 || Parsed > 65535)
        {
            fprintf(stderr, "usage: server [port] [--map <name>] [--bots <count>] [--name <server name>]\n");
            return 1;
        }
        Port = (u16)Parsed;
    }

    if (!NetSocketsStartup())
    {
        fprintf(stderr, "could not start networking\n");
        return 1;
    }

    static server Server;
    if (!ServerStart(&Server, Port, MapId, Name))
    {
        fprintf(stderr, "could not open UDP port %u\n", Port);
        return 1;
    }
    Server.Logging = true;
    Server.Game.BotTarget = Bots;
    StopSignalInstall();
    // The content id tells which game build this is; clients must match it.
    printf("server%s%s%s listening on UDP port %u at %d ticks/s, content id %08x, map %s\n",
           Server.Clients.ServerName[0] ? " \"" : "", Server.Clients.ServerName,
           Server.Clients.ServerName[0] ? "\"" : "", Port, SERVER_TICK_RATE,
           Server.Clients.ContentId, GetMapDef((map_id)MapId)->Name);
    fflush(stdout);

    double TickSeconds = 1.0 / SERVER_TICK_RATE;
    double NextTick = ClockSeconds();
    double LastStats = NextTick;
    while (!StopSignalReceived())
    {
        double TickStart = ClockSeconds();
        ServerTick(&Server);

        // Schedule from the ideal time, not from now, so ticks do not drift.
        // If the server fell more than a second behind, stop trying to catch up.
        NextTick += TickSeconds;
        double Now = ClockSeconds();
        ServerRecordTick(&Server, Now - TickStart, Now > NextTick);
        if (Now - NextTick > 1.0) NextTick = Now;

        if (Now - LastStats >= SERVER_STATS_SECONDS)
        {
            char Line[256];
            ServerFormatStats(&Server, Now - LastStats, Line, sizeof(Line));
            ServerLog(&Server, "%s", Line);
            LastStats = Now;
        }

        if (NextTick > Now) ClockSleep(NextTick - Now);
    }

    ServerLog(&Server, "shutting down, %u players disconnected", ServerPlayerCount(&Server));
    ServerStop(&Server);
    NetSocketsShutdown();
    return 0;
}
