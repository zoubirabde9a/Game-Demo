/* Chat view: the chat's lines (chat.cpp) at the bottom left, newest at the
   bottom, as "Name: text" with the name in a colour (gold for your own),
   wrapped to the room left of the ability bar. A line shows for
   CHAT_SHOW_SECONDS and fades over the last second; while the line to
   type in is open, every kept line shows on a glass panel above it. In a
   dungeon the whole chat sits above the party frames. */

#define CHAT_SHOW_SECONDS 12.f
#define CHAT_FADE_SECONDS 1.f
#define CHAT_MAX_WIDTH 460.f
#define CHAT_MIN_WIDTH 240.f
#define CHAT_WRAP_LINES 4 // one line of chat takes at most this many rows

// NOTE(zoubir): where each row of Text starts when it is wrapped at spaces
// to Width, the first row FirstWidth wide (it follows the name); returns
// the number of rows, at most CHAT_WRAP_LINES (the last one may run long)
internal u32
WrapChatText(font *Font, char *Text, float FirstWidth, float Width, u32 *Starts)
{
    u32 Count = 0;
    u32 At = 0;
    while (Count < CHAT_WRAP_LINES)
    {
        Starts[Count] = At;
        float Room = Count == 0 ? FirstWidth : Width;
        float Used = 0.f;
        u32 LastSpace = 0;
        u32 End = At;
        while (Text[End])
        {
            float Next = Used + GetCharacterWidth(Font, Text[End]);
            if (Next > Room && End > At) break;
            if (Text[End] == ' ') LastSpace = End;
            Used = Next;
            ++End;
        }
        ++Count;
        if (!Text[End]) break;
        At = (LastSpace > At) ? LastSpace + 1 : End;
    }
    return Count;
}

