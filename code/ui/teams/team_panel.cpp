/* Team panel: M opens it in a duel (or the team plate's hint, or the
   Teams button in the Esc menu), M or Esc closes it. In a team duel it
   shows both teams side by side: each team's kills, its players with
   their level, kills and deaths and whether they are still standing, and
   under each team a button to join it. The button says why when a team
   cannot take the player (teams stay even), that a bot will swap out to
   make room, or "Switching" while the server has not said yes yet. A
   line at the bottom says what a switch costs right now: nothing while
   the player is out or between rounds, else sitting out until the round
   ends or a respawn. In a free-for-all duel it offers the vote for a
   team duel instead. The game goes on under it; clicks on it never cast.
   Requests go out through RequestTeam (client/teams/team_net.cpp). */

#define TEAM_PANEL_WIDTH 620.f
#define TEAM_PANEL_ROW 28.f
#define TEAM_PANEL_BUTTON 38.f
// NOTE(zoubir): rows kept for each team, so the panel does not jump in
// height as players come and go
#define TEAM_PANEL_ROWS 4

inline bool32
TeamPanelAllowed(app_state *AppState)
{
    bool32 Result = !IsDungeonMap(AppState->World.MapId) &&
        AppState->Players[AppState->LocalPlayerIndex].Active;
    return Result;
}

internal void
OpenTeamPanel(app_state *AppState)
{
    GetTeamUi(AppState)->PanelOpen = true;
    GetTalentPanel(AppState)->Open = false;
}

// NOTE(zoubir): from CloseTopScreen (options_menu.cpp): Esc closes it
internal bool32
CloseTeamPanel(app_state *AppState)
{
    team_ui *Ui = AppState->TeamUi;
    bool32 Result = Ui && Ui->PanelOpen;
    if (Ui)
    {
        Ui->PanelOpen = false;
    }
    return Result;
}

// NOTE(zoubir): a button that can be greyed out; Accent is the outline
// when lit. True when clicked while enabled
internal bool32
TeamButton(render_context *RenderContext, app_input *Input, float X, float Y, float Width,
           float Height, bool32 Enabled, bool32 Lit, u32 Accent)
{
    bool32 Hot = Enabled && IsMouseOnRectangle(Input->MouseX, Input->MouseY, X, Y, Width, Height);
    if (IsMouseOnRectangle(Input->MouseX, Input->MouseY, X, Y, Width, Height))
    {
        GlobalMouseOnButton = true;
    }
    u32 Fill = Hot ? UI_COLOR_CONTROL_HOT : UI_COLOR_CONTROL;
    DrawRoundRect(RenderContext, X, Y, Width, Height, Enabled || Lit ? Fill : WithAlpha(Fill, 0.45f));
    if (Lit || Hot)
    {
        DrawRoundOutline(RenderContext, X, Y, Width, Height, Lit ? Accent : UI_COLOR_BORDER);
    }
    bool32 Result = Hot && Input->LeftButton.Pressed;
    return Result;
}

