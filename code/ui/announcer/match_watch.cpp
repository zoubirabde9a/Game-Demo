/* Match watch (announcer.cpp): what the announcer says about the match
   itself, in every mode.

   - The game is ready: once the connect screen is closed and the local
     player is in, and again on every new map, a title card names the
     mode, the map and the goal. Duel rounds then call "Round 1, Fight!".
   - Rounds (sim/round_break.cpp): the last three seconds of a break count
     down, and the next round opens with "Round N, Fight!". When a round
     that started with three or more players is down to two, "Final two".
   - Kills, from the kill feed: first blood, the local player's double,
     triple and quad kills and rampages (monsters count too, from the
     score), killing sprees for anyone (the local player's as a callout,
     the others' as a toast) and the streak that ends them.
   - Players joining or leaving, and the map vote opening, passing or
     failing, as toasts. Names come one slot per snapshot online, so a
     join waits a moment for the name.

   For WARMUP seconds after a new map or a new connection the watch only
   takes note, so a whole server's players do not all "join" at once. */

#define ANNOUNCE_WARMUP_SECONDS 2.f
#define ANNOUNCE_TITLE_DELAY 0.4f
#define ANNOUNCE_JOIN_NAME_WAIT 1.5f
#define ANNOUNCE_MULTI_KILL_SECONDS 4.f
#define ANNOUNCE_COUNTDOWN_FROM 3

#define ANNOUNCE_COLOR_GOLD UI_RGBA(255, 214, 96, 255)
#define ANNOUNCE_COLOR_RED UI_RGBA(255, 96, 72, 255)
#define ANNOUNCE_COLOR_BLUE UI_RGBA(120, 190, 255, 255)
#define ANNOUNCE_COLOR_VIOLET UI_RGBA(200, 140, 255, 255)

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
                 "Pick your role, clear every room and beat the bosses");
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
    ZeroArray(Announcer->Streak, MAX_PLAYERS, u32);
    Announcer->FeedSeen = AppState->KillFeed ? AppState->KillFeed->Total : 0;
    Announcer->MonsterKillsSeen = AppState->Players[AppState->LocalPlayerIndex].MonsterKills;
    Announcer->LastRoundBreak = AppState->RoundBreak;
    Announcer->Run = 0;
    // NOTE(zoubir): the old map's cards mean nothing on the new one
    Announcer->Showing = false;
    Announcer->QueueCount = 0;
}

// NOTE(zoubir): a slot whose player came or went, or a bot slot a human
// took over (bots have no name)
internal void
WatchPlayers(app_state *AppState, announcer *Announcer, float DeltaTime, bool32 Quiet)
{
    char Text[96];
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        bool32 Local = SlotIndex == AppState->LocalPlayerIndex;
        bool32 Was = Announcer->SlotActive[SlotIndex];
        char *OldName = Announcer->SlotName[SlotIndex];
        if (!Quiet && !Local)
        {
            if (Slot->Active && !Was)
            {
                Announcer->JoinWait[SlotIndex] = ANNOUNCE_JOIN_NAME_WAIT;
            }
            else if (!Slot->Active && Was)
            {
                char Name[24];
                GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
                snprintf(Text, sizeof(Text), "%s left the game", OldName[0] ? OldName : Name);
                PushToast(AppState, UI_COLOR_TEXT_MUTED, Text);
                Announcer->JoinWait[SlotIndex] = 0.f;
            }
            else if (Slot->Active && !OldName[0] && Slot->Name[0] &&
                     Announcer->JoinWait[SlotIndex] <= 0.f)
            {
                snprintf(Text, sizeof(Text), "%s joined the game", Slot->Name);
                PushToast(AppState, UI_COLOR_GOOD, Text);
            }
            else if (Slot->Active && OldName[0] && !Slot->Name[0])
            {
                snprintf(Text, sizeof(Text), "%s left the game", OldName);
                PushToast(AppState, UI_COLOR_TEXT_MUTED, Text);
            }
            if (Announcer->JoinWait[SlotIndex] > 0.f)
            {
                Announcer->JoinWait[SlotIndex] -= DeltaTime;
                if (Slot->Name[0] || Announcer->JoinWait[SlotIndex] <= 0.f)
                {
                    Announcer->JoinWait[SlotIndex] = 0.f;
                    char Name[24];
                    GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
                    snprintf(Text, sizeof(Text), "%s joined the game", Name);
                    PushToast(AppState, UI_COLOR_GOOD, Text);
                }
            }
        }
        Announcer->SlotActive[SlotIndex] = Slot->Active;
        snprintf(Announcer->SlotName[SlotIndex], sizeof(Announcer->SlotName[SlotIndex]), "%s",
                 Slot->Active ? Slot->Name : "");
    }
}

