/* Edit boxes: clicking selects one, typing appends to its text (keys
   applied in the order typed, with Backspace, Ctrl+Backspace for a word
   and Ctrl+V pasting, see app_input.TextInput), and DrawEditBox draws the
   field and its cursor, the end of a long text scrolled into view.
   UISelectAllText selects the whole text, so the first key typed replaces
   it. */

internal void
DrawEditBox(ui_element *EditBox, ui_context *UIContext)
{
    ui_state *State = EditBox->State;
    render_context *RenderContext = UIContext->RenderContext;
    v4 Inner = UIInnerRect(EditBox, State->Padding);
    bool32 IsSelected = (UIContext->SelectedState == State);
    // NOTE(zoubir): only the selected box is scrolled to its end
    char *Text = State->Text + (IsSelected ? UIContext->ElementCursorCharOffset : 0);
    bool32 IsHighlighted = (UIContext->HighlightedElement == EditBox);

    float Hot = UIEaseHot(State, IsHighlighted || IsSelected, UIContext->Input->DeltaTime);

    DrawRoundRect(RenderContext, EditBox->X, EditBox->Y,
                  EditBox->Width, EditBox->Height, UI_COLOR_FIELD);
    DrawRoundOutline(RenderContext, EditBox->X, EditBox->Y,
                     EditBox->Width, EditBox->Height,
                     UIMixColor(UI_COLOR_BORDER, UI_COLOR_ACCENT, Hot));
    if (IsSelected && State->AllSelected && State->TextCount > 0)
    {
        DrawRoundRect(RenderContext, Inner.X - 3.f, Inner.Y,
                      UIContext->ElementCursor.Offset + 6.f, Inner.W,
                      WithAlpha(UI_COLOR_ACCENT, 0.3f));
    }
    if (IsSelected)
    {
        // NOTE(zoubir): the caret breathes rather than blinking hard
        float Breath = 0.55f + 0.45f * cosf(5.f * RenderContext->Time);
        DrawFilledRectangle(RenderContext,
                            Inner.X + UIContext->ElementCursor.Offset, Inner.Y + 2.f,
                            2.f, Inner.W - 4.f, WithAlpha(UI_COLOR_ACCENT, Breath), 0.f);
    }
    RenderText(RenderContext, Inner.X, Inner.Y, Inner.Z, Inner.W,
               UIInnerClip(EditBox, State->Padding), State->Font,
               RenderContext->TextureProgram, Text,
               TEXT_JUSTIFICATION_LEFT, UI_COLOR_TEXT, 0.f);
}

// NOTE(zoubir): the width Text takes on screen in Font
internal float
EditBoxTextWidth(font *Font, char *Text)
{
    float Result = 0.f;
    for(; *Text; Text++)
    {
        Result += GetCharacterWidth(Font, *Text);
    }
    return Result;
}

// NOTE(zoubir): the caret sits after the last character; when the text is
// wider than the field, its start scrolls out so the end stays in view
internal void
PlaceEditBoxCaret(ui_context *UIContext, ui_state *EditBox, font *Font, float TextWidth)
{
    u32 FirstShown = 0;
    float Width = EditBoxTextWidth(Font, EditBox->Text);
    while (Width > TextWidth && FirstShown < EditBox->TextCount)
    {
        Width -= GetCharacterWidth(Font, EditBox->Text[FirstShown++]);
    }
    UIContext->ElementCursorCharOffset = FirstShown;
    UIContext->ElementCursor.Pos = EditBox->TextCount;
    UIContext->ElementCursor.Offset = Width;
}

// NOTE(zoubir): typing goes to the selected edit box; it only appends,
// so the cursor sits after the last character (placed by DoEditBox)
internal void
UISelectEditBox(ui_context *UIContext, ui_state *EditBox)
{
    UIContext->SelectedState = EditBox;
    UIContext->ElementCursorCharOffset = 0;
    UIContext->ElementCursor = {};
}

// NOTE(zoubir): selects EditBox with all its text selected, like a field
// a program filled in for the player to keep or type over
internal void
UISelectAllText(ui_context *UIContext, ui_state *EditBox)
{
    UISelectEditBox(UIContext, EditBox);
    EditBox->AllSelected = true;
}