internal void
DrawChat(render_context *RenderContext, app_state *AppState, u32 WindowWidth, u32 WindowHeight)
{
    chat *Chat = AppState->Chat;
    font *Font = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    if (!Chat || !Font || ConnectScreenTakesInput(AppState) || (!Chat->Typing && !Chat->LogCount))
    {
        return;
    }
    player_slot *LocalSlot = &AppState->Players[AppState->LocalPlayerIndex];
    float BarWidth = Maximum(AbilityBarWidth(AppState, LocalSlot), 5.f * ABILITY_SLOT_SIZE);
    float Room = 0.5f * ((float)WindowWidth - BarWidth) - 2.f * UI_GAP_LARGE - ABILITY_PLATE_PAD;
    float Width = Minimum(Maximum(Room, CHAT_MIN_WIDTH), CHAT_MAX_WIDTH);
    float Left = UI_GAP_LARGE;
    float LineHeight = UILineHeight(Font);
    // NOTE(zoubir): above the controls hint in the corner (controls_panel.cpp),
    // and above the party frames in a dungeon, which would cover it
    float Bottom = (float)WindowHeight - UI_GAP_LARGE - UILineHeight(Small) - UI_GAP_SMALL;
    Bottom = Minimum(Bottom, PartyFramesTop(AppState, WindowHeight) - UI_GAP_LARGE);

    char Draft[NET_CHAT_SIZE + 8];
    float BoxHeight = LineHeight + 2.f * UI_GAP_SMALL;
    if (Chat->Typing)
    {
        // NOTE(zoubir): the log's panel first, so the box and text sit on it
        float LogHeight = 0.f;
        for (u32 Index = 0; Index < Chat->LogCount; ++Index)
        {
            chat_entry *Entry = &Chat->Log[Index];
            char Name[NET_NAME_SIZE + 2];
            snprintf(Name, sizeof(Name), "%s: ", Entry->Name);
            u32 Starts[CHAT_WRAP_LINES];
            float Inner = Width - 2.f * UI_GAP_SMALL;
            LogHeight += LineHeight * WrapChatText(Font, Entry->Text, Inner - UITextWidth(Font, Name),
                                                   Inner, Starts);
        }
        float PanelTop = Bottom - BoxHeight - (LogHeight > 0.f ? LogHeight + UI_GAP_SMALL : 0.f);
        DrawUIPanel(RenderContext, Left - UI_GAP_SMALL, PanelTop - UI_GAP_SMALL,
                    Width + 2.f * UI_GAP_SMALL, Bottom - PanelTop + 2.f * UI_GAP_SMALL);

        u32 Edge = Chat->Refused > 0.f ? UI_COLOR_HEALTH : UI_COLOR_ACCENT;
        DrawRoundRect(RenderContext, Left, Bottom - BoxHeight, Width, BoxHeight, UI_COLOR_FIELD);
        DrawRoundOutline(RenderContext, Left, Bottom - BoxHeight, Width, BoxHeight, Edge);
        // NOTE(zoubir): a long draft shows its end, where the caret is
        float TextLeft = Left + UI_GAP_SMALL;
        float TextRoom = Width - 2.f * UI_GAP_SMALL - 4.f;
        char *Shown = Chat->Draft;
        while (*Shown && UITextWidth(Font, Shown) > TextRoom) ++Shown;
        snprintf(Draft, sizeof(Draft), "%s", Shown);
        float TextY = Bottom - BoxHeight + UI_GAP_SMALL;
        if (Chat->DraftCount == 0)
        {
            UIText(RenderContext, Font, TextLeft, TextY, (char *)"Say something to everyone",
                   WithAlpha(UI_COLOR_TEXT_MUTED, 0.7f));
        }
        UIText(RenderContext, Font, TextLeft, TextY, Draft, UI_COLOR_TEXT);
        float Breath = 0.55f + 0.45f * cosf(5.f * Chat->TypingFor);
        DrawFilledRectangle(RenderContext, TextLeft + UITextWidth(Font, Draft) + 1.f, TextY + 2.f,
                            2.f, LineHeight - 4.f, WithAlpha(UI_COLOR_ACCENT, Breath), 0.f);
        Bottom -= BoxHeight + UI_GAP_SMALL;
    }

    // NOTE(zoubir): newest at the bottom, so drawn from the bottom up
    float Inner = Width - (Chat->Typing ? 2.f * UI_GAP_SMALL : 0.f);
    float TextLeft = Left + (Chat->Typing ? UI_GAP_SMALL : 0.f);
    for (u32 Index = 0; Index < Chat->LogCount; ++Index)
    {
        chat_entry *Entry = &Chat->Log[Index];
        float Alpha = Chat->Typing ? 1.f :
            Clamp01((CHAT_SHOW_SECONDS - Entry->Age) / CHAT_FADE_SECONDS);
        if (Alpha <= 0.f) break;
        char Name[NET_NAME_SIZE + 2];
        snprintf(Name, sizeof(Name), "%s: ", Entry->Name);
        float NameWidth = UITextWidth(Font, Name);
        u32 Starts[CHAT_WRAP_LINES];
        u32 Rows = WrapChatText(Font, Entry->Text, Inner - NameWidth, Inner, Starts);
        Bottom -= LineHeight * (float)Rows;
        bool32 Mine = Entry->Slot == AppState->LocalPlayerIndex;
        u32 NameColor = Mine ? UI_COLOR_ACCENT : UI_RGBA(132, 196, 255, 255);
        UIText(RenderContext, Font, TextLeft, Bottom, Name, WithAlpha(NameColor, Alpha));
        for (u32 Row = 0; Row < Rows; ++Row)
        {
            char Piece[NET_CHAT_SIZE];
            u32 End = (Row + 1 < Rows) ? Starts[Row + 1] : (u32)strlen(Entry->Text);
            u32 Length = End - Starts[Row];
            memcpy(Piece, Entry->Text + Starts[Row], Length);
            Piece[Length] = 0;
            float X = TextLeft + (Row == 0 ? NameWidth : 0.f);
            UIText(RenderContext, Font, X, Bottom + LineHeight * (float)Row, Piece,
                   WithAlpha(UI_COLOR_TEXT, Alpha));
        }
    }
}
