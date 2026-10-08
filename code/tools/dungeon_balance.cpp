/* Dungeon balance probe: three server bots (server/bots.cpp), a tank, a
   healer and a damage player, walk the dungeon with nobody else, the
   Sunken Crypt and then the Ember Depths (sim/dungeon/levels.cpp), and
   every fight is timed. Prints one line per fight: the room, how long
   it lasted, how it ended (cleared or wiped), the party's deaths, and for
   a boss whether its enrage timer ran out (sim/dungeon/boss_clock.cpp).
   Bots play worse than a party of people, so their times are an upper
   bound to tune timers against, not the target.

   Usage: dungeon_balance [minutes] [players] [room] [seeds]
   (default 20, 3, 2, 1). With seeds N past 1 the whole probe runs N
   times, each with the bots' choices and the run's own randomness (pack
   spots, elite affixes) seeded differently, every line led by its seed:
   one run is too noisy to tune by. PROBE_DEATHS=1 also prints, for each
   player who falls, the monsters within 260 of them.
   A room past 2 marks the rooms before it cleared, puts the bots inside
   it once they have joined, and gives each the experience of
   PROBE_SECONDS_PER_ROOM of play per room skipped, to time one fight
   over and over at about the level a party reaches it. Bots that wiped
   there are walked back in after PROBE_RETRY_SECONDS, as they only chase
   what they see and would wait at the checkpoint for good.
   PROBE_MAP=depths starts in the Ember Depths instead, the bots given
   the experience of the whole crypt first (and of the rooms skipped).
   Build: cl -nologo -O2 -DAPP_DEV=1 -DAPP_SLOW=0 -DAPP_WIN32=1
          ..\code\tools\dungeon_balance.cpp /link user32.lib Gdi32.lib Winmm.lib OpenGL32.lib */

#include <stdio.h>
#include <stdlib.h>
#include "../server/server.cpp"

// NOTE(zoubir): about how long a party takes over a room, walking and
// resting included
#define PROBE_SECONDS_PER_ROOM 90
#define PROBE_RETRY_SECONDS 12.f

// NOTE(zoubir): every bot with a body to Position, rested: full health
// and no burning, as a party waits before pulling again. Waiting by the
// Ashfall Bridge the bots walk into its lava, and came back into the
// Throne of Embers at a tenth to two thirds of their health, dying in a
// second: wipes the boss had no part in
internal void
PlaceBots(server_game *Game, v3 Position)
{
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        player_slot *Player = &Game->AppState->Players[Slot];
        world_entity *Entity = Player->Entity;
        if (Player->Active && Entity && !IsDeadPlayer(Entity))
        {
            MovePlayerTo(Game->AppState, &Game->AppState->World, Game->Arena,
                         Entity, Position);
            Entity->Hp = Entity->MaxHp;
            ZeroArray(Entity->StatusTimers, StatusEffect_Count, float);
        }
    }
}

// NOTE(zoubir): one probe, its randomness seeded by SeedNumber (0 leaves
// the game's own seeds)
internal void
ProbeOneSeed(u32 Minutes, u32 Players, u32 FirstRoom, u32 SeedNumber)
{
    bool32 Placed = false;
    static server_game Game;
    Game = {};
#pragma warning(push)
#pragma warning(disable: 4996)
    char *MapName = getenv("PROBE_MAP");
#pragma warning(pop)
    u32 StartMap = (MapName && strcmp(MapName, "depths") == 0) ? MapId_Depths : MapId_Crypt;
    // NOTE(zoubir): a party reaching the depths has played the crypt
    u32 RoomsBefore = StartMap == MapId_Depths ? CountRooms(MapId_Crypt) : 0;
    GameInit(&Game, StartMap);
    dungeon_run *SeededRun = 0;
    u32 SeededBots = 0;
    u32 SlotDeaths[MAX_PLAYERS] = {};
#pragma warning(push)
#pragma warning(disable: 4996)
    bool32 ShowDeaths = getenv("PROBE_DEATHS") != 0;
#pragma warning(pop)
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
            Game.AppState->World.MapId == StartMap &&
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
            // NOTE(zoubir): the rooms skipped count as cleared, or the
            // level cap (LevelCap, experience.cpp) would hold the bots back
            // NOTE(zoubir): every room but each level's empty first one
            // has monsters
            Game.AppState->DungeonRoomsCleared =
                Maximum(Game.AppState->DungeonRoomsCleared,
                        (RoomsBefore ? RoomsBefore - 1 : 0) +
                        (FirstRoom > 2 ? FirstRoom - 2 : 0));
            for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
            {
                player_slot *Player = &Game.AppState->Players[Slot];
                if (Player->Active && Player->Entity)
                {
                    if (FirstRoom > 2)
                    {
                        MovePlayerTo(Game.AppState, &Game.AppState->World, Game.Arena,
                                     Player->Entity, Run->RoomEntry[FirstRoom]);
                    }
                    AwardXp(Game.AppState, Player, (RoomsBefore + FirstRoom - 2) *
                            PROBE_SECONDS_PER_ROOM * XP_PER_SECOND);
                }
            }
            continue;
        }
        u32 Deaths = 0;
        for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
        {
            player_slot *Player = &Game.AppState->Players[Slot];
            Deaths += Player->Deaths;
            // NOTE(zoubir): with PROBE_DEATHS set, who stood round each
            // player as they fell, to find what kills a party
            if (ShowDeaths && Player->Deaths != SlotDeaths[Slot] && Player->Entity)
            {
                world *World = &Game.AppState->World;
                printf("    %s (slot %u) fell %.1f s in, at %.0f%% of the room's foes alive:",
                       GetRoleDef(Player->Role)->Title, Slot, Seconds,
                       100.f * (float)Run->ShownFoesLeft / (float)Maximum(1u, Run->FoeCount));
                for (u32 Index = 0; Index < World->EntityCount; ++Index)
                {
                    world_entity *Foe = &World->Entities[Index];
                    float Gap = Length(Foe->Position.XY - Player->Entity->Position.XY);
                    if (Foe->IsPresent && Foe->Type == EntityType_Monster && Foe->Hp > 0.f &&
                        Gap < 260.f)
                    {
                        printf(" %s%s%s %.0f", GetAffix(Foe->EliteAffix)->Name,
                               Foe->EliteAffix ? " " : "",
                               GetMonsterDef((monster_kind)Foe->MonsterKind)->Name, Gap);
                    }
                }
                printf("\n");
            }
            SlotDeaths[Slot] = Player->Deaths;
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
                       GetRoomName(Game.AppState->World.MapId, Room), Seconds,
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
            else if (Seconds > 60.f)
            {
                // NOTE(zoubir): a long walk between fights: lost bots, or
                // a room that will not start
                printf("  run %u  %.1f s between fights before the %s\n", Runs + 1,
                       Seconds, GetRoomName(Game.AppState->World.MapId, Run->FightingRoom));
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
               GetRoomName(Game.AppState->World.MapId, Room), Seconds, Run->ShownFoesLeft);
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
    else if (Run)
    {
        // NOTE(zoubir): no fight going: where the party stands, to tell
        // a party that cannot reach the next room from one that will not
        u32 Next = NextRoomToClear(Run->RoomStates, Run->RoomCount);
        printf("  between fights after %.1f s, the %s waits\n", Seconds,
               GetRoomName(Game.AppState->World.MapId, Next));
        world *World = &Game.AppState->World;
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