internal void
WatchVote(app_state *AppState, announcer *Announcer, float DeltaTime)
{
    char Text[96];
    if (AppState->VoteOpen)
    {
        if (!Announcer->VoteWasOpen && AppState->VoteBy != AppState->LocalPlayerIndex)
        {
            char Who[24];
            GetPlayerName(AppState, AppState->VoteBy, Who, sizeof(Who));
            snprintf(Text, sizeof(Text), "%s started a map vote", Who);
            PushToast(AppState, UI_COLOR_ACCENT, Text);
            PlayGameSound(AppState, AssetType_SfxCountdown);
        }
        Announcer->VoteMapSeen = AppState->VoteMap;
        Announcer->VoteYesSeen = AppState->VoteYes;
        Announcer->VotePlayersSeen = CountActivePlayers(AppState);
    }
    else if (Announcer->VoteWasOpen)
    {
        // NOTE(zoubir): a vote that passes moves the map a tick later;
        // give the snapshot a moment to say so
        Announcer->VoteResultDue = 0.6f;
    }
    Announcer->VoteWasOpen = AppState->VoteOpen;
    if (Announcer->VoteResultDue > 0.f)
    {
        Announcer->VoteResultDue -= DeltaTime;
        u32 MapId = Announcer->VoteMapSeen;
        bool32 Passed = AppState->World.MapId == MapId ||
            2 * Announcer->VoteYesSeen > Announcer->VotePlayersSeen;
        if (Passed || Announcer->VoteResultDue <= 0.f)
        {
            Announcer->VoteResultDue = 0.f;
            char *Name = MapId < MapId_Count ? GetMapDef((map_id)MapId)->Name : (char *)"";
            if (Passed)
            {
                snprintf(Text, sizeof(Text), "Vote passed: on to %s", Name);
                PushToast(AppState, UI_COLOR_GOOD, Text);
            }
            else
            {
                snprintf(Text, sizeof(Text), "Vote failed: %s stays",
                         GetMapDef((map_id)AppState->World.MapId)->Name);
                PushToast(AppState, UI_COLOR_HEALTH, Text);
            }
        }
    }
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

// NOTE(zoubir): Count kills by the local player at once, within the
// multi-kill window of the last one
internal void
CountLocalKills(app_state *AppState, announcer *Announcer, u32 Count)
{
    if (!Count)
    {
        return;
    }
    if (Announcer->Clock - Announcer->LastKillAt > ANNOUNCE_MULTI_KILL_SECONDS)
    {
        Announcer->MultiKills = 0;
    }
    Announcer->LastKillAt = Announcer->Clock;
    u32 Before = Announcer->MultiKills;
    Announcer->MultiKills += Count;
    u32 Now = Announcer->MultiKills;
    char *Title = 0;
    if (Now >= 5 && Before < 5) Title = (char *)"RAMPAGE!";
    else if (Now == 4) Title = (char *)"QUAD KILL!";
    else if (Now == 3) Title = (char *)"TRIPLE KILL!";
    else if (Now == 2) Title = (char *)"DOUBLE KILL!";
    if (Title && FinalBlowLeft(AppState) <= 0.f)
    {
        announce_card Card = MakeCard(AnnounceStyle_Callout, AnnouncePriority_Medal,
                                      ANNOUNCE_COLOR_GOLD, 0, Title, 0);
        Card.Seconds = 1.4f;
        Card.Sound = AssetType_SfxFight;
        PushCard(AppState, Card);
    }
}

internal void
AnnounceSpree(app_state *AppState, u32 Killer, u32 Streak)
{
    char *Name = 0;
    if (Streak == 3) Name = (char *)"KILLING SPREE";
    else if (Streak == 5) Name = (char *)"UNSTOPPABLE";
    else if (Streak == 7) Name = (char *)"GODLIKE";
    if (!Name)
    {
        return;
    }
    char Text[96];
    if (Killer == AppState->LocalPlayerIndex)
    {
        snprintf(Text, sizeof(Text), "%u kills without dying", Streak);
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Event,
                                    ANNOUNCE_COLOR_GOLD, 0, Name, Text));
    }
    else
    {
        char Who[24];
        GetPlayerName(AppState, Killer, Who, sizeof(Who));
        snprintf(Text, sizeof(Text), "%s: %s (%u)", Who, Name, Streak);
        PushToast(AppState, ANNOUNCE_COLOR_RED, Text);
    }
}