// NOTE(zoubir): one player's row: a dot (team colour while standing, grey
// when out), the name (gold for the local player), a bot tag, and level,
// kills and deaths on the right
internal void
DrawTeamPanelRow(render_context *RenderContext, app_state *AppState, u32 SlotIndex,
                 float Left, float Top, float Width)
{
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    player_slot *Slot = &AppState->Players[SlotIndex];
    bool32 IsLocal = SlotIndex == AppState->LocalPlayerIndex;
    bool32 Out = !Slot->Entity || IsDeadPlayer(Slot->Entity);
    float TextTop = Top + 0.5f * (TEAM_PANEL_ROW - UILineHeight(Body));
    if (IsLocal)
    {
        DrawRoundRect(RenderContext, Left - 6.f, Top + 1.f, Width + 12.f, TEAM_PANEL_ROW - 2.f,
                      UI_RGBA(240, 200, 48, 28));
    }
    float Dot = 8.f;
    DrawRoundRect(RenderContext, Left, Top + 0.5f * (TEAM_PANEL_ROW - Dot), Dot, Dot,
                  Out ? UI_COLOR_DIM : TeamColor(Slot->Team));
    char Name[32];
    GetPlayerName(AppState, SlotIndex, Name, sizeof(Name));
    if (IsLocal)
    {
        snprintf(Name, sizeof(Name), "You");
    }
    u32 NameColor = IsLocal ? UI_COLOR_ACCENT : (Out ? UI_COLOR_TEXT_MUTED : UI_COLOR_TEXT);
    float NameX = Left + Dot + UI_GAP_SMALL;
    UIText(RenderContext, Body, NameX, TextTop, Name, NameColor);
    float After = NameX + UITextWidth(Body, Name) + UI_GAP_SMALL;
    if (Slot->Bot)
    {
        UIText(RenderContext, Small, After, TextTop + UILineHeight(Body) - UILineHeight(Small),
               "bot", UI_COLOR_TEXT_MUTED);
    }
    char Stats[32];
    snprintf(Stats, sizeof(Stats), "Lv %u   %u / %u", Slot->Level ? Slot->Level : 1,
             Slot->Kills, Slot->Deaths);
    UIText(RenderContext, Small, Left + Width, TextTop + UILineHeight(Body) - UILineHeight(Small),
           Stats, UI_COLOR_TEXT_MUTED, UIAlign_Right);
}

// NOTE(zoubir): the player rows each column keeps room for, the same for
// both so their buttons line up
internal u32
TeamPanelRows(app_state *AppState)
{
    u32 Result = TEAM_PANEL_ROWS;
    for(u32 Team = Team_Red; Team <= Team_Blue; Team++)
    {
        Result = Maximum(Result, CountTeam(AppState, Team));
    }
    return Result;
}

// NOTE(zoubir): one team's column: its name and kills over a bar in its
// colour, its players, and the button to join it
internal void
DoTeamColumn(render_context *RenderContext, app_state *AppState, app_input *Input,
             u32 Team, float Left, float Top, float Width, u32 Pending)
{
    font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    u32 Color = TeamColor(Team);
    DrawRoundRect(RenderContext, Left, Top, Width, 4.f, Color);
    Top += 4.f + UI_GAP_SMALL;
    UIText(RenderContext, Title, Left, Top, TeamName(Team), TeamStrongColor(Team));
    char Score[32];
    snprintf(Score, sizeof(Score), "%u", TeamKills(AppState, Team));
    float ScoreRight = Left + Width;
    UIText(RenderContext, Title, ScoreRight, Top, Score, UI_COLOR_TEXT, UIAlign_Right);
    UIText(RenderContext, Small, ScoreRight - UITextWidth(Title, Score) - UI_GAP_SMALL,
           Top + UILineHeight(Title) - UILineHeight(Small) - 2.f, "kills", UI_COLOR_TEXT_MUTED,
           UIAlign_Right);
    Top += UILineHeight(Title) + UI_GAP_SMALL;

    float RowsTop = Top;
    u32 Shown = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (PlayerTeam(AppState, SlotIndex) == Team)
        {
            DrawTeamPanelRow(RenderContext, AppState, SlotIndex, Left,
                             RowsTop + (float)Shown * TEAM_PANEL_ROW, Width);
            Shown++;
        }
    }
    if (!Shown)
    {
        UIText(RenderContext, Body, Left, Top + 0.5f * (TEAM_PANEL_ROW - UILineHeight(Body)),
               "Nobody yet", UI_COLOR_TEXT_MUTED);
    }
    Top += (float)TeamPanelRows(AppState) * TEAM_PANEL_ROW + UI_GAP_SMALL;

    u32 Local = AppState->LocalPlayerIndex;
    u32 MakesWay;
    team_refusal Refusal = CanJoinTeam(AppState, Local, Team, &MakesWay);
    bool32 Mine = Refusal == TeamRefusal_Already;
    bool32 Waiting = Pending == Team;
    bool32 Enabled = Refusal == TeamRefusal_None && !Pending;
    char Label[48];
    u32 LabelColor = UI_COLOR_TEXT;
    if (Mine)
    {
        snprintf(Label, sizeof(Label), "Your team");
        LabelColor = TeamStrongColor(Team);
    }
    else if (Waiting)
    {
        snprintf(Label, sizeof(Label), "Switching to %s...", TeamName(Team));
        LabelColor = TeamStrongColor(Team);
    }
    else if (Refusal == TeamRefusal_Uneven)
    {
        snprintf(Label, sizeof(Label), "%s is full", TeamName(Team));
        LabelColor = UI_COLOR_TEXT_MUTED;
    }
    else
    {
        snprintf(Label, sizeof(Label), "Join %s", TeamName(Team));
    }
    if (TeamButton(RenderContext, Input, Left, Top, Width, TEAM_PANEL_BUTTON, Enabled,
                   Mine || Waiting, Color))
    {
        RequestTeam(AppState, Team);
    }
    UIText(RenderContext, Body, Left + 0.5f * Width,
           Top + 0.5f * (TEAM_PANEL_BUTTON - UILineHeight(Body)), Label, LabelColor,
           UIAlign_Center);
    Top += TEAM_PANEL_BUTTON + 2.f;
    char *Note = 0;
    if (!Mine && !Waiting && Refusal == TeamRefusal_None && MakesWay < MAX_PLAYERS)
    {
        Note = (char *)"A bot moves over to make room";
    }
    else if (!Mine && Refusal == TeamRefusal_Uneven)
    {
        Note = (char *)"Teams must stay even. Wait for someone to leave";
    }
    if (Note)
    {
        UIText(RenderContext, Small, Left + 0.5f * Width, Top, Note, UI_COLOR_TEXT_MUTED,
               UIAlign_Center);
    }
}

