/* Team HUD: in a team duel, a plate under the score at the top left with
   each team's kills in its colour, the local player's team first and
   marked, and the key that opens the team panel. Also what the
   announcer says about teams (ui/announcer/): a card naming the player's
   team when the team duel starts or they join one, a toast when they
   change sides or are moved, and the rounds each team won, which the
   round's "Fight!" card lists. */

// NOTE(zoubir): the plate at X, Y; returns its bottom, Y when no team
// duel is on
internal float
DrawTeamPlate(render_context *RenderContext, app_state *AppState, float X, float Y)
{
    u32 Mine = LocalTeam(AppState);
    if (Mine == Team_None)
    {
        return Y;
    }
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    u32 Order[2] = {Mine, OtherTeam(Mine)};
    char Texts[2][32];
    float Width = 2.f * UI_GAP;
    for(u32 Index = 0; Index < 2; Index++)
    {
        snprintf(Texts[Index], sizeof(Texts[Index]), "%s %u", TeamName(Order[Index]),
                 TeamKills(AppState, Order[Index]));
        Width += 14.f + UITextWidth(Body, Texts[Index]) + UI_GAP_LARGE;
    }
    char Hint[24];
    snprintf(Hint, sizeof(Hint), "M  teams");
    Width += UITextWidth(Small, Hint);
    float Height = UILineHeight(Body) + 2.f * UI_GAP_SMALL + 4.f;
    DrawUIPanel(RenderContext, X, Y, Width, Height, TeamColor(Mine));
    float TextX = X + UI_GAP;
    float TextY = Y + UI_GAP_SMALL + 4.f;
    for(u32 Index = 0; Index < 2; Index++)
    {
        u32 Team = Order[Index];
        float Chip = Index == 0 ? 10.f : 8.f;
        DrawRoundRect(RenderContext, TextX, TextY + 0.5f * (UILineHeight(Body) - Chip), Chip, Chip,
                      TeamColor(Team));
        if (Index == 0)
        {
            DrawRoundOutline(RenderContext, TextX - 2.f,
                             TextY + 0.5f * (UILineHeight(Body) - Chip) - 2.f,
                             Chip + 4.f, Chip + 4.f, UI_COLOR_ACCENT);
        }
        TextX += 14.f;
        UIText(RenderContext, Body, TextX, TextY, Texts[Index], TeamStrongColor(Team));
        TextX += UITextWidth(Body, Texts[Index]) + UI_GAP_LARGE;
    }
    UIText(RenderContext, Small, TextX, TextY + UILineHeight(Body) - UILineHeight(Small) - 2.f,
           Hint, UI_COLOR_TEXT_MUTED);
    return Y + Height;
}

// NOTE(zoubir): what a team duel asks of the players, for the cards
inline char *
TeamDuelGoal(app_state *AppState)
{
    char *Result = IsRoundMap(AppState) ? (char *)"Last team standing wins the round" :
        (char *)"Fight the other team and the monsters";
    return Result;
}

// NOTE(zoubir): once a frame: tells the local player about its team and
// counts the rounds each team wins. A team duel that starts on a new map
// is named by the map's title card (match_watch.cpp); one that starts on
// the same map, after a vote, gets its own card
internal void
WatchTeams(app_state *AppState)
{
    team_ui *Ui = GetTeamUi(AppState);
    announcer *Announcer = GetAnnouncer(AppState);
    u32 Mine = LocalTeam(AppState);
    bool32 Teams = IsTeamDuel(AppState);
    bool32 TitleComing = !Announcer->Started || Announcer->TitleDue > 0.f ||
        Announcer->MapId != AppState->World.MapId;
    if (!Teams)
    {
        if (Ui->TeamDuelAnnounced && !TitleComing)
        {
            PushToast(AppState, AnnounceIcon_Swords, UI_COLOR_ACCENT,
                      (char *)"Back to free for all");
        }
        Ui->TeamDuelAnnounced = false;
    }
    else if (!Ui->TeamDuelAnnounced && Mine != Team_None)
    {
        Ui->TeamDuelAnnounced = true;
        if (!TitleComing)
        {
            char Title[48];
            snprintf(Title, sizeof(Title), "You're on %s", TeamName(Mine));
            announce_card Card = MakeCard(AnnounceStyle_Title, AnnouncePriority_Event,
                                          AnnounceIcon_Swords, TeamStrongColor(Mine),
                                          (char *)"TEAM DUEL", Title, TeamDuelGoal(AppState));
            Card.Sound = AssetType_SfxAnnounce;
            PushCard(AppState, Card);
        }
    }
    if (!Teams || Ui->RoundMapId != AppState->World.MapId || !Ui->SeenTeamDuel)
    {
        ZeroArray(Ui->RoundWins, Team_Count, u32);
        Ui->RoundMapId = AppState->World.MapId;
    }
    if (Mine != Team_None && Ui->SeenTeam != Team_None && Mine != Ui->SeenTeam)
    {
        char Text[64];
        snprintf(Text, sizeof(Text), "You're on %s now", TeamName(Mine));
        PushToast(AppState, AnnounceIcon_Joined, TeamStrongColor(Mine), Text);
    }
    Ui->SeenTeam = Mine;
    Ui->SeenTeamDuel = Teams;

    // NOTE(zoubir): the team still standing as the final blow plays won
    // the round (ui/final_blow_view.cpp)
    bool32 FinalBlow = FinalBlowLeft(AppState) > 0.f;
    if (Teams && FinalBlow && !Ui->FinalBlowSeen)
    {
        u32 Winner = FinalBlowWinner(AppState);
        u32 Team = Winner < MAX_PLAYERS ? PlayerTeam(AppState, Winner) : Team_None;
        if (Team != Team_None)
        {
            Ui->RoundWins[Team]++;
        }
    }
    Ui->FinalBlowSeen = FinalBlow;
}

// NOTE(zoubir): "Red 2  -  Blue 1", the local player's team first; empty
// outside a team duel or before a team has won a round
internal void
TeamRoundWinsLine(app_state *AppState, char *Out, u32 Size)
{
    team_ui *Ui = GetTeamUi(AppState);
    u32 Mine = LocalTeam(AppState);
    Out[0] = 0;
    if (Mine != Team_None && (Ui->RoundWins[Team_Red] || Ui->RoundWins[Team_Blue]))
    {
        snprintf(Out, Size, "%s %u  -  %s %u", TeamName(Mine), Ui->RoundWins[Mine],
                 TeamName(OtherTeam(Mine)), Ui->RoundWins[OtherTeam(Mine)]);
    }
}
