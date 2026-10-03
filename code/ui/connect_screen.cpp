/* Connect screen: pick the map for offline play, or type the server
   address and your name, connect, watch the connection state, retry, or
   play offline. F4 opens and closes it;
   it also opens at launch when no server is configured. While it is open
   the keyboard and mouse belong to it, not to the player
   (ConnectScreenTakesInput). A connect from here is saved to server.txt
   (client/online.cpp). */

struct connect_screen
{
    bool32 Open;
    // NOTE(zoubir): a connect from this screen is in progress; the screen
    // closes itself once it succeeds
    bool32 WaitingToJoin;
    ui_state Address;
    ui_state Name;
    ui_state LeftButton;
    ui_state RightButton;
    ui_state MapButtons[MapId_Count];
};


internal void
SetEditBoxText(ui_state *EditBox, char *Text)
{
    CopyString(EditBox->Text, sizeof(EditBox->Text), Text);
    EditBox->TextCount = 0;
    while (EditBox->Text[EditBox->TextCount])
    {
        EditBox->TextCount++;
    }
}

internal void
OpenConnectScreen(connect_screen *Screen, online_session *Online,
                  ui_context *UIContext)
{
    Screen->Open = true;
    SetEditBoxText(&Screen->Address, Online->AddressText);
    SetEditBoxText(&Screen->Name, Online->NameText);
    UISelectEditBox(UIContext, &Screen->Address);
}

// NOTE(zoubir): made on first use; opens at once when there is no server
// to join (never in the browser, which cannot)
internal connect_screen *
GetConnectScreen(app_state *AppState)
{
    if (!AppState->ConnectScreen)
    {
        connect_screen *Screen =
            AllocateStruct(&AppState->MemoryArena, connect_screen);
        *Screen = {};
#if !COMPILER_EMSCRIPTEN
        if (AppState->Online && !AppState->Online->Enabled)
        {
            OpenConnectScreen(Screen, AppState->Online, AppState->UIContext);
        }
#endif
        AppState->ConnectScreen = Screen;
    }
    return AppState->ConnectScreen;
}

inline bool32
ConnectScreenTakesInput(app_state *AppState)
{
    bool32 Result = AppState->ConnectScreen && AppState->ConnectScreen->Open;
    return Result;
}

