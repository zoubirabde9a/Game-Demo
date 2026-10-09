/* Browser input: keyboard and mouse events from the page, turned into
   the same app_input the Windows layer makes. Events only record which
   keys are down (and which went down since the last frame, so a tap
   shorter than a frame still counts); WebReadInput then sets each
   button's Pressed and Released against the frame before.

   Keys follow the Windows layer: letters by the keyboard layout (the key
   marked A is A on AZERTY too), digits by their place on the top row.
   Typed text comes from the same key events: printable characters,
   Backspace (Ctrl+Backspace erases a word), Enter as a submit, and
   Ctrl+V through the page's paste event. The page keeps the keys the
   game uses (Tab, Backspace, the arrows...) from also scrolling or
   leaving the page; Ctrl combinations, F5 and F12 still reach the
   browser. */

// NOTE(zoubir): every button in app_input, in one order, so events and
// frames can index them
#define WEB_BUTTON_COUNT (5 + 4 + 7 + 12 + 10 + 26)

internal app_button_state *
WebButton(app_input *Input, u32 Index)
{
    app_button_state *Simple[] =
    {
        &Input->ArrowUp, &Input->ArrowDown, &Input->ArrowRight, &Input->ArrowLeft,
        &Input->CapsButton, &Input->BackspaceButton, &Input->SpaceButton,
        &Input->ShiftButton, &Input->AltButton, &Input->TabButton, &Input->EscapeButton,
    };
    if (Index < 5) return &Input->mouseButtons[Index];
    Index -= 5;
    if (Index < ArrayCount(Simple)) return Simple[Index];
    Index -= ArrayCount(Simple);
    if (Index < 12) return &Input->FButtons[Index];
    Index -= 12;
    if (Index < 10) return &Input->NumbersButtons[Index];
    Index -= 10;
    return &Input->AlphaButtons[Index];
}

#define WEB_KEY_ARROWS 5
#define WEB_KEY_F1 (5 + 4 + 7)
#define WEB_KEY_DIGITS (WEB_KEY_F1 + 12)
#define WEB_KEY_LETTERS (WEB_KEY_DIGITS + 10)

struct web_input
{
    bool32 Down[WEB_BUTTON_COUNT];
    bool32 WentDown[WEB_BUTTON_COUNT]; // since the last frame
    bool32 TouchDown[WEB_BUTTON_COUNT]; // held by the touch controls (touch.cpp)
    i32 MouseX, MouseY;                // game pixels from the canvas's top left
    float Scale;                       // game pixels per CSS pixel, 0 until the first frame
    char Typed[64];
    u32 TypedCount;
    bool32 Submit;
};

global_variable web_input WebInput;

internal bool32
WebNameIs(const char *A, const char *B)
{
    return strcmp(A, B) == 0;
}

// NOTE(zoubir): the button index for a key event, -1 for a key the game
// does not read
internal int
WebKeyIndex(const EmscriptenKeyboardEvent *Event)
{
    const char *Key = Event->key;
    const char *Code = Event->code;
    if (Key[0] && !Key[1])
    {
        char C = Key[0];
        if (C >= 'a' && C <= 'z') return WEB_KEY_LETTERS + (C - 'a');
        if (C >= 'A' && C <= 'Z') return WEB_KEY_LETTERS + (C - 'A');
    }
    if (strncmp(Code, "Key", 3) == 0 && Code[3] >= 'A' && Code[3] <= 'Z' && !Code[4])
        return WEB_KEY_LETTERS + (Code[3] - 'A');
    if (strncmp(Code, "Digit", 5) == 0 && Code[5] >= '0' && Code[5] <= '9' && !Code[6])
        return WEB_KEY_DIGITS + (Code[5] - '0');
    if (Code[0] == 'F' && Code[1] >= '1' && Code[1] <= '9')
    {
        int Number = atoi(Code + 1);
        if (Number >= 1 && Number <= 12) return WEB_KEY_F1 + Number - 1;
    }
    const char *Named[] =
    {
        "ArrowUp", "ArrowDown", "ArrowRight", "ArrowLeft",
        "CapsLock", "Backspace", "Space", "Shift", "Alt", "Tab", "Escape",
    };
    for (u32 Index = 0; Index < ArrayCount(Named); Index++)
    {
        // NOTE(zoubir): ShiftLeft and ShiftRight are both Shift
        size_t Length = strlen(Named[Index]);
        if (strncmp(Code, Named[Index], Length) == 0 &&
            (!Code[Length] || WebNameIs(Code + Length, "Left") || WebNameIs(Code + Length, "Right")))
        {
            return WEB_KEY_ARROWS + (int)Index;
        }
    }
    return -1;
}

internal void
WebAddTyped(char Key)
{
    if (WebInput.TypedCount + 1 < ArrayCount(WebInput.Typed))
    {
        WebInput.Typed[WebInput.TypedCount++] = Key;
    }
}

