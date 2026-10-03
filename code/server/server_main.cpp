/* Entry point of the dedicated server. Usage: server [port]
   Runs ServerTick SERVER_TICK_RATE times a second until Ctrl+C or a
   service stop, then tells every player it is closing and exits 0.
   Build with build_server.bat (Windows) or build_server.sh (Linux). */

#include <stdio.h>
#include <stdlib.h>
#include "server.cpp"
#include "platform/clock.cpp"
#include "platform/stop_signal.cpp"

int
main(int ArgCount, char **Args)
{
    u16 Port = SERVER_DEFAULT_PORT;
    if (ArgCount > 1)
    {
        int Parsed = atoi(Args[1]);
        if (Parsed <= 0 || Parsed > 65535)
        {
            fprintf(stderr, "usage: server [port]\n");
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
    if (!ServerStart(&Server, Port))
    {
        fprintf(stderr, "could not open UDP port %u\n", Port);
        return 1;
    }
    Server.Logging = true;
    StopSignalInstall();
    printf("server listening on UDP port %u at %d ticks/s\n", Port, SERVER_TICK_RATE);
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
