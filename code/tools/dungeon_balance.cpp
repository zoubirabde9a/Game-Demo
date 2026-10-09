/* Dungeon balance probe: three server bots (server/bots.cpp), a tank, a
   healer and a damage player, walk the dungeon with nobody else, the
   Sunken Crypt and then the Ember Depths (sim/dungeon/levels.cpp), and
   every fight is timed. Prints one line per fight: the room, how long
   it lasted, how it ended (cleared or wiped), the party's deaths, and for
   a boss whether its enrage timer ran out (sim/dungeon/boss_clock.cpp).
   Bots play worse than a party of people, so their times are an upper
   bound to tune timers against, not the target.

   Usage: dungeon_balance [minutes] [players] [room] [seeds]
   (default 20, 3, 2, 1). Minutes is only a ceiling: a probe stops as
   soon as its work is done. With seeds N past 1 the probe runs N times,
   each with the bots' choices and the run's own randomness (pack spots,
   elite affixes) seeded differently, one process per seed, PROBE_JOBS
   at once (default: every core), and prints one table: per room, kills,
   wipes and deaths per kill, average and longest kill. One run is too
   noisy to tune by. PROBE_VERBOSE=1 also prints every fight of every
   seed. PROBE_DEATHS=1 also prints, for each player who falls, the
   monsters within 260 of them (one seed, or with PROBE_VERBOSE).
   A room past 2 marks the rooms before it cleared, puts the bots inside
   it once they have joined, at the level a party reaches it with (one
   per room with monsters cleared before it, as LevelCap allows), and
   stops when that room is cleared. Bots that wiped there are walked
   back in after PROBE_RETRY_SECONDS, as they only chase what they see
   and would wait at the checkpoint for good. From room 2 a probe stops
   after clearing PROBE_LEVELS levels (default 1); PROBE_LEVELS=3 from
   the crypt plays the crypt, the depths and the vault in a row.
   Bots that stand PROBE_STALL_SECONDS between fights (lost: they wander
   before the Throne of Dust for minutes) are walked to the next room's
   entrance and counted as a stall; PROBE_STALLS_PER_ROOM in a row at
   one room, or a fight past PROBE_FIGHT_SECONDS (a foe nobody can
   reach), ends the seed, reported as stuck.
   PROBE_MAP=depths starts in the Ember Depths instead, the bots given
   the experience of the whole crypt first (and of the rooms skipped);
   PROBE_MAP=vault in the Rimeheart Vault, with the crypt's and the
   depths'; PROBE_MAP=rift in the Aurora Rift, with all three before it;
   PROBE_MAP=starless in the Starless Deep, with all four.
   PROBE_TREE=class has the bots spend their points in the class tree
   only, PROBE_TREE=run in the second tree first (sim/dungeon/run_tree/);
   unset, they pick from both at random as a server's bots do.
   PROBE_TREE=core-class and core-run unlock the class's C and V spells
   first, then spend the rest in that one tree.
   Build and run: misc\balance.bat [same arguments], which rebuilds the
   probe only when the code changed. */

#include <stdio.h>
#include <stdlib.h>
#include "../server/server.cpp"

#define PROBE_RETRY_SECONDS 12.f
#define PROBE_STALL_SECONDS 120.f
#define PROBE_STALLS_PER_ROOM 3
#define PROBE_FIGHT_SECONDS 300.f
#define PROBE_MAX_JOBS 64
#define PROBE_MAX_ROOMS 64

#if defined(_WIN32)
#define ProbeOpen _popen
#define ProbeClose _pclose
#else
#define ProbeOpen popen
#define ProbeClose pclose
#endif

// NOTE(zoubir): a seed run by main in its own process, which prints a
// line per fight for the table besides the lines people read
global_variable bool32 ProbeChild;

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

