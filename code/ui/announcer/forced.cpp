/* Forced cards (announcer.cpp): GAME_ANNOUNCE=<card> pushes one card as
   the map starts and holds it still, so misc\screenshot.bat can catch
   how it looks without playing up to the moment. The cards: fight,
   double, firstblood, boss, cleared, level, wipe, toasts, and vote (the
   vote plate, ui/map_vote_view.cpp). */

internal void
ForceAnnouncement(app_state *AppState, char *Which)
{
    if (!Which || !Which[0])
    {
        return;
    }
    announcer *Announcer = GetAnnouncer(AppState);
    Announcer->Showing = false;
    Announcer->QueueCount = 0;
    if (!strcmp(Which, "fight"))
    {
        AnnounceFight(AppState, 2);
    }
    else if (!strcmp(Which, "double"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Medal,
                                    ANNOUNCE_COLOR_GOLD, 0, (char *)"DOUBLE KILL!", 0));
    }
    else if (!strcmp(Which, "firstblood"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Event,
                                    ANNOUNCE_COLOR_RED, 0, (char *)"FIRST BLOOD",
                                    (char *)"You drew first blood"));
    }
    else if (!strcmp(Which, "boss"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                    ANNOUNCE_COLOR_RED, (char *)"BOSS FIGHT",
                                    (char *)"Gravecaller Ossian",
                                    (char *)"The Ossuary. Stay out of the red, mind its enrage timer"));
    }
    else if (!strcmp(Which, "cleared"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Big,
                                    ANNOUNCE_COLOR_GOLD, (char *)"Gravecaller Ossian",
                                    (char *)"BOSS DEFEATED", (char *)"Next: the Bone Halls"));
    }
    else if (!strcmp(Which, "level"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                    ANNOUNCE_COLOR_GOLD, (char *)"LEVEL 1 CLEARED",
                                    (char *)"VICTORY", (char *)"Down to the Ember Depths next"));
    }
    else if (!strcmp(Which, "wipe"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                    ANNOUNCE_COLOR_RED, (char *)"THE ROOM HOLDS",
                                    (char *)"PARTY WIPED",
                                    (char *)"Back at the gate. Regroup and pull again"));
    }
    else if (!strcmp(Which, "vote"))
    {
        // NOTE(zoubir): offline a lone player's vote passes at once; this
        // opens one from an empty slot, which stays open its 30 seconds
        AppState->VoteOpen = true;
        AppState->VoteMap = FirstMapOfMode(!IsDungeonMap(AppState->World.MapId));
        AppState->VoteBy = (AppState->LocalPlayerIndex + 1) % MAX_PLAYERS;
        AppState->VoteSeconds = 21.f;
    }
    else if (!strcmp(Which, "toasts"))
    {
        PushToast(AppState, UI_COLOR_GOOD, (char *)"Mira joined the game");
        PushToast(AppState, UI_COLOR_ACCENT, (char *)"Gary started a map vote");
        PushToast(AppState, UI_COLOR_HEALTH, (char *)"Mira is down");
    }
    // NOTE(zoubir): held still at full strength
    if (Announcer->Showing)
    {
        Announcer->Now.Seconds = 1000.f;
        Announcer->Now.Age = 0.6f;
    }
}
