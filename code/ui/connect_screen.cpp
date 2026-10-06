/* Connect screen: the server list with each server's name, players and
   ping (client/server_browser.cpp); click one to join it. Below it your
   name, a field for an address that is not in the list (a DNS name or
   a.b.c.d:port), the map for offline play, and the connection state.
   F4 opens and closes it; it also opens at launch when no server is
   configured. While it is open the keyboard and mouse belong to it, not
   to the player (ConnectScreenTakesInput). A connect from here is saved
   to server.txt (client/online_config.cpp). */

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
    ui_state JoinButton;
    server_browser Browser;
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
    UISelectEditBox(UIContext, &Screen->Name);
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

// NOTE(zoubir): joins Address from the screen and remembers it for the
// next launch
internal void
ConnectScreenJoin(connect_screen *Screen, online_session *Online, char *Address)
{
    if (OnlineConnect(Online, Address, Screen->Name.Text))
    {
        SaveOnlineConfig(Online->AddressText, Online->NameText);
        Screen->WaitingToJoin = true;
    }
}

#define CONNECT_SERVER_ROW_HEIGHT 54.f

// NOTE(zoubir): one server in the list: signal bars, its name, where it is
// and who plays, and the ping or why it cannot be joined. True when clicked
internal bool32
DrawServerRow(render_context *RenderContext, app_state *AppState, app_input *Input,
              server_entry *Entry, server_status *Status, bool32 Current,
              float X, float Y, float Width)
{
    float Height = CONNECT_SERVER_ROW_HEIGHT;
    bool32 Hot = IsMouseOnRectangle(Input->MouseX, Input->MouseY, X, Y, Width, Height);
    bool32 Joinable = (Status->Reach == ServerReach_Up ||
                       Status->Reach == ServerReach_Unknown);
    DrawRoundRect(RenderContext, X, Y, Width, Height,
                  Hot ? UI_COLOR_CONTROL_HOT : UI_COLOR_CONTROL);
    if (Current || Hot)
    {
        DrawRoundOutline(RenderContext, X, Y, Width, Height,
                         Current ? UI_COLOR_ACCENT : UI_COLOR_TEXT_MUTED);
    }

    u32 Bars = 0;
    u32 BarColor = UI_COLOR_HEALTH;
    if (Status->Reach == ServerReach_Up)
    {
        Bars = Status->PingMs < CONNECTION_GOOD_MS ? 3 :
            (Status->PingMs < CONNECTION_SLOW_MS ? 2 : 1);
        BarColor = Bars == 3 ? UI_COLOR_GOOD : (Bars == 2 ? UI_COLOR_ACCENT : UI_COLOR_HEALTH);
    }
    float BarsX = X + 14.f;
    float BarsBottom = Y + 0.5f * Height + 9.f;
    for(u32 Index = 0; Index < 3; Index++)
    {
        float BarHeight = 6.f + 6.f * (float)Index;
        DrawRoundRect(RenderContext, BarsX + (float)Index * 7.f, BarsBottom - BarHeight,
                      5.f, BarHeight, Index < Bars ? BarColor : UI_RGBA(0, 0, 0, 140));
    }

    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float TextX = BarsX + 34.f;
    char *Name = Status->Name[0] ? Status->Name : Entry->Name;
    UIText(RenderContext, Body, TextX, Y + 7.f, Name, UI_COLOR_TEXT);
    char Detail[96];
    if (Status->Reach == ServerReach_Up || Status->Reach == ServerReach_OtherVersion)
    {
        char *Map = Status->MapId < MapId_Count ?
            GetMapDef((map_id)Status->MapId)->Name : (char *)"?";
        snprintf(Detail, sizeof(Detail), "%s  -  %s  -  %u/%u players", Entry->Region, Map,
                 Status->Players, Status->MaxPlayers);
    }
    else
    {
        snprintf(Detail, sizeof(Detail), "%s  -  %s", Entry->Region, Entry->Address);
    }
    UIText(RenderContext, Small, TextX, Y + 9.f + UILineHeight(Body), Detail,
           UI_COLOR_TEXT_MUTED);

    char *Right = (char *)"asking";
    char Ping[16];
    u32 RightColor = UI_COLOR_TEXT_MUTED;
    switch (Status->Reach)
    {
        case ServerReach_Up:
        {
            snprintf(Ping, sizeof(Ping), "%d ms", (int)(Status->PingMs + 0.5f));
            Right = Ping;
            RightColor = BarColor;
        } break;
        case ServerReach_OtherVersion:
        {
            Right = (char *)"other version";
            RightColor = UI_COLOR_HEALTH;
        } break;
        case ServerReach_Silent:
        {
            Right = (char *)"not answering";
            RightColor = UI_COLOR_HEALTH;
        } break;
        case ServerReach_BadAddress:
        {
            Right = (char *)"address not found";
            RightColor = UI_COLOR_HEALTH;
        } break;
        default: break;
    }
    if (Current)
    {
        Right = (char *)(IsOnline(AppState->Online) ? "joined" : "joining");
        RightColor = UI_COLOR_ACCENT;
    }
    UIText(RenderContext, Body, X + Width - 14.f, Y + 0.5f * (Height - UILineHeight(Body)),
           Right, RightColor, UIAlign_Right);

    bool32 Result = Hot && Joinable && !Current && Input->LeftButton.Released;
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
        StopServerBrowser(&Screen->Browser);
        return;
    }
    UpdateServerBrowser(&Screen->Browser, Input->DeltaTime);

    // NOTE(zoubir): laid out top to bottom with a running Y, inside the
    // panel. The map grid is two buttons wide
    float Width = 480.f;
    float Pad = UI_GAP_LARGE;
    float FieldWidth = Width - 2.f * Pad;
    float RowHeight = UI_ROW_HEIGHT;
    float LabelHeight = UILineHeight(Font) + UI_GAP_SMALL;
    float MapRowHeight = 34.f;
    u32 MapRows = (MapId_Count + 1) / 2;
    float ServersHeight = SERVER_LIST_COUNT * (CONNECT_SERVER_ROW_HEIGHT + UI_GAP_SMALL);
    float Height = Pad + UILineHeight(AppState->Fonts.Title) + UI_GAP +
        LabelHeight + ServersHeight + UI_GAP +
        2.f * (LabelHeight + RowHeight + UI_GAP) +
        LabelHeight + MapRows * (MapRowHeight + UI_GAP_SMALL) + UI_GAP +
        LabelHeight + UI_GAP + RowHeight + Pad;
    float Left = 0.5f * ((float)WindowWidth - Width);
    float Top = Maximum(8.f, 0.5f * ((float)WindowHeight - Height));
    DrawUIPanel(RenderContext, Left - 6.f, Top - 6.f, Width + 12.f, Height + 12.f);

    float Y = Pad;
    UIText(RenderContext, AppState->Fonts.Title, Left + Pad, Top + Y, "Play",
           UI_COLOR_TEXT);
    UIText(RenderContext, AppState->Fonts.Small, Left + Width - Pad,
           Top + Y + 8.f, "F4 to close", UI_COLOR_TEXT_MUTED, UIAlign_Right);
    Y += UILineHeight(AppState->Fonts.Title) + UI_GAP;

    UIText(RenderContext, Font, Left + Pad, Top + Y, "Servers: click one to join",
           UI_COLOR_TEXT_MUTED);
    Y += LabelHeight;
    online_phase Phase = GetOnlinePhase(Online);
    bool32 Joined = (Phase == OnlinePhase_Joined);
    bool32 Trying = (Phase == OnlinePhase_Joining);
    for(u32 Index = 0; Index < SERVER_LIST_COUNT; Index++)
    {
        server_entry *Entry = &ServerList[Index];
        bool32 Current = (Joined || Trying) &&
            FindServerByAddress(Online->AddressText) == Entry;
        if (DrawServerRow(RenderContext, AppState, Input, Entry,
                          &Screen->Browser.Servers[Index], Current,
                          Left + Pad, Top + Y, FieldWidth))
        {
            ConnectScreenJoin(Screen, Online, Entry->Address);
        }
        Y += CONNECT_SERVER_ROW_HEIGHT + UI_GAP_SMALL;
    }
    Y += UI_GAP;

    UIText(RenderContext, Font, Left + Pad, Top + Y, "Your name", UI_COLOR_TEXT_MUTED);
    Y += LabelHeight;
    float NameY = Y;
    Y += RowHeight + UI_GAP;
    UIText(RenderContext, Font, Left + Pad, Top + Y,
           "Another server: host name or a.b.c.d:port", UI_COLOR_TEXT_MUTED);
    Y += LabelHeight;
    float AddressY = Y;
    Y += RowHeight + UI_GAP;

    bool32 PlayingOnline = IsOnline(Online);
    UIText(RenderContext, Font, Left + Pad, Top + Y,
           PlayingOnline ? (char *)"Map (the server picks it online)" :
           (char *)"Map for offline play", UI_COLOR_TEXT_MUTED);
    Y += LabelHeight;
    float MapsTop = Y;
    Y += MapRows * (MapRowHeight + UI_GAP_SMALL) + UI_GAP;

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

    DoEditBox(&Screen->Name, AppState, UIContext, Pad, NameY,
              FieldWidth, RowHeight, NET_NAME_SIZE - 1);
    float JoinWidth = 96.f;
    DoEditBox(&Screen->Address, AppState, UIContext, Pad, AddressY,
              FieldWidth - JoinWidth - UI_GAP_SMALL, RowHeight,
              sizeof(Online->AddressText) - 1);
    if (DoButton(&Screen->JoinButton, AppState, UIContext,
                 Width - Pad - JoinWidth, AddressY, JoinWidth, RowHeight, "Join") &&
        Screen->Address.Text[0])
    {
        ConnectScreenJoin(Screen, Online, Screen->Address.Text);
    }

    bool32 Ended = (Phase == OnlinePhase_Ended);
    float ButtonWidth = HalfWidth;
    float ButtonY = Y;
    char *LeftText = (char *)(Trying ? "Cancel" : (Ended ? "Retry" : "Reconnect"));
    if (DoButton(&Screen->LeftButton, AppState, UIContext, Pad, ButtonY,
                 ButtonWidth, RowHeight, LeftText))
    {
        if (Trying)
        {
            OnlineDisconnect(Online);
            Screen->WaitingToJoin = false;
        }
        else
        {
            ConnectScreenJoin(Screen, Online, Online->AddressText[0] ?
                              Online->AddressText : ServerList[0].Address);
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
