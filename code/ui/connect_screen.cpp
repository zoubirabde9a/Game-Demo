/* Connect screen: type the server address and your name, connect, watch
   the connection state, retry, or play offline. F4 opens and closes it;
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
};

#define RGBA8_CONNECT_PANEL (0xE0181818)
#define RGBA8_CONNECT_LABEL (0xFFA0A0A0)

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

    float Width = 380.f;
    float Height = 292.f;
    // NOTE(zoubir): tall enough that descenders (g, y) are not clipped
    float RowHeight = 40.f;
    float Left = 0.5f * ((float)WindowWidth - Width);
    float Top = 0.5f * ((float)WindowHeight - Height);
    float Pad = 20.f;
    float FieldWidth = Width - 2.f * Pad;
    DrawFilledRectangle(RenderContext, Left, Top, Width, Height,
                        RGBA8_CONNECT_PANEL, 0.f);
    DrawRectangle(RenderContext, Left, Top, Width, Height, RGBA8_WHITE, 0.f);

    DrawScreenText(RenderContext, Font, Left + Pad, Top + 14.f,
                   "Play online (F4 to close)", RGBA8_WHITE);
    DrawScreenText(RenderContext, Font, Left + Pad, Top + 46.f,
                   "Server address, a.b.c.d:port", RGBA8_CONNECT_LABEL);
    DrawScreenText(RenderContext, Font, Left + Pad, Top + 118.f,
                   "Your name", RGBA8_CONNECT_LABEL);

    char Status[128];
    GetOnlineStatusText(Online, Status, sizeof(Status));
    DrawScreenText(RenderContext, Font, Left + Pad, Top + 190.f,
                   Status[0] ? Status : "Playing offline", RGBA8_WHITE);

    BeginContainer(UIContext, Left, Top, Width, Height);
    DoEditBox(&Screen->Address, AppState, UIContext, Pad, 66.f,
              FieldWidth, RowHeight, sizeof(Online->AddressText) - 1);
    DoEditBox(&Screen->Name, AppState, UIContext, Pad, 138.f,
              FieldWidth, RowHeight, NET_NAME_SIZE - 1);

    online_phase Phase = GetOnlinePhase(Online);
    bool32 Joined = (Phase == OnlinePhase_Joined);
    bool32 Trying = (Phase == OnlinePhase_Joining);
    bool32 Ended = (Phase == OnlinePhase_Ended);
    float ButtonWidth = 0.5f * (FieldWidth - 10.f);
    float ButtonY = Height - Pad - RowHeight;
    char *LeftText = Trying ? "Cancel" : (Ended ? "Retry" : "Connect");
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
    char *RightText = Joined ? "Back to game" : "Play offline";
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
