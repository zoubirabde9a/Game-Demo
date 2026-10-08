/* Match watch (announcer.cpp): what the announcer says about the match
   itself, in every mode.

   - The game is ready: once the connect screen is closed and the local
     player is in, and again on every new map, a title card names the
     mode, the map and the goal. Duel rounds then call "Round 1, Fight!".
   - Rounds (sim/round_break.cpp): the last three seconds of a break count
     down, and the next round opens with "Round N, Fight!". When a round
     that started with three or more players is down to two, "Final two".
   - Kills (kill_watch.cpp) and people coming and going (people_watch.cpp)
     are watched from here too.

   For WARMUP seconds after a new map or a new connection the watch only
   takes note, so a whole server's players do not all "join" at once. */

internal void
ForceAnnouncement(app_state *AppState, char *Which);

// NOTE(zoubir): the title card for the map being played
internal void
AnnounceMapStart(app_state *AppState)
{
    u32 MapId = AppState->World.MapId;
    map_def *Map = GetMapDef((map_id)MapId);
    char Kicker[48];
    char Detail[128];
    if (Map->Dungeon)
    {
        dungeon_level *Level = GetDungeonLevel(MapId);
        u32 Number = Level ? Level->Number : 1;
        snprintf(Kicker, sizeof(Kicker), "DUNGEON  -  LEVEL %u", Number);
        snprintf(Detail, sizeof(Detail), "%s",
                 Number > 1 ? "Deeper and deadlier. Clear every room and beat its bosses" :
                 "Pick your class, clear every room and beat the bosses");
    }
    else if (IsRoundMap(AppState))
    {
        snprintf(Kicker, sizeof(Kicker), "DUEL");
        snprintf(Detail, sizeof(Detail), "Last one standing wins the round");
    }
    else
    {
        snprintf(Kicker, sizeof(Kicker), "DUEL  -  FREE FOR ALL");
        snprintf(Detail, sizeof(Detail), "Fight players and monsters. You come back after each death");
    }
    announce_card Card = MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                  Map->Dungeon ? ANNOUNCE_COLOR_VIOLET : ANNOUNCE_COLOR_GOLD,
                                  Kicker, Map->Name, Detail);
    Card.Sound = AssetType_SfxAnnounce;
    PushCard(AppState, Card);
}

internal void
AnnounceFight(app_state *AppState, u32 Round)
{
    char Kicker[32];
    snprintf(Kicker, sizeof(Kicker), "ROUND %u", Round);
    announce_card Card = MakeCard(AnnounceStyle_Callout, AnnouncePriority_Big,
                                  ANNOUNCE_COLOR_RED, Kicker, (char *)"FIGHT!", 0);
    Card.Seconds = 1.3f;
    Card.Sound = AssetType_SfxFight;
    PushCard(AppState, Card);
}

// NOTE(zoubir): a new map or connection: everything seen so far is old
internal void
StartAnnouncerSession(app_state *AppState, announcer *Announcer)
{
    Announcer->Started = true;
    Announcer->MapId = AppState->World.MapId;
    Announcer->TitleDue = ANNOUNCE_TITLE_DELAY;
    Announcer->Warmup = ANNOUNCE_WARMUP_SECONDS;
    Announcer->Round = 1;
    Announcer->LastCountdown = 0;
    Announcer->FinalTwoShown = false;
    Announcer->FirstBloodDone = false;
    Announcer->MultiKills = 0;
    Announcer->Leader = MAX_PLAYERS;
    ZeroArray(Announcer->Streak, MAX_PLAYERS, u32);
    Announcer->FeedSeen = AppState->KillFeed ? AppState->KillFeed->Total : 0;
    Announcer->MonsterKillsSeen = AppState->Players[AppState->LocalPlayerIndex].MonsterKills;
    Announcer->LastRoundBreak = AppState->RoundBreak;
    Announcer->Run = 0;
    // NOTE(zoubir): the old map's cards mean nothing on the new one
    Announcer->Showing = false;
    Announcer->QueueCount = 0;
}

