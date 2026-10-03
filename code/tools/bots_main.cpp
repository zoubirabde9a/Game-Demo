/* Load test: fills a server with bot players that hold random buttons,
   like the soak test but over a real network.
   Usage: bots address:port content-id [count] [seconds]   (8 bots, 60 s)
   content-id is the hex value the server prints when it starts.
   At the end it prints, per bot, whether it stayed connected, how many
   snapshots arrived per second, and the newest snapshot's size, then
   leaves cleanly. Built by build_server.sh / .bat. */

#include <stdio.h>
#include <stdlib.h>
#include "../net/protocol.cpp"
#include "../net/socket.cpp"
#include "../net/client.cpp"
#include "../server/platform/clock.cpp"

#define MAX_BOTS 16

struct bot
{
    net_client Client;
    u16 Held;
    u32 Snapshots;
    u32 LastTick;
};

internal u32
BotRandom(u32 *State)
{
    u32 X = *State;
    X ^= X << 13; X ^= X >> 17; X ^= X << 5;
    *State = X;
    return X;
}

// Change direction and attacks now and then, like a real player.
internal u16
BotButtons(u32 *Random, u16 Held)
{
    if (BotRandom(Random) % 30) return Held;
    static const u16 Moves[] = {0, NetButton_Left, NetButton_Right, NetButton_Up, NetButton_Down,
                                NetButton_Left | NetButton_Up, NetButton_Right | NetButton_Down};
    u16 Result = Moves[BotRandom(Random) % ArrayCount(Moves)];
    if (BotRandom(Random) % 3 == 0) Result |= NetButton_Sword;
    if (BotRandom(Random) % 3 == 0) Result |= NetButton_Fireball;
    if (BotRandom(Random) % 6 == 0) Result |= NetButton_Jump;
    if (BotRandom(Random) % 8 == 0) Result |= NetButton_Dash;
    if (BotRandom(Random) % 12 == 0) Result |= NetButton_Shockwave;
    return Result;
}

int
main(int ArgCount, char **Args)
{
    net_address Server;
    if (ArgCount < 3 || !NetParseAddress(Args[1], &Server))
    {
        fprintf(stderr, "usage: bots a.b.c.d:port content-id-hex [count] [seconds]\n");
        return 1;
    }
    u32 ContentId = (u32)strtoul(Args[2], 0, 16);
    u32 Count = ArgCount > 3 ? (u32)atoi(Args[3]) : 8;
    double Seconds = ArgCount > 4 ? atof(Args[4]) : 60.0;
    if (Count < 1 || Count > MAX_BOTS) Count = 8;
    if (!NetSocketsStartup()) return 1;

    static bot Bots[MAX_BOTS];
    u32 Random = (u32)(ClockSeconds() * 1000.0) | 1;
    for (u32 Index = 0; Index < Count; ++Index)
    {
        if (!NetClientConnect(&Bots[Index].Client, Server, BotRandom(&Random), ContentId))
        {
            fprintf(stderr, "could not open a socket\n");
            return 1;
        }
    }

    double Start = ClockSeconds();
    double Next = Start;
    while (ClockSeconds() - Start < Seconds)
    {
        for (u32 Index = 0; Index < Count; ++Index)
        {
            bot *Bot = &Bots[Index];
            Bot->Held = BotButtons(&Random, Bot->Held);
            NetClientUpdate(&Bot->Client, 1.0f / 60.0f, Bot->Held, 0, 0);
            if (Bot->Client.HasSnapshot && Bot->Client.Snapshot.Tick != Bot->LastTick)
            {
                Bot->LastTick = Bot->Client.Snapshot.Tick;
                Bot->Snapshots++;
            }
        }
        Next += 1.0 / 60.0;
        double Now = ClockSeconds();
        if (Next > Now) ClockSleep(Next - Now);
    }

    static const char *Ends[] = {"connected", "no answer", "server full", "server closed",
                                 "lost connection", "left", "wrong version"};
    u32 Connected = 0;
    for (u32 Index = 0; Index < Count; ++Index)
    {
        bot *Bot = &Bots[Index];
        bool32 Up = Bot->Client.State == NetClient_Connected;
        Connected += Up ? 1 : 0;
        u32 Reason = (u32)Bot->Client.EndReason;
        printf("bot %u: %s, slot %u, %.1f snapshots/s, %u entities in the last one\n", Index,
               Up ? "connected" : (Reason < ArrayCount(Ends) ? Ends[Reason] : "disconnected"),
               Bot->Client.PlayerIndex, Bot->Snapshots / Seconds, Bot->Client.Snapshot.Count);
        NetClientDisconnect(&Bot->Client);
    }
    printf("%u of %u bots stayed connected for %.0f s\n", Connected, Count, Seconds);
    NetSocketsShutdown();
    return Connected == Count ? 0 : 2;
}
