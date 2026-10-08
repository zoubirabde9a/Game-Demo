/* Dungeon balance seeds (dungeon_balance.cpp): runs every seed of a
   probe in its own process, as many at once as there are cores, and
   sums their fights into one table per room. */

// NOTE(zoubir): one room's fights over every seed, for the table
struct probe_tally
{
    char Name[64];
    u32 Kills;
    u32 Wipes;
    u32 Deaths;
    float Seconds;
    float Longest;
};

internal probe_tally *
FindTally(probe_tally *Tallies, u32 *Count, char *Name)
{
    probe_tally *Result = 0;
    for (u32 Index = 0; Index < *Count && !Result; ++Index)
    {
        if (strcmp(Tallies[Index].Name, Name) == 0)
        {
            Result = &Tallies[Index];
        }
    }
    if (!Result && *Count < PROBE_MAX_ROOMS)
    {
        Result = &Tallies[(*Count)++];
        *Result = {};
        snprintf(Result->Name, sizeof(Result->Name), "%s", Name);
    }
    return Result;
}

// NOTE(zoubir): every seed in its own process, Jobs at once (the game
// keeps its world in globals, so seeds cannot share one), each writing
// to a file of its own; then one table over all of them
internal void
ProbeSeeds(u32 Minutes, u32 Players, u32 FirstRoom, u32 Seeds)
{
#pragma warning(push)
#pragma warning(disable: 4996)
    char *JobsText = getenv("PROBE_JOBS");
    char *Cores = getenv("NUMBER_OF_PROCESSORS");
    bool32 Verbose = getenv("PROBE_VERBOSE") != 0;
#pragma warning(pop)
    u32 Jobs = JobsText ? (u32)atoi(JobsText) : Cores ? (u32)atoi(Cores) : 8;
    Jobs = Maximum(1u, Minimum(Jobs, (u32)PROBE_MAX_JOBS));
    char Program[1024] = "dungeon_balance";
#if defined(_WIN32)
    GetModuleFileNameA(0, Program, sizeof(Program));
#endif
    static probe_tally Tallies[PROBE_MAX_ROOMS];
    u32 TallyCount = 0;
    u32 Stuck = 0;
    u32 Stalls = 0;
    printf("dungeon balance: %u bots, %u seeds, %u at once\n", Players, Seeds, Jobs);
    for (u32 First = 1; First <= Seeds; First += Jobs)
    {
        u32 Last = Minimum(Seeds, First + Jobs - 1);
        FILE *Pipes[PROBE_MAX_JOBS] = {};
        for (u32 SeedNumber = First; SeedNumber <= Last; ++SeedNumber)
        {
            // NOTE(zoubir): cmd strips the outer quotes round the whole line
            char Command[2048];
            snprintf(Command, sizeof(Command),
#if defined(_WIN32)
                     "\"\"%s\" %u %u %u 1 %u > \"probe_seed_%u.txt\"\"",
#else
                     "\"%s\" %u %u %u 1 %u > \"probe_seed_%u.txt\"",
#endif
                     Program, Minutes, Players, FirstRoom, SeedNumber, SeedNumber);
            Pipes[SeedNumber - First] = ProbeOpen(Command, "r");
        }
        for (u32 SeedNumber = First; SeedNumber <= Last; ++SeedNumber)
        {
            if (Pipes[SeedNumber - First])
            {
                ProbeClose(Pipes[SeedNumber - First]);
            }
            char FileName[64];
            snprintf(FileName, sizeof(FileName), "probe_seed_%u.txt", SeedNumber);
#pragma warning(push)
#pragma warning(disable: 4996)
            FILE *File = fopen(FileName, "r");
#pragma warning(pop)
            if (!File)
            {
                printf("  seed %u: no output\n", SeedNumber);
                continue;
            }
            if (Verbose)
            {
                printf("seed %u\n", SeedNumber);
            }
            char Line[1024];
            while (fgets(Line, sizeof(Line), File))
            {
                char Name[64];
                u32 Cleared = 0;
                float Seconds = 0.f;
                u32 Deaths = 0;
#pragma warning(push)
#pragma warning(disable: 4996)
                bool32 IsFight = sscanf(Line, "fight\t%63[^\t]\t%u\t%f\t%u",
                                        Name, &Cleared, &Seconds, &Deaths) == 4;
                bool32 IsStuck = !IsFight && sscanf(Line, "stuck\t%63[^\n]", Name) == 1;
#pragma warning(pop)
                if (IsFight)
                {
                    probe_tally *Tally = FindTally(Tallies, &TallyCount, Name);
                    if (Tally)
                    {
                        Tally->Deaths += Deaths;
                        if (Cleared)
                        {
                            Tally->Kills++;
                            Tally->Seconds += Seconds;
                            Tally->Longest = Maximum(Tally->Longest, Seconds);
                        }
                        else
                        {
                            Tally->Wipes++;
                        }
                    }
                }
                else if (strncmp(Line, "stall\t", 6) == 0)
                {
                    Stalls++;
                }
                else if (IsStuck)
                {
                    Stuck++;
                    printf("  seed %u: stuck at the %s, ended\n", SeedNumber, Name);
                }
                else if (Verbose)
                {
                    printf("%s", Line);
                }
            }
            fclose(File);
            remove(FileName);
        }
    }
    printf("  %-20s %5s %8s %9s %7s %7s\n", "room", "kills", "wipes/k", "deaths/k",
           "avg s", "max s");
    for (u32 Index = 0; Index < TallyCount; ++Index)
    {
        probe_tally *Tally = &Tallies[Index];
        float Kills = (float)Maximum(1u, Tally->Kills);
        printf("  %-20s %5u %8.2f %9.2f %7.1f %7.1f\n", Tally->Name, Tally->Kills,
               (float)Tally->Wipes / Kills, (float)Tally->Deaths / Kills,
               Tally->Seconds / Kills, Tally->Longest);
    }
    if (Stalls)
    {
        printf("  bots got lost between fights %u times and were walked on\n", Stalls);
    }
    if (Stuck)
    {
        printf("  %u of %u seeds ended early, stuck\n", Stuck, Seeds);
    }
}