// NOTE(zoubir): what changing sides costs the local player right now
internal char *
TeamSwitchCost(app_state *AppState)
{
    world_entity *Player = GetLocalPlayer(AppState);
    char *Result;
    if (!Player || IsDeadPlayer(Player))
    {
        Result = (char *)"You're out, so switching is free. You come back on the new side";
    }
    else if (AppState->RoundBreak > 0.f)
    {
        Result = (char *)"Switching is free between rounds. You start the next one on the new side";
    }
    else if (IsRoundMap(AppState))
    {
        Result = (char *)"Switching mid-round takes you out until the round ends";
    }
    else
    {
        Result = (char *)"Switching takes you out of the fight. You respawn on the new side";
    }
    return Result;
}

internal float
TeamPanelHeight(app_state *AppState)
{
    font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float Column = 4.f + UI_GAP_SMALL + UILineHeight(Title) + UI_GAP_SMALL;
    Column += (float)TeamPanelRows(AppState) * TEAM_PANEL_ROW + UI_GAP_SMALL + TEAM_PANEL_BUTTON + 2.f +
        UILineHeight(Small);
    float Result = 2.f * UI_GAP_LARGE + UILineHeight(Title) + UILineHeight(Small) + UI_GAP +
        Column + UI_GAP + UILineHeight(Small);
    return Result;
}

// NOTE(zoubir): in a free-for-all duel: what a team duel is and the
// button that asks everyone for one
internal void
DoTeamDuelOffer(render_context *RenderContext, app_state *AppState, app_input *Input,
                float Left, float Top, float Inner)
{
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Body, Left, Top, "This duel is free for all.", UI_COLOR_TEXT);
    Top += UILineHeight(Body);
    UIText(RenderContext, Small, Left, Top,
           "In a team duel, Red fights Blue and teammates cannot hurt each other.",
           UI_COLOR_TEXT_MUTED);
    Top += UILineHeight(Small) + UI_GAP;
    bool32 Asked = AppState->VoteOpen && AppState->VoteTeams;
    bool32 Enabled = !AppState->VoteOpen && !AppState->NextMapVoted;
    if (TeamButton(RenderContext, Input, Left, Top, Inner, TEAM_PANEL_BUTTON, Enabled, Asked,
                   UI_COLOR_ACCENT))
    {
        RequestVote(AppState, MapVote_Teams);
    }
    char *Label = Asked ? (char *)"Team duel vote open" :
        AppState->VoteOpen ? (char *)"Another vote is open" :
        CountActivePlayers(AppState) > 1 ? (char *)"Ask for a team duel" : (char *)"Start a team duel";
    UIText(RenderContext, Body, Left + 0.5f * Inner,
           Top + 0.5f * (TEAM_PANEL_BUTTON - UILineHeight(Body)), Label,
           Enabled || Asked ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED, UIAlign_Center);
}