// NOTE(zoubir): applies this frame's keys to EditBox in the order they
// were typed: TEXT_KEY_ERASE takes off one character, TEXT_KEY_ERASE_WORD
// the last word and the spaces after it, anything else is appended while
// there is room. A box whose text is all selected is emptied first
internal void
TypeIntoEditBox(ui_state *EditBox, char *Keys, u32 KeyCount, u32 MaxLength)
{
    if (EditBox->AllSelected && KeyCount > 0)
    {
        EditBox->TextCount = 0;
        EditBox->AllSelected = false;
        if (Keys[0] == TEXT_KEY_ERASE || Keys[0] == TEXT_KEY_ERASE_WORD)
        {
            Keys++;
            KeyCount--;
        }
    }
    for(u32 Index = 0; Index < KeyCount; Index++)
    {
        char C = Keys[Index];
        if (C == TEXT_KEY_ERASE)
        {
            if (EditBox->TextCount > 0)
            {
                EditBox->TextCount--;
            }
        }
        else if (C == TEXT_KEY_ERASE_WORD)
        {
            while (EditBox->TextCount > 0 && EditBox->Text[EditBox->TextCount - 1] == ' ')
            {
                EditBox->TextCount--;
            }
            while (EditBox->TextCount > 0 && EditBox->Text[EditBox->TextCount - 1] != ' ')
            {
                EditBox->TextCount--;
            }
        }
        else if (EditBox->TextCount < MaxLength)
        {
            EditBox->Text[EditBox->TextCount++] = C;
        }
    }
    EditBox->Text[EditBox->TextCount] = 0;
}

// NOTE(zoubir): MaxLength 0 means as much as Text holds
internal void
DoEditBox(ui_state *EditBox, app_state *AppState,
          ui_context *UIContext,
          float X, float Y, float Width, float Height, u32 MaxLength = 0)
{
    if (MaxLength == 0 || MaxLength > ArrayCount(EditBox->Text) - 1)
    {
        MaxLength = ArrayCount(EditBox->Text) - 1;
    }
    app_input *Input = UIContext->Input;
    bool32 MouseIsPressed = Input->LeftButton.Released;
    font *Font = AppState->DefaultFont;
    EditBox->Padding = V4(UI_PADDING, UI_PADDING, UI_PADDING, UI_PADDING);

    v2 ContainerOffset = UIContext->ContainerOffset;
    ui_container *Container = UIContextGetCurrentContainer(UIContext);

    EditBox->Font = Font;
    X += ContainerOffset.X;
    Y += ContainerOffset.Y;

    float ClipX = ContainerOffset.X;
    float ClipY = ContainerOffset.Y;
    float ClipWidth = Container->Width;
    float ClipHeight = Container->Height;

    v4 DrawRect = ClipRectangle(X, Y, Width, Height,
                  ClipX, ClipY, ClipWidth, ClipHeight);

    ui_element *EditBoxElement =
        UIContextInsertElement(UIContext, UIE_EditBox,
                               X, Y, DrawRect, EditBox, Width, Height);

    bool32 IsHighlighted = IsMouseOnRectangle(Input->MouseX, Input->MouseY,
                                              DrawRect.X, DrawRect.Y,
                                              DrawRect.Z, DrawRect.W);

    if (IsHighlighted)
    {
        UIContext->HighlightedElement = EditBoxElement;
    }

    bool32 IsPressed = IsHighlighted && MouseIsPressed;
    if (IsPressed)
    {
        UISelectEditBox(UIContext, EditBox);
        EditBox->AllSelected = false;
    }
    if (UIContext->SelectedState == EditBox)
    {
        TypeIntoEditBox(EditBox, Input->TextInput, Input->TextInputCount, MaxLength);
        float TextWidth = Width - (EditBox->Padding.X + EditBox->Padding.Z);
        PlaceEditBoxCaret(UIContext, EditBox, Font, TextWidth);
    }
}

inline void
DoEditBox(u32 StateIndex, app_state *AppState,
          ui_context *UIContext,
          float X, float Y, float Width, float Height, u32 MaxLength = 0)
{
    ui_state *EditBox = UIContextGetState(UIContext, StateIndex);
    DoEditBox(EditBox, AppState, UIContext, X, Y, Width, Height, MaxLength);
}
