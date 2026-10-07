/* Dungeon balance probe: three server bots (server/bots.cpp), a tank, a
   healer and a damage player, walk the Sunken Crypt with nobody else,
   and every fight is timed. Prints one line per fight: the room, how long
   it lasted, how it ended (cleared or wiped), the party's deaths, and for
   a boss whether its enrage timer ran out (sim/dungeon/boss_clock.cpp).
   Bots play worse than a party of people, so their times are an upper
   bound to tune timers against, not the target.

   Usage: dungeon_balance [minutes] [players] [room] [seeds]
   (default 20, 3, 2, 1). With seeds N past 1 the whole probe runs N
   times, each with the bots' choices and the run's own randomness (pack
   spots, elite affixes) seeded differently, every line led by its seed:
   one run is too noisy to tune by.
   A room past 2 marks the rooms before it cleared, puts the bots inside
   it once they have joined, and gives each the experience of
   PROBE_SECONDS_PER_ROOM of play per room skipped, to time one fight
   over and over at about the level a party reaches it. Bots that wiped
   there are walked back in after PROBE_RETRY_SECONDS, as they only chase
   what they see and would wait at the checkpoint for good.
   Build: cl -nologo -O2 -DAPP_DEV=1 -DAPP_SLOW=0 -DAPP_WIN32=1
          ..\code\tools\dungeon_balance.cpp /link user32.lib Gdi32.lib Winmm.lib OpenGL32.lib */

#include <stdio.h>
#include <stdlib.h>
#include "../server/server.cpp"

// NOTE(zoubir): about how long a party takes over a room, walking and
// resting included
#define PROBE_SECONDS_PER_ROOM 90
#define PROBE_RETRY_SECONDS 12.f

// NOTE(zoubir): every bot with a body to Position
internal void
PlaceBots(server_game *Game, v3 Position)
{
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        player_slot *Player = &Game->AppState->Players[Slot];
        if (Player->Active && Player->Entity && !IsDeadPlayer(Player->Entity))
        {
            MovePlayerTo(Game->AppState, &Game->AppState->World, Game->Arena,
                         Player->Entity, Position);
        }
    }
}

