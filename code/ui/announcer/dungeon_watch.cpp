/* Dungeon watch (announcer.cpp): what the announcer says during a dungeon
   run (sim/dungeon/), read from the run's room states, fight and Shown
   fields, which the snapshot fills online.

   - A fight starts: a boss gets a title card with its name and its room;
     any other room a callout with its name and how many enemies wait.
   - A boss under a fifth of its health: "Finish it!".
   - A room cleared: "Room cleared", or "<boss> defeated" for a boss room.
   - The last room cleared: a title card, "Level N cleared", and where the
     party goes next (the next level, or back to the top, stronger), with
     the run's time offline.
   - The party wiped (the run's Wipes went up): a red title card.

   The level's own title card comes from match_watch.cpp, as for any new
   map. Allies going down are in the kill feed, so match_watch.cpp says
   that too. */

// NOTE(zoubir): a fight's boss shows in the Shown fields a snapshot or
// two after the fight starts; the card waits that long to know
#define ANNOUNCE_FIGHT_DELAY 0.35f
// NOTE(zoubir): the boss's share of health that calls "Finish it"
#define ANNOUNCE_BOSS_LOW_SHARE 0.2f

internal void
AnnounceFightStart(app_state *AppState, announcer *Announcer)
{
    dungeon_run *Run = AppState->Dungeon;
    u32 MapId = AppState->World.MapId;
    char *Room = GetRoomName(MapId, Run->FightingRoom);
    char Text[128];
    if (Run->ShownBossKind < MonsterKind_Count)
    {
        Announcer->FightBoss = Run->ShownBossKind;
        monster_def *Boss = GetMonsterDef((monster_kind)Run->ShownBossKind);
        snprintf(Text, sizeof(Text), "%s. Stay out of the red, mind its enrage timer", Room);
        announce_card Card = MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                      AnnounceIcon_Boss,
                                      ANNOUNCE_COLOR_RED, (char *)"BOSS FIGHT", Boss->Name, Text);
        Card.Seconds = 2.8f;
        Card.Sound = AssetType_SfxAnnounce;
        PushCard(AppState, Card);
    }
    else
    {
        char Kicker[48];
        snprintf(Kicker, sizeof(Kicker), "ROOM %u OF %u", Run->FightingRoom, Run->RoomCount);
        snprintf(Text, sizeof(Text), "%u %s. Tank first, then burn them down",
                 Run->ShownFoesLeft, Run->ShownFoesLeft == 1 ? "enemy" : "enemies");
        announce_card Card = MakeCard(AnnounceStyle_Callout, AnnouncePriority_Event,
                                      AnnounceIcon_Gate,
                                      ANNOUNCE_COLOR_VIOLET, Kicker, Room, Text);
        Card.Sound = AssetType_SfxFight;
        PushCard(AppState, Card);
    }
}

internal void
AnnounceRoomCleared(app_state *AppState, announcer *Announcer, u32 Room)
{
    dungeon_run *Run = AppState->Dungeon;
    u32 MapId = AppState->World.MapId;
    char Text[128];
    if (!NextRoomToClear(Run->RoomStates, Run->RoomCount))
    {
        Announcer->LevelDone = true;
        dungeon_level *Level = GetDungeonLevel(MapId);
        u32 Next = NextRunMap(MapId);
        dungeon_level *NextLevel = GetDungeonLevel(Next);
        bool32 Deeper = NextLevel && Level && NextLevel->Number > Level->Number;
        char Kicker[48];
        snprintf(Kicker, sizeof(Kicker), "LEVEL %u CLEARED", Level ? Level->Number : 1);
        u32 Used = (u32)snprintf(Text, sizeof(Text), Deeper ? "Down to the %s next" :
                                 "Every level beaten. Back up to the %s, stronger",
                                 GetMapDef((map_id)Next)->Name);
        // NOTE(zoubir): the run's clock runs where the run does, so only
        // offline knows it
        if (!IsOnline(AppState->Online) && Used < sizeof(Text))
        {
            u32 Seconds = (u32)Run->Seconds;
            snprintf(Text + Used, sizeof(Text) - Used, "   (%u:%02u)", Seconds / 60, Seconds % 60);
        }
        announce_card Card = MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                      AnnounceIcon_Trophy,
                                      ANNOUNCE_COLOR_GOLD, Kicker,
                                      (char *)(Deeper ? "VICTORY" : "DUNGEON CONQUERED"), Text);
        Card.Seconds = 4.f;
        Card.Sound = AssetType_SfxLevelUp;
        PushCard(AppState, Card);
        return;
    }
    u32 Next = NextRoomToClear(Run->RoomStates, Run->RoomCount);
    snprintf(Text, sizeof(Text), "Next: %s", GetRoomName(MapId, Next));
    if (Announcer->FightBoss < MonsterKind_Count && Room == Announcer->FightingRoom)
    {
        announce_card Card = MakeCard(AnnounceStyle_Callout, AnnouncePriority_Big,
                                      AnnounceIcon_Crown,
                                      ANNOUNCE_COLOR_GOLD,
                                      GetMonsterDef((monster_kind)Announcer->FightBoss)->Name,
                                      (char *)"BOSS DEFEATED", Text);
        Card.Seconds = 2.4f;
        Card.Sound = AssetType_SfxLevelUp;
        PushCard(AppState, Card);
    }
    else
    {
        announce_card Card = MakeCard(AnnounceStyle_Callout, AnnouncePriority_Event,
                                      AnnounceIcon_Check,
                                      UI_COLOR_GOOD, 0, (char *)"ROOM CLEARED", Text);
        Card.Sound = AssetType_SfxCountdown;
        PushCard(AppState, Card);
    }
}