// NOTE(zoubir): every bot's unspent talent points, spent now as the bot
// would pick them. A bot spends one every BOT_LEARN_SECONDS, so bots
// given a dozen levels and put straight into a boss room fought it with
// most of their talents unlearned, where a party that walked there had
// spent them long before: the probe ran far harder than the full run
internal void
SpendBotTalents(server_game *Game)
{
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
    {
        player_slot *Player = &Game->AppState->Players[Slot];
        if (!Player->Active || !Game->Bots[Slot].Active)
        {
            continue;
        }
        // NOTE(zoubir): PROBE_TREE=class spends only in the class tree,
        // =run in the second tree first; unset, the bot picks from both
        // (sim/dungeon/run_tree/)
#pragma warning(push)
#pragma warning(disable: 4996)
        char *Tree = getenv("PROBE_TREE");
#pragma warning(pop)
        u32 Wanted = (Tree && strcmp(Tree, "class") == 0) ? TalentBranch_Role :
            (Tree && strcmp(Tree, "run") == 0) ? TalentBranch_Run : TalentBranch_Count;
        // NOTE(zoubir): =core-class and =core-run unlock the class's C and
        // V spells first, as a player would, then spend the rest in one
        // tree, so the two trees' points compare
        bool32 Core = Tree && strncmp(Tree, "core-", 5) == 0;
        if (Core)
        {
            Wanted = strcmp(Tree + 5, "run") == 0 ? TalentBranch_Run : TalentBranch_Role;
        }
        while (TalentPointsLeft(Player) > 0)
        {
            if (Core)
            {
                bool32 Spells = Player->Ranks[Talent_RoleFirst + ROLE_TALENT_C_SPELL] &&
                    Player->Ranks[Talent_RoleFirst + ROLE_TALENT_V_SPELL];
                Wanted = Spells ? (strcmp(Tree + 5, "run") == 0 ? TalentBranch_Run : TalentBranch_Role) :
                    TalentBranch_Role;
            }
            u32 Pick = BotPickTalent(&Game->Bots[Slot], Game->AppState, Player) >> NET_LEARN_SHIFT;
            for (u32 Try = 0; Try < 64 && Pick && Wanted != TalentBranch_Count &&
                 TalentDefs[Pick - 1].Branch != Wanted; ++Try)
            {
                Pick = BotPickTalent(&Game->Bots[Slot], Game->AppState, Player) >> NET_LEARN_SHIFT;
            }
            if (!Pick || !LearnTalent(Game->AppState, Slot, Pick - 1))
            {
                break;
            }
        }
    }
}