internal void
DoConnectScreen(render_context *RenderContext, app_state *AppState,
                ui_context *UIContext, app_input *Input,
                u32 WindowWidth, u32 WindowHeight)
{
    online_session *Online = AppState->Online;
    font *Font = AppState->DefaultFont;
    if (!Online || !Font)
    {
        return;
    }
    connect_screen *Screen = GetConnectScreen(AppState);
#if !COMPILER_EMSCRIPTEN
    if (Input->ButtonF4.Pressed)
    {
        if (Screen->Open)
        {
            Screen->Open = false;
        }
        else
        {
            OpenConnectScreen(Screen, Online, UIContext);
        }
    }
#endif
    if (Screen->WaitingToJoin && IsOnline(Online))
    {
        Screen->WaitingToJoin = false;
        Screen->Open = false;
    }
    if (!Screen->Open)
    {
        return;
    }

    // NOTE(zoubir): laid out top to bottom with a running Y, inside the
    // panel. The map grid is two buttons wide
    float Width = 380.f;
    float Pad = UI_GAP_LARGE;
    float FieldWidth = Width - 2.f * Pad;
    float RowHeight = UI_ROW_HEIGHT;
    float LabelHeight = UILineHeight(Font) + UI_GAP_SMALL;
    float MapRowHeight = 34.f;
    u32 MapRows = (MapId_Count + 1) / 2;
    float Height = Pad + UILineHeight(AppState->Fonts.Title) + UI_GAP +
        LabelHeight + MapRows * (MapRowHeight + UI_GAP_SMALL) + UI_GAP_LARGE +
        2.f * (LabelHeight + RowHeight + UI_GAP) +
        LabelHeight + UI_GAP + RowHeight + Pad;
    float Left = 0.5f * ((float)WindowWidth - Width);
    float Top = 0.5f * ((float)WindowHeight - Height);
    DrawFilledRectangle(RenderContext, Left, Top, Width, Height,
                        UI_COLOR_PANEL, 0.f);
    DrawRectangle(RenderContext, Left, Top, Width, Height, UI_COLOR_BORDER, 0.f);

    float Y = Pad;
    UIText(RenderContext, AppState->Fonts.Title, Left + Pad, Top + Y, "Play",
           UI_COLOR_TEXT);
    UIText(RenderContext, AppState->Fonts.Small, Left + Width - Pad,
           Top + Y + 8.f, "F4 to close", UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Y += UILineHeight(AppState->Fonts.Title) + UI_GAP;

    bool32 PlayingOnline = IsOnline(Online);
    UIText(RenderContext, Font, Left + Pad, Top + Y,
           PlayingOnline ? (char *)"Map (the server picks it online)" :
           (char *)"Map", UI_COLOR_TEXT_MUTED);
    Y += LabelHeight;
    float MapsTop = Y;
    Y += MapRows * (MapRowHeight + UI_GAP_SMALL) + UI_GAP_LARGE;

    UIText(RenderContext, Font, Left + Pad, Top + Y,
           "Server address, a.b.c.d:port", UI_COLOR_TEXT_MUTED);
    Y += LabelHeight;
    float AddressY = Y;
    Y += RowHeight + UI_GAP;
    UIText(RenderContext, Font, Left + Pad, Top + Y, "Your name",
           UI_COLOR_TEXT_MUTED);
    Y += LabelHeight;
    float NameY = Y;
    Y += RowHeight + UI_GAP;

    char Status[128];
    GetOnlineStatusText(Online, Status, sizeof(Status));
    UIText(RenderContext, Font, Left + Pad, Top + Y,
           Status[0] ? Status : (char *)"Playing offline", UI_COLOR_TEXT);
    Y += LabelHeight + UI_GAP;

    BeginContainer(UIContext, Left, Top, Width, Height);
    float HalfWidth = 0.5f * (FieldWidth - UI_GAP_SMALL);
    for(u32 MapIndex = 0; MapIndex < MapId_Count; MapIndex++)
    {
        float MapX = Pad + (MapIndex % 2) * (HalfWidth + UI_GAP_SMALL);
        float MapY = MapsTop + (MapIndex / 2) * (MapRowHeight + UI_GAP_SMALL);
        bool32 Current = (AppState->World.MapId == MapIndex);
        if (DoButton(&Screen->MapButtons[MapIndex], AppState, UIContext,
                     MapX, MapY, HalfWidth, MapRowHeight,
                     GetMapDef((map_id)MapIndex)->Name, Current) &&
            !PlayingOnline && !Current)
        {
            StartOfflineMap(AppState, MapIndex);
        }
    }

    DoEditBox(&Screen->Address, AppState, UIContext, Pad, AddressY,
              FieldWidth, RowHeight, sizeof(Online->AddressText) - 1);
    DoEditBox(&Screen->Name, AppState, UIContext, Pad, NameY,
              FieldWidth, RowHeight, NET_NAME_SIZE - 1);

    online_phase Phase = GetOnlinePhase(Online);
    bool32 Joined = (Phase == OnlinePhase_Joined);
    bool32 Trying = (Phase == OnlinePhase_Joining);
    bool32 Ended = (Phase == OnlinePhase_Ended);
    float ButtonWidth = HalfWidth;
    float ButtonY = Y;
    char *LeftText = (char *)(Trying ? "Cancel" : (Ended ? "Retry" : "Connect"));
    if (DoButton(&Screen->LeftButton, AppState, UIContext, Pad, ButtonY,
                 ButtonWidth, RowHeight, LeftText))
    {
        if (Trying)
        {
            OnlineDisconnect(Online);
            Screen->WaitingToJoin = false;
        }
        else if (OnlineConnect(Online, Screen->Address.Text, Screen->Name.Text))
        {
            SaveOnlineConfig(Online->AddressText, Online->NameText);
            Screen->WaitingToJoin = true;
        }
    }
    char *RightText = (char *)(Joined ? "Back to game" : "Play offline");
    if (DoButton(&Screen->RightButton, AppState, UIContext,
                 Width - Pad - ButtonWidth, ButtonY, ButtonWidth, RowHeight,
                 RightText))
    {
        if (!Joined)
        {
            OnlineDisconnect(Online);
        }
        Screen->WaitingToJoin = false;
        Screen->Open = false;
    }
    EndContainer(UIContext);
}
