/* Chat: while joined to a server, Enter opens a line at the bottom left
   to type in; Enter again says it to everyone on the server (the
   connection's side is net/client_chat.h), and Esc, or Enter on an empty
   line, closes it. While the line is open the keyboard types and does
   nothing else: ChatTakesKeys, read in app.cpp before anything looks at
   the keys, empties the frame's keys, so no action or screen sees them
   and the player stands still. The last CHAT_LOG_SIZE lines are kept for
   chat_view.cpp, which draws them. Offline there is nobody to talk to, so
   Enter does nothing. */

#define CHAT_LOG_SIZE 8

struct chat_entry
{
    u8 Slot;
    char Name[NET_NAME_SIZE];
    char Text[NET_CHAT_SIZE];
    float Age; // seconds since it arrived
};

struct chat
{
    bool32 Typing;
    char Draft[NET_CHAT_SIZE];
    u32 DraftCount;
    // NOTE(zoubir): seconds the line stays red after an Enter that could
    // not go out (four lines still on their way)
    float Refused;
    // NOTE(zoubir): seconds since the line opened, for the caret
    float TypingFor;
    chat_entry Log[CHAT_LOG_SIZE]; // newest first
    u32 LogCount;
};

internal chat *
GetChat(app_state *AppState)
{
    if (!AppState->Chat)
    {
        AppState->Chat = AllocateStruct(&AppState->MemoryArena, chat);
        *AppState->Chat = {};
    }
    return AppState->Chat;
}

internal void
AddChatEntry(chat *Chat, u8 Slot, char *Name, char *Text)
{
    u32 Keep = (Chat->LogCount < CHAT_LOG_SIZE) ? Chat->LogCount : CHAT_LOG_SIZE - 1;
    for (u32 Index = Keep; Index > 0; --Index)
    {
        Chat->Log[Index] = Chat->Log[Index - 1];
    }
    chat_entry *Entry = &Chat->Log[0];
    Entry->Slot = Slot;
    CopyString(Entry->Name, sizeof(Entry->Name), Name);
    CopyString(Entry->Text, sizeof(Entry->Text), Text);
    Entry->Age = 0.f;
    Chat->LogCount = Keep + 1;
}

// NOTE(zoubir): this frame's typed keys onto the draft, as an edit box
// takes them (engine/ui/edit_box.cpp)
internal void
TypeIntoChat(chat *Chat, char *Keys, u32 KeyCount)
{
    for (u32 Index = 0; Index < KeyCount; ++Index)
    {
        char C = Keys[Index];
        if (C == TEXT_KEY_ERASE)
        {
            if (Chat->DraftCount > 0) Chat->DraftCount--;
        }
        else if (C == TEXT_KEY_ERASE_WORD)
        {
            while (Chat->DraftCount > 0 && Chat->Draft[Chat->DraftCount - 1] == ' ') Chat->DraftCount--;
            while (Chat->DraftCount > 0 && Chat->Draft[Chat->DraftCount - 1] != ' ') Chat->DraftCount--;
        }
        else if (Chat->DraftCount + 1 < sizeof(Chat->Draft))
        {
            Chat->Draft[Chat->DraftCount++] = C;
        }
    }
    Chat->Draft[Chat->DraftCount] = 0;
}

// NOTE(zoubir): no key is down, pressed or typed this frame; the mouse
// stays, so the screens it points at still work
internal void
EmptyFrameKeys(app_input *Input)
{
    u8 *First = (u8 *)&Input->ArrowUp;
    u8 *End = (u8 *)(Input->AlphaButtons + ArrayCount(Input->AlphaButtons));
    memset(First, 0, (size_t)(End - First));
    Input->TextInputCount = 0;
    Input->TextInput[0] = 0;
    Input->TextSubmit = false;
}

// NOTE(zoubir): Draft says something when it holds more than spaces
internal bool32
ChatDraftSaysSomething(chat *Chat)
{
    for (u32 Index = 0; Index < Chat->DraftCount; ++Index)
    {
        if (Chat->Draft[Index] != ' ') return true;
    }
    return false;
}

// Once a frame, before anything reads the keys: takes the lines the server
// sent, ages the log, and opens, fills, sends or closes the line being
// typed. Returns true when the keys were the chat's this frame (they are
// then emptied from Input).
internal bool32
ChatTakesKeys(app_state *AppState, app_input *Input)
{
    chat *Chat = GetChat(AppState);
    for (u32 Index = 0; Index < Chat->LogCount; ++Index) Chat->Log[Index].Age += Input->DeltaTime;
    Chat->Refused = Maximum(0.f, Chat->Refused - Input->DeltaTime);
    Chat->TypingFor += Input->DeltaTime;

    bool32 CanChat = false;
#if !COMPILER_EMSCRIPTEN
    online_session *Online = AppState->Online;
    if (IsOnline(Online))
    {
        net_chat_line Line;
        while (NetClientTakeChat(&Online->Client, &Line))
        {
            AddChatEntry(Chat, Line.Slot, Line.Name, Line.Text);
        }
        CanChat = !ConnectScreenTakesInput(AppState) && !AppState->OptionsOpen;
    }
#endif
    if (!CanChat)
    {
        Chat->Typing = false;
        return false;
    }
    if (!Chat->Typing)
    {
        if (!Input->TextSubmit) return false;
        Chat->Typing = true;
        Chat->TypingFor = 0.f;
        Chat->DraftCount = 0;
        Chat->Draft[0] = 0;
        EmptyFrameKeys(Input);
        return true;
    }

    if (Input->EscapeButton.Pressed)
    {
        Chat->Typing = false;
    }
    else
    {
        TypeIntoChat(Chat, Input->TextInput, Input->TextInputCount);
        if (Input->TextSubmit)
        {
            if (!ChatDraftSaysSomething(Chat))
            {
                Chat->Typing = false;
            }
#if !COMPILER_EMSCRIPTEN
            else if (NetClientSay(&Online->Client, Chat->Draft))
            {
                Chat->Typing = false;
            }
#endif
            else
            {
                Chat->Refused = 0.6f;
            }
        }
    }
    EmptyFrameKeys(Input);
    return true;
}
