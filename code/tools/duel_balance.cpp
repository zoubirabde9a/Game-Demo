/* Duel balance probe: server bots (server/bots.cpp, bots/duel_kit.cpp)
   fight rounds of the duel on the Old Arena with nobody else, and each
   spends its points by a build, so the talent tree
   (sim/progression/talents.cpp) can be tuned on numbers, not guesses.
   Every round prints who won, how long it took, and what each player
   had learned.

   Usage: duel_balance [rounds] [players] [seeds]
   (default 19, 2, 1): 19 rounds take a pair of players from level 1 to
   the duel's top level, 20. With seeds past 1 every seed runs in its own
   process, PROBE_JOBS at once (default: every core), and the probe prints
   one table: the share of rounds each build won, then each talent's share
   of rounds won by the players who had it against those who did not, and
   the rounds' average length. PROBE_VERBOSE=1 also prints every round of
   every seed.

   PROBE_BUILDS=fire,guard gives slot 0 the first build, slot 1 the
   second, and so on round the list. A build is a branch name (fire,
   motion, guard: every point in that branch while it takes one, then
   anywhere) or random (any talent it may take, as a server's bots do,
   the default). Each point goes in at random among those the build
   allows, so a seed is one way of playing the build.

   Build and run: misc\duel_balance.bat [same arguments]. */

#include <stdio.h>
#include <stdlib.h>
#include "../server/server.cpp"
// NOTE(zoubir): getenv everywhere here, read once
#pragma warning(disable: 4996)

#define PROBE_MAX_JOBS 64
#define PROBE_MAX_BUILDS 8
// NOTE(zoubir): a round longer than this ends the seed: bots that cannot
// find each other
#define PROBE_ROUND_SECONDS 240.f

#if defined(_WIN32)
#define ProbeOpen _popen
#define ProbeClose _pclose
#else
#define ProbeOpen popen
#define ProbeClose pclose
#endif

global_variable bool32 ProbeChild;

// NOTE(zoubir): the probe's builds; a branch to spend in, or
// TalentBranch_Count for anywhere
struct probe_builds
{
    u32 Count;
    u32 Branch[PROBE_MAX_BUILDS];
    char Name[PROBE_MAX_BUILDS][16];
};

internal probe_builds
ReadBuilds()
{
    probe_builds Result = {};
    char *Text = getenv("PROBE_BUILDS");
    char Copy[256];
    snprintf(Copy, sizeof(Copy), "%s", Text ? Text : "random");
    for (char *Word = strtok(Copy, ","); Word && Result.Count < PROBE_MAX_BUILDS;
         Word = strtok(0, ","))
    {
        u32 Branch = TalentBranch_Count;
        for (u32 Index = 0; Index < TALENT_GAME_BRANCHES; ++Index)
        {
            if (_stricmp(Word, TalentBranchNames[Index]) == 0)
            {
                Branch = Index;
            }
        }
        Result.Branch[Result.Count] = Branch;
        snprintf(Result.Name[Result.Count], sizeof(Result.Name[0]), "%s",
                 Branch < TalentBranch_Count ? TalentBranchNames[Branch] : "Random");
        Result.Count++;
    }
    if (!Result.Count)
    {
        Result.Count = 1;
        Result.Branch[0] = TalentBranch_Count;
        snprintf(Result.Name[0], sizeof(Result.Name[0]), "Random");
    }
    return Result;
}

inline u32
ProbeRandom(u32 *State)
{
    u32 X = *State;
    X ^= X << 13; X ^= X >> 17; X ^= X << 5;
    *State = X;
    return X;
}

// NOTE(zoubir): every point Slot has, spent now by its build: at random
// among the talents of the build's branch it may take, else among all
internal void
SpendProbePoints(app_state *AppState, u32 SlotIndex, u32 Branch, u32 *Random)
{
    player_slot *Slot = &AppState->Players[SlotIndex];
    while (TalentPointsLeft(Slot) > 0)
    {
        u32 Choices[Talent_Count];
        u32 ChoiceCount = 0;
        for (u32 Pass = 0; Pass < 2 && !ChoiceCount; ++Pass)
        {
            for (u32 Talent = 0; Talent < Talent_RoleFirst; ++Talent)
            {
                if ((Pass || Branch == TalentBranch_Count ||
                     TalentDefs[Talent].Branch == Branch) &&
                    CanLearnTalent(Slot, Talent) == TalentRefusal_None)
                {
                    Choices[ChoiceCount++] = Talent;
                }
            }
        }
        if (!ChoiceCount ||
            !LearnTalent(AppState, SlotIndex, Choices[ProbeRandom(Random) % ChoiceCount]))
        {
            break;
        }
    }
}

