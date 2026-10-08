/* Player name tests: the name checks the connect screen runs before it
   joins (client/player_name.cpp), the suggested names, and a launch with
   a server but no name waiting offline instead of joining nameless.
   Included by sim_tests.cpp, which calls RunPlayerNameTests. */

internal void
TestPlayerNamesAreCleanedAndChecked()
{
    char Name[NET_NAME_SIZE];
    CleanPlayerName(Name, sizeof(Name), (char *)"   Ash    Fox  ");
    Check(strcmp(Name, "Ash Fox") == 0);
    CleanPlayerName(Name, sizeof(Name), (char *)"    ");
    Check(Name[0] == 0);
    CleanPlayerName(Name, 6, (char *)"Longer than six");
    Check(strcmp(Name, "Longe") == 0);

    Check(PlayerNameProblem((char *)"") != 0);
    Check(PlayerNameProblem((char *)"A") != 0);
    Check(PlayerNameProblem((char *)"Player 3") != 0);
    Check(PlayerNameProblem((char *)"Ax") == 0);
    Check(PlayerNameProblem((char *)"Player One") == 0);
    Check(PlayerNameProblem((char *)"Mahdi") == 0);
}

internal void
TestEverySuggestedNameFitsAndShuffles()
{
    char Name[64];
    for(u32 First = 0; First < ArrayCount(NameFirstWords); First++)
    {
        for(u32 Second = 0; Second < ArrayCount(NameSecondWords); Second++)
        {
            snprintf(Name, sizeof(Name), "%s %s", NameFirstWords[First], NameSecondWords[Second]);
            Check(strlen(Name) < NET_NAME_SIZE);
        }
    }
    char Shown[NET_NAME_SIZE], Next[NET_NAME_SIZE];
    for(u32 Seed = 0; Seed < 200; Seed++)
    {
        SuggestPlayerName(Shown, sizeof(Shown), Seed);
        Check(PlayerNameProblem(Shown) == 0);
        SuggestPlayerName(Next, sizeof(Next), Seed, Shown);
        Check(strcmp(Next, Shown) != 0);
    }
}

internal void
TestLaunchWithoutANameWaitsForOne()
{
    memory_index Size = Megabytes(8);
    memory_arena Arena;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);

    // NOTE(zoubir): nothing listens on port 9; nothing should even try
    TestSetEnv(ONLINE_ADDRESS_ENV, "127.0.0.1:9");
    TestSetEnv(ONLINE_NAME_ENV, "");
    online_session *Nameless = StartOnlineSession(&Arena, 0, true);
    Check(!Nameless->Enabled);
    Check(Nameless->NeedsName);

    TestSetEnv(ONLINE_NAME_ENV, "  Iron   Owl ");
    online_session *Named = StartOnlineSession(&Arena, 0, true);
    Check(Named->Enabled);
    Check(!Named->NeedsName);
    Check(strcmp(Named->NameText, "Iron Owl") == 0);
    NetClientDisconnect(&Named->Client);

    TestSetEnv(ONLINE_ADDRESS_ENV, "");
    TestSetEnv(ONLINE_NAME_ENV, "");
    online_session *Offline = StartOnlineSession(&Arena, 0, true);
    Check(!Offline->Enabled);
    Check(!Offline->NeedsName);
    free(Arena.Base);
}

internal void
RunPlayerNameTests()
{
    printf("TestPlayerNamesAreCleanedAndChecked\n");
    TestPlayerNamesAreCleanedAndChecked();
    printf("TestEverySuggestedNameFitsAndShuffles\n");
    TestEverySuggestedNameFitsAndShuffles();
    printf("TestLaunchWithoutANameWaitsForOne\n");
    TestLaunchWithoutANameWaitsForOne();
}