// NOTE(zoubir): one probe, its randomness seeded by SeedNumber (0 leaves
// the game's own seeds)
internal void
ProbeOneSeed(u32 Minutes, u32 Players, u32 FirstRoom, u32 SeedNumber)
{
    bool32 Placed = FirstRoom <= 2;
    static server_game Game;
    Game = {};
    GameInit(&Game, MapId_Crypt);
    dungeon_run *SeededRun = 0;
    u32 SeededBots = 0;
    Game.BotTarget = Players;
    float Dt = 1.0f / SERVER_TICK_RATE;
    u32 Ticks = Minutes * 60 * SERVER_TICK_RATE;
    u32 Room = 0;
    u32 WipesAtStart = 0;
    u32 DeathsAtStart = 0;
    u32 Runs = 0;
    bool32 Enraged = false;
    float Seconds = 0.f;
    float BossShare = 0.f;
    float EnrageShare = 0.f;
    printf("dungeon balance: %u bots, %u simulated minutes\n", Players, Minutes);
    for (u32 Tick = 0; Tick < Ticks; ++Tick)
    {
        GameKeepBots(&Game, 0, Dt);
        GameTick(&Game, Dt);
        dungeon_run *Run = Game.AppState->Dungeon;
        if (!Run)
        {
            continue;
        }
        if (SeedNumber && Run != SeededRun)
        {
            SeededRun = Run;
            Run->Series = Seed(SeedNumber * 7919 + 101);
        }
        for (u32 Slot = 0; Slot < MAX_PLAYERS && SeedNumber; ++Slot)
        {
            if (Game.Bots[Slot].Active && !(SeededBots & (1u << Slot)))
            {
                SeededBots |= 1u << Slot;
                Game.Bots[Slot].Random ^= 2654435761u * SeedNumber;
                Game.Bots[Slot].Random |= 1;
            }
        }
        if (Placed && FirstRoom > 2 && !Run->FightingRoom &&
            Run->RoomStates[FirstRoom] != RoomState_Cleared && Seconds > PROBE_RETRY_SECONDS)
        {
            PlaceBots(&Game, Run->RoomEntry[FirstRoom]);
        }
        if (!Placed && Tick > SERVER_TICK_RATE)
        {
            // NOTE(zoubir): every run built later starts from the
            // Antechamber again, so this is the first run only
            Placed = true;
            for (u32 Before = 1; Before < FirstRoom && Before <= Run->RoomCount; ++Before)
            {
                Run->RoomStates[Before] = RoomState_Cleared;
            }
            for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
            {
                player_slot *Player = &Game.AppState->Players[Slot];
                if (Player->Active && Player->Entity)
                {
                    MovePlayerTo(Game.AppState, &Game.AppState->World, Game.Arena,
                                 Player->Entity, Run->RoomEntry[FirstRoom]);
                    AwardXp(Game.AppState, Player,
                            (FirstRoom - 2) * PROBE_SECONDS_PER_ROOM * XP_PER_SECOND);
                }
            }
            continue;
        }
        u32 Deaths = 0;
        for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
        {
            Deaths += Game.AppState->Players[Slot].Deaths;
        }
        world_entity *Boss = FightBoss(&Game.AppState->World, Run);
        if (Boss && Boss->MaxHp > 0.f)
        {
            BossShare = Boss->Hp / Boss->MaxHp;
        }
        if (Room && !Enraged && Run->Clock.Stage == BossClock_Enraged)
        {
            Enraged = true;
            EnrageShare = BossShare;
        }
        if (Run->FightingRoom != Room)
        {
            if (Room)
            {
                bool32 Wiped = Run->Wipes != WipesAtStart;
                printf("  run %u  %-18s %6.1f s  %s  deaths %u", Runs + 1,
                       GetRoomName(MapId_Crypt, Room), Seconds,
                       Wiped ? "WIPED  " : "cleared", Deaths - DeathsAtStart);
                if (Enraged)
                {
                    printf("  enraged with the boss at %.0f%%", 100.f * EnrageShare);
                }
                if (Wiped && BossShare > 0.f)
                {
                    printf("  wiped with the boss at %.0f%%", 100.f * BossShare);
                }
                printf("\n");
                if (!Wiped && Room == Run->RoomCount)
                {
                    Runs++;
                }
            }
            Room = Run->FightingRoom;
            WipesAtStart = Run->Wipes;
            DeathsAtStart = Deaths;
            Enraged = false;
            Seconds = 0.f;
            BossShare = 0.f;
        }
        Seconds += Dt;
    }
    dungeon_run *Run = Game.AppState->Dungeon;
    if (Run && Room)
    {
        printf("  still in the %s after %.1f s, %u foes left\n",
               GetRoomName(MapId_Crypt, Room), Seconds, Run->ShownFoesLeft);
        // NOTE(zoubir): where the foes the party never finished are, to
        // tell a stuck monster from a party that cannot win
        world *World = &Game.AppState->World;
        for (u32 Index = 0; Index < World->EntityCount; ++Index)
        {
            world_entity *Foe = &World->Entities[Index];
            if (Foe->IsPresent && Foe->Type == EntityType_Monster && Foe->Hp > 0.f)
            {
                printf("    %s at (%.0f, %.0f), room %u, %.0f of %.0f health\n",
                       GetMonsterDef((monster_kind)Foe->MonsterKind)->Name,
                       Foe->Position.X, Foe->Position.Y,
                       RoomAtPosition(World, Foe->Position.XY), Foe->Hp, Foe->MaxHp);
            }
        }
        for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
        {
            world_entity *Player = Game.AppState->Players[Slot].Entity;
            if (Player && Game.AppState->Players[Slot].Active)
            {
                printf("    player %u at (%.0f, %.0f), room %u%s\n", Slot, Player->Position.X,
                       Player->Position.Y, RoomAtPosition(World, Player->Position.XY),
                       IsDeadPlayer(Player) ? ", down" : "");
            }
        }
    }
    printf("dungeon balance: %u runs cleared\n", Runs);
    GameShutdown(&Game);
}

int
main(int ArgCount, char **Args)
{
    u32 Minutes = ArgCount > 1 ? (u32)atoi(Args[1]) : 20;
    u32 Players = ArgCount > 2 ? (u32)atoi(Args[2]) : 3;
    u32 FirstRoom = ArgCount > 3 ? (u32)atoi(Args[3]) : 2;
    u32 Seeds = ArgCount > 4 ? (u32)atoi(Args[4]) : 1;
    for (u32 SeedNumber = (Seeds > 1 ? 1 : 0); SeedNumber <= (Seeds > 1 ? Seeds : 0); ++SeedNumber)
    {
        printf("seed %u\n", SeedNumber);
        ProbeOneSeed(Minutes, Players, FirstRoom, SeedNumber);
    }
    return 0;
}
