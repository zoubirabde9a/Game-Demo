/* Kill watch (announcer.cpp): what the kill feed and the scores say.

   - First blood, once per map.
   - The local player's double, triple and quad kills and rampages within
     ANNOUNCE_MULTI_KILL_SECONDS of each other; only players count, so
     clearing a monster pack is not a multi-kill.
   - Killing sprees for anyone (the local player's as a callout, the
     others' as a toast) and the kill that ends one.
   - In a dungeon, allies going down.
   - Outside one, who leads on kills, when that changes. */

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
                                      AnnounceIcon_Skull,
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
                                    AnnounceIcon_Flame,
                                    ANNOUNCE_COLOR_GOLD, 0, Name, Text));
    }
    else
    {
        char Who[24];
        GetPlayerName(AppState, Killer, Who, sizeof(Who));
        snprintf(Text, sizeof(Text), "%s: %s (%u)", Who, Name, Streak);
        PushToast(AppState, AnnounceIcon_Flame, ANNOUNCE_COLOR_RED, Text);
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
                    PushToast(AppState, AnnounceIcon_HeartBroken, UI_COLOR_HEALTH, Text);
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
                    PushToast(AppState, AnnounceIcon_Skull, ANNOUNCE_COLOR_GOLD, Text);
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
                                                AnnounceIcon_Blood,
                                                ANNOUNCE_COLOR_RED, 0, (char *)"FIRST BLOOD",
                                                Text));
                }
            }
            AnnounceSpree(AppState, Killer, Announcer->Streak[Killer]);
            LocalKills += Killer == Local ? 1 : 0;
        }
    }
    CountLocalKills(AppState, Announcer, LocalKills);
}

// NOTE(zoubir): outside a dungeon, the one player with the most kills,
// once they have a few; a tie keeps the old leader. Quiet only takes note
internal void
WatchLead(app_state *AppState, announcer *Announcer, bool32 Quiet)
{
    if (IsDungeon(AppState))
    {
        return;
    }
    u32 Best = MAX_PLAYERS;
    u32 BestKills = 0;
    bool32 Tie = false;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active)
        {
            continue;
        }
        if (Slot->Kills > BestKills)
        {
            Best = SlotIndex;
            BestKills = Slot->Kills;
            Tie = false;
        }
        else if (Slot->Kills == BestKills)
        {
            Tie = true;
        }
    }
    if (Tie || Best == MAX_PLAYERS || BestKills < ANNOUNCE_LEAD_MIN_KILLS ||
        Best == Announcer->Leader)
    {
        return;
    }
    u32 Before = Announcer->Leader;
    Announcer->Leader = Best;
    if (Quiet)
    {
        return;
    }
    char Text[96];
    u32 Local = AppState->LocalPlayerIndex;
    if (Best == Local)
    {
        snprintf(Text, sizeof(Text), "You took the lead with %u kills", BestKills);
        PushToast(AppState, AnnounceIcon_Crown, ANNOUNCE_COLOR_GOLD, Text);
    }
    else
    {
        char Name[24];
        GetPlayerName(AppState, Best, Name, sizeof(Name));
        snprintf(Text, sizeof(Text), Before == Local ? "%s took the lead from you (%u kills)" :
                 "%s took the lead (%u kills)", Name, BestKills);
        PushToast(AppState, AnnounceIcon_Crown,
                  Before == Local ? ANNOUNCE_COLOR_RED : UI_COLOR_TEXT_MUTED, Text);
    }
}
