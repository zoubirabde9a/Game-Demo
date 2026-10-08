/* People watch (announcer.cpp): who comes and goes, and the map vote.

   - A human joining or leaving, as a toast. Bots (the server names them
     "Bot N") fill the slots nobody uses and are never announced; a human
     taking a bot's slot over joins, a bot taking a human's leaves it.
     Names come one slot per snapshot online, so a join waits a moment
     for the name.
   - The map vote opening (with a tick, unless the local player asked),
     then passing or failing.
   - The connection to the server dropping. */

// NOTE(zoubir): the server names its bots "Bot N" (server/sim_game.cpp)
internal bool32
IsBotName(char *Name)
{
    bool32 Result = Name[0] == 'B' && Name[1] == 'o' && Name[2] == 't' && Name[3] == ' ' &&
        Name[4] >= '0' && Name[4] <= '9';
    return Result;
}

// NOTE(zoubir): a human in the slot; a slot not named yet counts, the
// name follows
inline bool32
IsHumanSlot(player_slot *Slot)
{
    bool32 Result = Slot->Active && !IsBotName(Slot->Name);
    return Result;
}

// NOTE(zoubir): a slot whose human came or went. Quiet only takes note
internal void
WatchPlayers(app_state *AppState, announcer *Announcer, float DeltaTime, bool32 Quiet)
{
    char Text[96];
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        bool32 Human = IsHumanSlot(Slot);
        bool32 Was = Announcer->SlotHuman[SlotIndex];
        char *Known = Announcer->SlotName[SlotIndex];
        if (Quiet || SlotIndex == AppState->LocalPlayerIndex)
        {
            Announcer->JoinWait[SlotIndex] = 0.f;
        }
        else if (Human && !Was)
        {
            Announcer->JoinWait[SlotIndex] = ANNOUNCE_JOIN_NAME_WAIT;
        }
        else if (!Human && Was && Announcer->JoinWait[SlotIndex] <= 0.f)
        {
            char Name[24];
            GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
            snprintf(Text, sizeof(Text), "%s left the game", Known[0] ? Known : Name);
            PushToast(AppState, AnnounceIcon_Left, UI_COLOR_TEXT_MUTED, Text);
        }
        else if (!Human)
        {
            // NOTE(zoubir): came and went before its name did
            Announcer->JoinWait[SlotIndex] = 0.f;
        }
        if (Human && Announcer->JoinWait[SlotIndex] > 0.f)
        {
            Announcer->JoinWait[SlotIndex] -= DeltaTime;
            if (Slot->Name[0] || Announcer->JoinWait[SlotIndex] <= 0.f)
            {
                Announcer->JoinWait[SlotIndex] = 0.f;
                char Name[24];
                GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
                snprintf(Text, sizeof(Text), "%s joined the game", Name);
                PushToast(AppState, AnnounceIcon_Joined, UI_COLOR_GOOD, Text);
            }
        }
        Announcer->SlotHuman[SlotIndex] = Human;
        if (!Human)
        {
            Known[0] = 0;
        }
        else if (Slot->Name[0])
        {
            snprintf(Known, sizeof(Announcer->SlotName[SlotIndex]), "%s", Slot->Name);
        }
    }
}

// NOTE(zoubir): in a dungeon, an ally downed and now standing again
// (a healer's revive, or the fight ending)
internal void
WatchRevives(app_state *AppState, announcer *Announcer, bool32 Quiet)
{
    bool32 Dungeon = IsDungeon(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        bool32 Down = Slot->Active && Slot->Entity && IsDeadPlayer(Slot->Entity);
        if (Dungeon && !Quiet && Announcer->SlotDown[SlotIndex] && !Down && Slot->Active &&
            Announcer->Run && Announcer->Run->FightingRoom)
        {
            char Text[96];
            if (SlotIndex == AppState->LocalPlayerIndex)
            {
                snprintf(Text, sizeof(Text), "You are back on your feet");
            }
            else
            {
                char Name[24];
                GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
                snprintf(Text, sizeof(Text), "%s is back up", Name);
            }
            PushToast(AppState, AnnounceIcon_Heart, UI_COLOR_GOOD, Text);
        }
        Announcer->SlotDown[SlotIndex] = Down;
    }
}

// NOTE(zoubir): the server stopped answering, and came back
// (client/online.cpp reconnects by itself when it is worth it)
internal void
WatchConnection(app_state *AppState, announcer *Announcer, bool32 Online)
{
    online_session *Session = AppState->Online;
    if (Announcer->WasOnline && !Online && Session && Session->Enabled)
    {
        Announcer->LostServer = true;
        PushToast(AppState, AnnounceIcon_SignalLost, UI_COLOR_HEALTH,
                  (char *)(WillReconnect(Session) ? "Lost the server. Reconnecting..." :
                           "Lost the server. F4 to join again"));
    }
    else if (Online && !Announcer->WasOnline && Announcer->LostServer)
    {
        Announcer->LostServer = false;
        PushToast(AppState, AnnounceIcon_Signal, UI_COLOR_GOOD, (char *)"Back on the server");
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
            PushToast(AppState, AnnounceIcon_Ballot, UI_COLOR_ACCENT, Text);
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
                PushToast(AppState, AnnounceIcon_Check, UI_COLOR_GOOD, Text);
            }
            else
            {
                snprintf(Text, sizeof(Text), "Vote failed: %s stays",
                         GetMapDef((map_id)AppState->World.MapId)->Name);
                PushToast(AppState, AnnounceIcon_Cross, UI_COLOR_HEALTH, Text);
            }
        }
    }
}