// NOTE(zoubir): a new run: what it shows now is where it starts
internal void
StartDungeonWatch(announcer *Announcer, dungeon_run *Run)
{
    Announcer->Run = Run;
    memcpy(Announcer->RoomStates, Run->RoomStates, sizeof(Announcer->RoomStates));
    // NOTE(zoubir): a fight already on (a pull right as the map loads, or
    // joining mid-fight) still gets its card
    Announcer->FightingRoom = 0;
    Announcer->Fighting = false;
    Announcer->FightDue = 0.f;
    Announcer->FightBoss = MonsterKind_Count;
    Announcer->Wipes = Run->Wipes;
    Announcer->LevelDone = !NextRoomToClear(Run->RoomStates, Run->RoomCount);
}

internal void
WatchDungeon(app_state *AppState, announcer *Announcer, float DeltaTime)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run || !Announcer->Started || Announcer->TitleDue > 0.f)
    {
        return;
    }
    if (Announcer->Run != Run)
    {
        StartDungeonWatch(Announcer, Run);
    }
    if (Run->Wipes > Announcer->Wipes)
    {
        announce_card Card = MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                      AnnounceIcon_Grave,
                                      ANNOUNCE_COLOR_RED, (char *)"THE ROOM HOLDS",
                                      (char *)"PARTY WIPED",
                                      (char *)"Back at the gate. Regroup and pull again");
        Card.Seconds = 3.f;
        Card.Sound = AssetType_SfxAnnounce;
        PushCard(AppState, Card);
        Announcer->FightDue = 0.f;
    }
    Announcer->Wipes = Run->Wipes;

    // NOTE(zoubir): FightingRoom keeps the last room fought after the
    // fight, so a boss room cleared a frame later is still known
    if (Run->FightingRoom && (!Announcer->Fighting || Run->FightingRoom != Announcer->FightingRoom))
    {
        Announcer->FightDue = ANNOUNCE_FIGHT_DELAY;
        Announcer->FightBoss = MonsterKind_Count;
        Announcer->BossLowShown = false;
    }
    if (Run->FightingRoom)
    {
        Announcer->FightingRoom = Run->FightingRoom;
        if (Run->ShownBossKind < MonsterKind_Count)
        {
            Announcer->FightBoss = Run->ShownBossKind;
        }
    }
    if (Announcer->FightDue > 0.f)
    {
        Announcer->FightDue -= DeltaTime;
        if (Announcer->FightDue <= 0.f && Run->FightingRoom)
        {
            AnnounceFightStart(AppState, Announcer);
        }
    }

    // NOTE(zoubir): the boss is nearly down: everything into it now
    if (Run->FightingRoom && Run->ShownBossKind < MonsterKind_Count && !Announcer->BossLowShown &&
        Run->ShownBossShare > 0.f && Run->ShownBossShare <= ANNOUNCE_BOSS_LOW_SHARE)
    {
        Announcer->BossLowShown = true;
        char Detail[64];
        snprintf(Detail, sizeof(Detail), "Under %.0f%% health. Everything into it",
                 100.f * ANNOUNCE_BOSS_LOW_SHARE);
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Event,
                                    AnnounceIcon_Target,
                                    ANNOUNCE_COLOR_RED,
                                    GetMonsterDef((monster_kind)Run->ShownBossKind)->Name,
                                    (char *)"FINISH IT!", Detail));
    }

    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        bool32 Cleared = Run->RoomStates[Room] == RoomState_Cleared;
        bool32 Was = Announcer->RoomStates[Room] == RoomState_Cleared;
        if (Cleared && !Was && !Announcer->LevelDone)
        {
            AnnounceRoomCleared(AppState, Announcer, Room);
        }
        Announcer->RoomStates[Room] = Run->RoomStates[Room];
    }
    Announcer->Fighting = Run->FightingRoom != 0;
}