// NOTE(zoubir): one probe, its randomness seeded by SeedNumber (0 leaves
// the game's own seeds)
internal void
ProbeOneSeed(u32 Minutes, u32 Players, u32 FirstRoom, u32 SeedNumber)
{
#pragma warning(push)
#pragma warning(disable: 4996)
    char *LevelsText = getenv("PROBE_LEVELS");
#pragma warning(pop)
    u32 Levels = LevelsText ? (u32)atoi(LevelsText) : 1;
    bool32 Done = false;
    u32 Stalls = 0;
    bool32 Placed = false;
    static server_game Game;
    Game = {};
#pragma warning(push)
#pragma warning(disable: 4996)
    char *MapName = getenv("PROBE_MAP");
#pragma warning(pop)
    u32 StartMap = (MapName && strcmp(MapName, "depths") == 0) ? MapId_Depths :
        (MapName && strcmp(MapName, "vault") == 0) ? MapId_Vault :
        (MapName && strcmp(MapName, "rift") == 0) ? MapId_Rift :
        (MapName && strcmp(MapName, "starless") == 0) ? MapId_Starless : MapId_Crypt;
    // NOTE(zoubir): a party reaching a level has played every level
    // before it
    u32 RoomsBefore = 0;
    u32 LevelsBefore = 0;
    for(u32 MapId = MapId_Crypt; MapId != StartMap && LevelsBefore < ArrayCount(DungeonLevels);
        MapId = NextRunMap(MapId))
    {
        RoomsBefore += CountRooms(MapId);
        LevelsBefore++;
    }
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
    for (u32 Tick = 0; Tick < Ticks && !Done; ++Tick)
    {
        GameKeepBots(&Game, 0, Dt);
        if (Placed)
        {
            SpendBotTalents(&Game);
        }
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
                // NOTE(zoubir): and its second tree rolls apart per seed
                Game.AppState->Players[Slot].TreeSeed =
                    RunHash(SeedNumber * 40503u + Slot * 977u) & RUN_SEED_MASK;
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
                        (RoomsBefore - LevelsBefore) +
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
                    // NOTE(zoubir): the level a party arrives with, the
                    // cap: guessing it from time played left the bots a
                    // level short at the vault's last rooms
                    u32 Want = XpToReach(LevelCap(Game.AppState, Player));
                    if (Want > Player->Xp)
                    {
                        AwardXp(Game.AppState, Player, Want - Player->Xp);
                    }
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
                // NOTE(zoubir): the same fight for the table (main)
                if (ProbeChild)
                {
                    printf("fight\t%s\t%u\t%.1f\t%u\n",
                           GetRoomName(Game.AppState->World.MapId, Room), Wiped ? 0 : 1,
                           Seconds, Deaths - DeathsAtStart);
                }
                if (!Wiped && Room == Run->RoomCount)
                {
                    Runs++;
                }
                if (!Wiped && FirstRoom > 2 && Room == FirstRoom &&
                    Game.AppState->World.MapId == StartMap)
                {
                    Done = true;
                }
                if (FirstRoom <= 2 && Runs >= Levels)
                {
                    Done = true;
                }
            }
            else if (Seconds > 60.f)
            {
                // NOTE(zoubir): a long walk between fights: lost bots, or
                // a room that will not start
                printf("  run %u  %.1f s between fights before the %s\n", Runs + 1,
                       Seconds, GetRoomName(Game.AppState->World.MapId, Run->FightingRoom));
            }
            if (Run->FightingRoom)
            {
                Stalls = 0;
            }
            Room = Run->FightingRoom;
            WipesAtStart = Run->Wipes;
            DeathsAtStart = Deaths;
            Enraged = false;
            Seconds = 0.f;
            BossShare = 0.f;
        }
        Seconds += Dt;
        // NOTE(zoubir): lost bots are walked on; a room that will not
        // start, or a fight that will not end, ends the seed, as the rest
        // of it would only wait
        if (Placed && !Room && !Done && Seconds > PROBE_STALL_SECONDS)
        {
            u32 Next = NextRoomToClear(Run->RoomStates, Run->RoomCount);
            char *Name = GetRoomName(Game.AppState->World.MapId, Next);
            if (++Stalls >= PROBE_STALLS_PER_ROOM)
            {
                printf("%s%s\n", ProbeChild ? "stuck\t" : "  stuck before the ", Name);
                Done = true;
            }
            else
            {
                printf("%s%s\n", ProbeChild ? "stall\t" : "  lost, walked to the ", Name);
                PlaceBots(&Game, Run->RoomEntry[Next]);
                Seconds = 0.f;
            }
        }
        if (Room && !Done && Seconds > PROBE_FIGHT_SECONDS)
        {
            printf("%s%s\n", ProbeChild ? "stuck\t" : "  a fight that will not end in the ",
                   GetRoomName(Game.AppState->World.MapId, Room));
            Done = true;
        }
    }
    dungeon_run *Run = Game.AppState->Dungeon;
    if (Done)
    {
        // NOTE(zoubir): finished, nothing left to show
    }
    else if (Run && Room)
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

#include "dungeon_balance/seeds.cpp"

int
main(int ArgCount, char **Args)
{
    u32 Minutes = ArgCount > 1 ? (u32)atoi(Args[1]) : 20;
    u32 Players = ArgCount > 2 ? (u32)atoi(Args[2]) : 3;
    u32 FirstRoom = ArgCount > 3 ? (u32)atoi(Args[3]) : 2;
    u32 Seeds = ArgCount > 4 ? (u32)atoi(Args[4]) : 1;
    if (ArgCount > 5)
    {
        // NOTE(zoubir): one seed for ProbeSeeds
        ProbeChild = true;
        ProbeOneSeed(Minutes, Players, FirstRoom, (u32)atoi(Args[5]));
    }
    else if (Seeds > 1)
    {
        ProbeSeeds(Minutes, Players, FirstRoom, Seeds);
    }
    else
    {
        ProbeOneSeed(Minutes, Players, FirstRoom, 0);
    }
    return 0;
}