internal EM_BOOL
WebOnKey(int EventType, const EmscriptenKeyboardEvent *Event, void *UserData)
{
    bool32 IsDown = (EventType == EMSCRIPTEN_EVENT_KEYDOWN);
    int Index = WebKeyIndex(Event);
    if (Index >= 0)
    {
        if (IsDown && !WebInput.Down[Index]) WebInput.WentDown[Index] = true;
        WebInput.Down[Index] = IsDown;
    }
    if (IsDown && !Event->altKey && !Event->metaKey)
    {
        const char *Key = Event->key;
        if (WebNameIs(Key, "Backspace")) WebAddTyped(Event->ctrlKey ? TEXT_KEY_ERASE_WORD : TEXT_KEY_ERASE);
        else if (WebNameIs(Key, "Enter")) WebInput.Submit = true;
        else if (!Event->ctrlKey && Key[0] >= 32 && Key[0] < 127 && !Key[1]) WebAddTyped(Key[0]);
    }
    if (Event->ctrlKey || Event->metaKey) return false;
    bool32 BrowserKey = WebNameIs(Event->key, "F5") || WebNameIs(Event->key, "F11") ||
        WebNameIs(Event->key, "F12");
    return (Index >= 0 || WebNameIs(Event->key, "Enter")) && !BrowserKey;
}

internal EM_BOOL
WebOnMouse(int EventType, const EmscriptenMouseEvent *Event, void *UserData)
{
    if (EventType == EMSCRIPTEN_EVENT_MOUSEMOVE || EventType == EMSCRIPTEN_EVENT_MOUSEDOWN)
    {
        float Scale = WebInput.Scale > 0.f ? WebInput.Scale : 1.f;
        WebInput.MouseX = (i32)((float)Event->targetX * Scale);
        WebInput.MouseY = (i32)((float)Event->targetY * Scale);
    }
    if (EventType != EMSCRIPTEN_EVENT_MOUSEMOVE && Event->button < 5)
    {
        // NOTE(zoubir): the page numbers the buttons left, middle, right
        // like app_input
        u32 Index = (u32)Event->button;
        bool32 IsDown = (EventType == EMSCRIPTEN_EVENT_MOUSEDOWN);
        if (IsDown && !WebInput.Down[Index]) WebInput.WentDown[Index] = true;
        WebInput.Down[Index] = IsDown;
    }
    return EventType != EMSCRIPTEN_EVENT_MOUSEMOVE;
}

// NOTE(zoubir): a key held while the page loses focus never sends its key
// up; let go of everything instead
internal EM_BOOL
WebOnBlur(int EventType, const EmscriptenFocusEvent *Event, void *UserData)
{
    for (u32 Index = 0; Index < WEB_BUTTON_COUNT; Index++) WebInput.Down[Index] = false;
    return false;
}

// NOTE(zoubir): EM_JS code reaches the page as one line, so it has no //
// comments, and a backslash in it does not survive: character codes
// stand in for the line breaks and tabs
EM_JS(void, WebListenForPaste, (), {
    Module.gamePaste = "";
    window.addEventListener("paste", function(Event)
    {
        var Text = (Event.clipboardData || window.clipboardData).getData("text") || "";
        var Line = Text.split(String.fromCharCode(10))[0];
        Module.gamePaste += Line.split(String.fromCharCode(13)).join("").split(String.fromCharCode(9)).join(" ");
        Event.preventDefault();
    });
});

// NOTE(zoubir): writes what was pasted since the last call into Out,
// printable ASCII only, and returns its length
EM_JS(int, WebTakePaste, (char *Out, int OutSize), {
    Out = Number(Out);
    var Text = Module.gamePaste || "";
    Module.gamePaste = "";
    var Count = 0;
    for (var Index = 0; Index < Text.length && Count < OutSize; Index++)
    {
        var Code = Text.charCodeAt(Index);
        if (Code >= 32 && Code < 127) HEAPU8[Out + Count++] = Code;
    }
    return Count;
});

internal void
WebStartInput()
{
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, WebOnKey);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, WebOnKey);
    emscripten_set_mousedown_callback("#canvas", 0, true, WebOnMouse);
    emscripten_set_mousemove_callback("#canvas", 0, true, WebOnMouse);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, WebOnMouse);
    emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, true, WebOnBlur);
    WebListenForPaste();
}

// NOTE(zoubir): this frame's input from the events since the last one
internal void
WebReadInput(app_input *OldInput, app_input *NewInput)
{
    for (u32 Index = 0; Index < WEB_BUTTON_COUNT; Index++)
    {
        app_button_state *Old = WebButton(OldInput, Index);
        app_button_state *New = WebButton(NewInput, Index);
        // NOTE(zoubir): a key that went down and up since the last frame
        // counts as held this frame, so the next one sees it released
        bool32 IsDown = WebInput.Down[Index] || WebInput.TouchDown[Index] ||
            WebInput.WentDown[Index];
        New->Pressed = !Old->EndedDown && IsDown;
        New->Released = Old->EndedDown && !IsDown;
        New->halfTransitionCount = (Old->EndedDown != IsDown) ? 1 : 0;
        New->EndedDown = IsDown;
        WebInput.WentDown[Index] = false;
    }
    NewInput->MouseX = WebInput.MouseX;
    NewInput->MouseY = WebInput.MouseY;
    NewInput->MouseZ = 0;

    char Pasted[64];
    int PastedCount = WebTakePaste(Pasted, (int)sizeof(Pasted));
    for (int Index = 0; Index < PastedCount; Index++) WebAddTyped(Pasted[Index]);
    NewInput->TextInputCount = 0;
    for (u32 Index = 0;
         Index < WebInput.TypedCount && NewInput->TextInputCount + 1 < ArrayCount(NewInput->TextInput);
         Index++)
    {
        NewInput->TextInput[NewInput->TextInputCount++] = WebInput.Typed[Index];
    }
    NewInput->TextInput[NewInput->TextInputCount] = 0;
    NewInput->TextSubmit = WebInput.Submit;
    WebInput.TypedCount = 0;
    WebInput.Submit = false;
}