internal void
WatchRounds(app_state *AppState, announcer *Announcer)
{
    if (!IsRoundMap(AppState))
    {
        return;
    }
    float Break = AppState->RoundBreak;
    if (Break > 0.f && FinalBlowLeft(AppState) <= 0.f)
    {
        i32 Count = (i32)ceilf(RoundBreakLeft(AppState));
        if (Count >= 1 && Count <= ANNOUNCE_COUNTDOWN_FROM && Count != Announcer->LastCountdown)
        {
            char Kicker[32];
            char Number[8];
            snprintf(Kicker, sizeof(Kicker), "ROUND %u IN", Announcer->Round + 1);
            snprintf(Number, sizeof(Number), "%d", Count);
            announce_card Card = MakeCard(AnnounceStyle_Callout, AnnouncePriority_Countdown,
                                          UI_COLOR_TEXT, Kicker, Number, 0);
            Card.Seconds = 0.95f;
            Card.Sound = AssetType_SfxCountdown;
            PushCard(AppState, Card);
        }
        Announcer->LastCountdown = Count;
    }
    if (Announcer->LastRoundBreak > 0.f && Break <= 0.f)
    {
        Announcer->Round++;
        Announcer->LastCountdown = 0;
        Announcer->FinalTwoShown = false;
        Announcer->RoundPlayers = CountActivePlayers(AppState);
        AnnounceFight(AppState, Announcer->Round);
    }
    Announcer->LastRoundBreak = Break;

    bool32 AnyoneOut;
    u32 Standing = CountStandingPlayers(AppState, &AnyoneOut);
    if (Break <= 0.f && Standing == 2 && Announcer->RoundPlayers >= 3 &&
        !Announcer->FinalTwoShown)
    {
        Announcer->FinalTwoShown = true;
        char Names[2][24] = {};
        u32 Found = 0;
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS && Found < 2; SlotIndex++)
        {
            player_slot *Slot = &AppState->Players[SlotIndex];
            if (Slot->Active && Slot->Entity && !IsDeadPlayer(Slot->Entity))
            {
                GetPlayerName(AppState, SlotIndex, Names[Found++], sizeof(Names[0]));
            }
        }
        char Detail[64];
        snprintf(Detail, sizeof(Detail), "%s  vs  %s", Names[0], Names[1]);
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Big,
                                    ANNOUNCE_COLOR_RED, (char *)"LAST DUEL", (char *)"FINAL TWO",
                                    Detail));
    }
}

internal void
WatchMatch(app_state *AppState, announcer *Announcer, float DeltaTime)
{
    player_slot *Local = &AppState->Players[AppState->LocalPlayerIndex];
    bool32 Online = IsOnline(AppState->Online);
    bool32 Ready = !ConnectScreenTakesInput(AppState) && Local->Active && Local->Entity;
    WatchConnection(AppState, Announcer, Online);
    if (!Ready)
    {
        WatchPlayers(AppState, Announcer, DeltaTime, true);
        Announcer->WasOnline = Online;
        return;
    }
    if (!Announcer->Started || Announcer->MapId != AppState->World.MapId ||
        (Online && !Announcer->WasOnline))
    {
        StartAnnouncerSession(AppState, Announcer);
    }
    Announcer->WasOnline = Online;
    if (Announcer->TitleDue > 0.f)
    {
        Announcer->TitleDue -= DeltaTime;
        if (Announcer->TitleDue <= 0.f)
        {
            AnnounceMapStart(AppState);
            Announcer->RoundPlayers = CountActivePlayers(AppState);
            if (IsRoundMap(AppState) && AppState->RoundBreak <= 0.f &&
                Announcer->RoundPlayers > 1)
            {
                AnnounceFight(AppState, Announcer->Round);
            }
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once per map, never kept
#endif
            ForceAnnouncement(AppState, getenv("GAME_ANNOUNCE"));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
        }
    }
    bool32 Quiet = Announcer->Warmup > 0.f;
    Announcer->Warmup -= DeltaTime;
    WatchPlayers(AppState, Announcer, DeltaTime, Quiet);
    WatchVote(AppState, Announcer, DeltaTime);
    WatchRounds(AppState, Announcer);
    WatchKills(AppState, Announcer);
    WatchLead(AppState, Announcer, Quiet);
    WatchRevives(AppState, Announcer, Quiet);
}