// NOTE(zoubir): team_hud.cpp, included after the announcer it talks to
internal void WatchTeams(app_state *AppState);

internal void
DoTeamPanel(render_context *RenderContext, app_state *AppState, app_input *Input,
            u32 WindowWidth, u32 WindowHeight)
{
    WatchTeams(AppState);
    team_ui *Ui = GetTeamUi(AppState);
    u32 Pending = PendingTeam(AppState, Input->DeltaTime);
    bool32 KeysToUi = ConnectScreenTakesInput(AppState) || AppState->OptionsOpen;
    if (!TeamPanelAllowed(AppState) || KeysToUi)
    {
        Ui->PanelOpen = Ui->PanelOpen && TeamPanelAllowed(AppState);
        return;
    }
    // NOTE(zoubir): a player who put an action on M keeps it
    if (Input->ButtonM.Pressed && !KeyCodeBoundNow(LetterKeyCode('M')))
    {
        if (Ui->PanelOpen)
        {
            CloseTeamPanel(AppState);
        }
        else
        {
            OpenTeamPanel(AppState);
        }
    }
    if (!Ui->PanelOpen)
    {
        return;
    }

    font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    bool32 Teams = IsTeamDuel(AppState);
    float Width = Minimum(TEAM_PANEL_WIDTH, (float)WindowWidth - 2.f * UI_GAP);
    float Height = Teams ? TeamPanelHeight(AppState) :
        2.f * UI_GAP_LARGE + UILineHeight(Title) + UI_GAP + UILineHeight(AppState->Fonts.Body) +
        UILineHeight(Small) + UI_GAP + TEAM_PANEL_BUTTON;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = Maximum(UI_GAP, 0.42f * ((float)WindowHeight - Height));
    if (IsMouseOnRectangle(Input->MouseX, Input->MouseY, X, Y, Width, Height))
    {
        GlobalMouseOnButton = true;
    }
    DrawUIPanel(RenderContext, X, Y, Width, Height);
    float Left = X + UI_GAP_LARGE;
    float Inner = Width - 2.f * UI_GAP_LARGE;
    float Top = Y + UI_GAP_LARGE;
    UIText(RenderContext, Title, Left, Top, Teams ? (char *)"Teams" : (char *)"Team duel",
           UI_COLOR_TEXT);
    UIText(RenderContext, Small, Left + Inner, Top + UILineHeight(Title) - UILineHeight(Small),
           "M or Esc to close", UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Top += UILineHeight(Title);
    if (!Teams)
    {
        DoTeamDuelOffer(RenderContext, AppState, Input, Left, Top + UI_GAP, Inner);
        return;
    }
    char *Goal = IsRoundMap(AppState) ?
        (char *)"Last team standing wins the round. Teammates cannot hurt each other." :
        (char *)"Kills count for your team. Teammates cannot hurt each other.";
    UIText(RenderContext, Small, Left, Top, Goal, UI_COLOR_TEXT_MUTED);
    Top += UILineHeight(Small) + UI_GAP;

    float ColumnWidth = 0.5f * (Inner - UI_GAP_LARGE);
    DoTeamColumn(RenderContext, AppState, Input, Team_Red, Left, Top, ColumnWidth, Pending);
    DoTeamColumn(RenderContext, AppState, Input, Team_Blue, Left + ColumnWidth + UI_GAP_LARGE,
                 Top, ColumnWidth, Pending);
    UIText(RenderContext, Small, Left + 0.5f * Inner, Y + Height - UI_GAP_LARGE - UILineHeight(Small),
           TeamSwitchCost(AppState), UI_COLOR_TEXT_MUTED, UIAlign_Center);
}
