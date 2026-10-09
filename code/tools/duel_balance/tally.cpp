/* Duel balance seeds (duel_balance.cpp): runs every seed in its own
   process, as many at once as there are cores, and sums their rounds
   into one table: rounds won by build, then by talent held. */

#define PROBE_MAX_ROWS 96

// NOTE(zoubir): one build's or one talent's rounds over every seed
struct probe_row
{
    char Name[48];
    u32 Rounds;
    u32 Wins;
};

internal probe_row *
FindRow(probe_row *Rows, u32 *Count, char *Name)
{
    probe_row *Result = 0;
    for (u32 Index = 0; Index < *Count && !Result; ++Index)
    {
        if (strcmp(Rows[Index].Name, Name) == 0)
        {
            Result = &Rows[Index];
        }
    }
    if (!Result && *Count < PROBE_MAX_ROWS)
    {
        Result = &Rows[(*Count)++];
        *Result = {};
        snprintf(Result->Name, sizeof(Result->Name), "%s", Name);
    }
    return Result;
}

internal int
CompareRowsByShare(const void *A, const void *B)
{
    probe_row *Left = (probe_row *)A;
    probe_row *Right = (probe_row *)B;
    float LeftShare = Left->Rounds ? (float)Left->Wins / (float)Left->Rounds : 0.f;
    float RightShare = Right->Rounds ? (float)Right->Wins / (float)Right->Rounds : 0.f;
    int Result = LeftShare < RightShare ? 1 : LeftShare > RightShare ? -1 : 0;
    return Result;
}

// NOTE(zoubir): one player line of a round: the build's row, and a row
// for every talent it held
internal void
TallyPlayer(char *Line, probe_row *Builds, u32 *BuildCount, probe_row *Talents,
            u32 *TalentCount, u32 *PlayerRounds, u32 *PlayerWins)
{
    u32 Slot = 0;
    u32 Won = 0;
    char Build[48];
    char Held[1024] = "";
    if (sscanf(Line, "player\t%u\t%47[^\t]\t%u\t%1023[^\n]", &Slot, Build, &Won, Held) < 3)
    {
        return;
    }
    *PlayerRounds += 1;
    *PlayerWins += Won;
    probe_row *Row = FindRow(Builds, BuildCount, Build);
    if (Row)
    {
        Row->Rounds++;
        Row->Wins += Won;
    }
    for (char *Item = strtok(Held, ","); Item; Item = strtok(0, ","))
    {
        char *Colon = strrchr(Item, ':');
        if (Colon)
        {
            *Colon = 0;
        }
        Row = FindRow(Talents, TalentCount, Item);
        if (Row)
        {
            Row->Rounds++;
            Row->Wins += Won;
        }
    }
}

internal void
ProbeSeeds(u32 Rounds, u32 Players, u32 Seeds)
{
    char *JobsText = getenv("PROBE_JOBS");
    char *Cores = getenv("NUMBER_OF_PROCESSORS");
    bool32 Verbose = getenv("PROBE_VERBOSE") != 0;
    u32 Jobs = JobsText ? (u32)atoi(JobsText) : Cores ? (u32)atoi(Cores) : 8;
    Jobs = Maximum(1u, Minimum(Jobs, (u32)PROBE_MAX_JOBS));
    char Program[1024] = "duel_balance";
#if defined(_WIN32)
    GetModuleFileNameA(0, Program, sizeof(Program));
#endif
    static probe_row Builds[PROBE_MAX_ROWS];
    static probe_row Talents[PROBE_MAX_ROWS];
    u32 BuildCount = 0;
    u32 TalentCount = 0;
    u32 PlayerRounds = 0;
    u32 PlayerWins = 0;
    u32 RoundCount = 0;
    u32 Draws = 0;
    u32 Stuck = 0;
    float Seconds = 0.f;
    float Longest = 0.f;
    printf("duel balance: %u bots, %u rounds, %u seeds, %u at once\n", Players, Rounds, Seeds, Jobs);
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
                     "\"\"%s\" %u %u 1 %u > \"duel_seed_%u.txt\"\"",
#else
                     "\"%s\" %u %u 1 %u > \"duel_seed_%u.txt\"",
#endif
                     Program, Rounds, Players, SeedNumber, SeedNumber);
            Pipes[SeedNumber - First] = ProbeOpen(Command, "r");
        }
        for (u32 SeedNumber = First; SeedNumber <= Last; ++SeedNumber)
        {
            if (Pipes[SeedNumber - First])
            {
                ProbeClose(Pipes[SeedNumber - First]);
            }
            char FileName[64];
            snprintf(FileName, sizeof(FileName), "duel_seed_%u.txt", SeedNumber);
            FILE *File = fopen(FileName, "r");
            if (!File)
            {
                printf("  seed %u: no output\n", SeedNumber);
                continue;
            }
            char Line[2048];
            while (fgets(Line, sizeof(Line), File))
            {
                if (Verbose)
                {
                    printf("  seed %u  %s", SeedNumber, Line);
                }
                u32 Number = 0;
                float Fought = 0.f;
                u32 Winner = 0;
                if (sscanf(Line, "round\t%u\t%f\t%u", &Number, &Fought, &Winner) == 3)
                {
                    RoundCount++;
                    Seconds += Fought;
                    Longest = Maximum(Longest, Fought);
                    Draws += Winner >= MAX_PLAYERS ? 1 : 0;
                }
                else if (strncmp(Line, "player\t", 7) == 0)
                {
                    TallyPlayer(Line, Builds, &BuildCount, Talents, &TalentCount,
                                &PlayerRounds, &PlayerWins);
                }
                else if (strncmp(Line, "stuck\t", 6) == 0)
                {
                    Stuck++;
                }
            }
            fclose(File);
            remove(FileName);
        }
    }

    printf("\n%u rounds, %.1f s on average, longest %.1f s, %u with nobody standing, %u seeds stuck\n",
           RoundCount, RoundCount ? Seconds / (float)RoundCount : 0.f, Longest, Draws, Stuck);
    printf("\n%-18s %8s %8s\n", "build", "rounds", "won");
    qsort(Builds, BuildCount, sizeof(probe_row), CompareRowsByShare);
    for (u32 Index = 0; Index < BuildCount; ++Index)
    {
        probe_row *Row = &Builds[Index];
        printf("%-18s %8u %7.0f%%\n", Row->Name, Row->Rounds,
               Row->Rounds ? 100.f * (float)Row->Wins / (float)Row->Rounds : 0.f);
    }
    // NOTE(zoubir): a talent's rounds won by those who had it, against
    // those who did not; the gap is what it is worth, as far as bots play it
    printf("\n%-18s %8s %8s %8s %8s\n", "talent held", "rounds", "won", "without", "gap");
    qsort(Talents, TalentCount, sizeof(probe_row), CompareRowsByShare);
    for (u32 Index = 0; Index < TalentCount; ++Index)
    {
        probe_row *Row = &Talents[Index];
        u32 OtherRounds = PlayerRounds - Row->Rounds;
        float With = Row->Rounds ? 100.f * (float)Row->Wins / (float)Row->Rounds : 0.f;
        float Without = OtherRounds ? 100.f * (float)(PlayerWins - Row->Wins) / (float)OtherRounds : 0.f;
        printf("%-18s %8u %7.0f%% %7.0f%% %+7.0f\n", Row->Name, Row->Rounds, With, Without,
               With - Without);
    }
}
