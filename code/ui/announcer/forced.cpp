/* Forced cards (announcer.cpp): GAME_ANNOUNCE=<card> pushes one card as
   the map starts and holds it still, so misc\screenshot.bat can catch
   how it looks without playing up to the moment. The cards: fight,
   double, firstblood, boss, cleared, level, wipe, toasts, and vote (the
   vote plate, ui/map_vote_view.cpp), and icons, every icon at two
   sizes (DrawIconSheet). */

// NOTE(zoubir): every announcer icon across the middle of the screen, as
// cards draw them and as toasts do, to look them over
internal void
DrawIconSheet(render_context *RenderContext, announcer *Announcer, u32 WindowWidth,
              u32 WindowHeight)
{
    float Big = 72.f;
    float Cell = Big + 24.f;
    u32 Columns = 10;
    float Left = 0.5f * ((float)WindowWidth - (float)Columns * Cell);
    float Top = 0.3f * (float)WindowHeight;
    DrawFilledRectangle(RenderContext, 0.f, Top - 0.5f * Cell, (float)WindowWidth, 4.f * Cell,
                        UI_RGBA(10, 12, 16, 230), 0.f);
    for(u32 Icon = 1; Icon < AnnounceIcon_Count; Icon++)
    {
        u32 Index = Icon - 1;
        float X = Left + (float)(Index % Columns) * Cell + 0.5f * Cell;
        float Y = Top + (float)(Index / Columns) * 2.f * Cell;
        DrawAnnounceIcon(RenderContext, &Announcer->IconAtlas, Icon, V2(X, Y), Big, 1.f);
        DrawAnnounceIcon(RenderContext, &Announcer->IconAtlas, Icon, V2(X, Y + 0.75f * Cell),
                         22.f, 1.f);
    }
}

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
                                    AnnounceIcon_Skull,
                                    ANNOUNCE_COLOR_GOLD, 0, (char *)"DOUBLE KILL!", 0));
    }
    else if (!strcmp(Which, "firstblood"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Event,
                                    AnnounceIcon_Blood,
                                    ANNOUNCE_COLOR_RED, 0, (char *)"FIRST BLOOD",
                                    (char *)"You drew first blood"));
    }
    else if (!strcmp(Which, "boss"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Title, AnnouncePriority_Title, AnnounceIcon_Boss,
                                    ANNOUNCE_COLOR_RED, (char *)"BOSS FIGHT",
                                    (char *)"Gravecaller Ossian",
                                    (char *)"The Ossuary. Stay out of the red, mind its enrage timer"));
    }
    else if (!strcmp(Which, "cleared"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Callout, AnnouncePriority_Big, AnnounceIcon_Crown,
                                    ANNOUNCE_COLOR_GOLD, (char *)"Gravecaller Ossian",
                                    (char *)"BOSS DEFEATED", (char *)"Next: the Bone Halls"));
    }
    else if (!strcmp(Which, "level"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Title, AnnouncePriority_Title,
                                    AnnounceIcon_Trophy,
                                    ANNOUNCE_COLOR_GOLD, (char *)"LEVEL 1 CLEARED",
                                    (char *)"VICTORY", (char *)"Down to the Ember Depths next"));
    }
    else if (!strcmp(Which, "wipe"))
    {
        PushCard(AppState, MakeCard(AnnounceStyle_Title, AnnouncePriority_Title, AnnounceIcon_Grave,
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
    else if (!strcmp(Which, "icons"))
    {
        Announcer->ShowIconSheet = true;
    }
    else if (!strcmp(Which, "toasts"))
    {
        PushToast(AppState, AnnounceIcon_Joined, UI_COLOR_GOOD, (char *)"Mira joined the game");
        PushToast(AppState, AnnounceIcon_Ballot, UI_COLOR_ACCENT,
                  (char *)"Gary started a map vote");
        PushToast(AppState, AnnounceIcon_HeartBroken, UI_COLOR_HEALTH, (char *)"Mira is down");
    }
    // NOTE(zoubir): held still at full strength
    if (Announcer->Showing)
    {
        Announcer->Now.Seconds = 1000.f;
        Announcer->Now.Age = 0.6f;
    }
}