internal void
WatchKills(app_state *AppState, announcer *Announcer)
{
    kill_feed *Feed = AppState->KillFeed;
    u32 Local = AppState->LocalPlayerIndex;
    bool32 Dungeon = IsDungeon(AppState);
    u32 LocalKills = 0;
    if (Feed)
    {
        u32 New = Minimum(Feed->Total - Announcer->FeedSeen, Feed->Count);
        Announcer->FeedSeen = Feed->Total;
        // NOTE(zoubir): oldest first, the feed lists newest first
        for(u32 Index = New; Index-- > 0;)
        {
            kill_feed_entry *Entry = &Feed->Entries[Index];
            u32 Killer = Entry->Killer;
            u32 Victim = Entry->Victim;
            char Text[96];
            char VictimName[24];
            GetPlayerName(AppState, Victim, VictimName, sizeof(VictimName));
            if (Dungeon)
            {
                // NOTE(zoubir): the local player's own death has the death plate
                if (Victim != Local)
                {
                    snprintf(Text, sizeof(Text), "%s is down", VictimName);
                    PushToast(AppState, UI_COLOR_HEALTH, Text);
                }
                continue;
            }
            bool32 ByPlayer = Killer < MAX_PLAYERS && Killer != Victim;
            if (Victim < MAX_PLAYERS)
            {
                if (ByPlayer && Announcer->Streak[Victim] >= 3)
                {
                    char KillerName[24];
                    GetPlayerName(AppState, Killer, KillerName, sizeof(KillerName));
                    snprintf(Text, sizeof(Text), "%s ended %s's streak", KillerName, VictimName);
                    PushToast(AppState, ANNOUNCE_COLOR_GOLD, Text);
                }
                Announcer->Streak[Victim] = 0;
            }
            if (!ByPlayer)
            {
                continue;
            }
            Announcer->Streak[Killer]++;
            if (!Announcer->FirstBloodDone)
            {
                Announcer->FirstBloodDone = true;
                char KillerName[24];
                GetPlayerName(AppState, Killer, KillerName, sizeof(KillerName));
                snprintf(Text, sizeof(Text), "%s drew first blood",
                         Killer == Local ? "You" : KillerName);
                if (FinalBlowLeft(AppState) <= 0.f)
                {
                    PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Event,
                                                ANNOUNCE_COLOR_RED, 0, (char *)"FIRST BLOOD",
                                                Text));
                }
            }
            AnnounceSpree(AppState, Killer, Announcer->Streak[Killer]);
            LocalKills += Killer == Local ? 1 : 0;
        }
    }
    u32 MonsterKills = AppState->Players[Local].MonsterKills;
    if (MonsterKills > Announcer->MonsterKillsSeen)
    {
        LocalKills += MonsterKills - Announcer->MonsterKillsSeen;
    }
    Announcer->MonsterKillsSeen = MonsterKills;
    CountLocalKills(AppState, Announcer, LocalKills);
}

internal void
WatchMatch(app_state *AppState, announcer *Announcer, float DeltaTime)
{
    player_slot *Local = &AppState->Players[AppState->LocalPlayerIndex];
    bool32 Online = IsOnline(AppState->Online);
    bool32 Ready = !ConnectScreenTakesInput(AppState) && Local->Active && Local->Entity;
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
}