// NOTE(zoubir): one line per player of a round that just ended: its slot,
// build, whether it won, and every talent it had with its ranks
internal void
PrintRoundPlayers(app_state *AppState, probe_builds *Builds, u32 Winner)
{
    for (u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; ++SlotIndex)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active)
        {
            continue;
        }
        printf("player\t%u\t%s\t%u\t", SlotIndex, Builds->Name[SlotIndex % Builds->Count],
               SlotIndex == Winner ? 1 : 0);
        for (u32 Talent = 0; Talent < Talent_RoleFirst; ++Talent)
        {
            if (Slot->Ranks[Talent])
            {
                printf("%s:%u,", ShownTalentDef(Slot, Talent)->Name, Slot->Ranks[Talent]);
            }
        }
        printf("\n");
    }
}

internal void
ProbeOneSeed(u32 Rounds, u32 Players, u32 SeedNumber)
{
    probe_builds Builds = ReadBuilds();
    static server_game Game;
    Game = {};
    GameRules = DuelRules;
    GameInit(&Game, MapId_Arena);
    Game.BotTarget = Players;
    u32 Random = 2654435761u * (SeedNumber + 1) | 1;
    u32 SeededBots = 0;
    float Dt = 1.0f / SERVER_TICK_RATE;
    u32 Played = 0;
    float Seconds = 0.f;
    bool32 InBreak = false;
    if (!ProbeChild)
    {
        printf("duel balance: %u bots, %u rounds\n", Players, Rounds);
    }
    for (u32 Tick = 0; Played < Rounds; ++Tick)
    {
        GameKeepBots(&Game, 0, Dt);
        app_state *AppState = Game.AppState;
        for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
        {
            if (!Game.Bots[Slot].Active)
            {
                continue;
            }
            if (!(SeededBots & (1u << Slot)))
            {
                SeededBots |= 1u << Slot;
                Game.Bots[Slot].Random ^= 2654435761u * (SeedNumber + 1);
                Game.Bots[Slot].Random |= 1;
            }
            SpendProbePoints(AppState, Slot, Builds.Branch[Slot % Builds.Count], &Random);
        }
        GameTick(&Game, Dt);
        bool32 Break = AppState->RoundBreak > 0.f;
        Seconds += InBreak ? 0.f : Dt;
        if (Break && !InBreak)
        {
            u32 Winner = MAX_PLAYERS;
            for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot)
            {
                player_slot *Player = &AppState->Players[Slot];
                if (Player->Active && Player->Entity && !IsDeadPlayer(Player->Entity))
                {
                    Winner = Slot;
                }
            }
            Played++;
            printf("round\t%u\t%.1f\t%u\t%u\n", Played, Seconds, Winner, AppState->DuelSeed);
            PrintRoundPlayers(AppState, &Builds, Winner);
            Seconds = 0.f;
        }
        InBreak = Break;
        if (!Break && Seconds > PROBE_ROUND_SECONDS)
        {
            printf("stuck\t%u\n", Played + 1);
            break;
        }
    }
    GameShutdown(&Game);
}

#include "duel_balance/tally.cpp"

int
main(int ArgCount, char **Args)
{
    u32 Rounds = ArgCount > 1 ? (u32)atoi(Args[1]) : 19;
    u32 Players = ArgCount > 2 ? (u32)atoi(Args[2]) : 2;
    u32 Seeds = ArgCount > 3 ? (u32)atoi(Args[3]) : 1;
    if (ArgCount > 4)
    {
        // NOTE(zoubir): one seed for ProbeSeeds
        ProbeChild = true;
        ProbeOneSeed(Rounds, Players, (u32)atoi(Args[4]));
    }
    else if (Seeds > 1)
    {
        ProbeSeeds(Rounds, Players, Seeds);
    }
    else
    {
        ProbeOneSeed(Rounds, Players, 1);
    }
    return 0;
}
