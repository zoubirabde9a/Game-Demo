/* Edit boxes: clicking selects one, typing appends to its text, and
   DrawEditBox draws the field and its cursor. UISelectAllText selects the
   whole text, so the first key typed replaces it. */

internal void
DrawEditBox(ui_element *EditBox, ui_context *UIContext)
{
    ui_state *State = EditBox->State;
    render_context *RenderContext = UIContext->RenderContext;
    char *Text = EditBox->State->Text + UIContext->ElementCursorCharOffset;
    v4 Inner = UIInnerRect(EditBox, State->Padding);
    bool32 IsSelected = (UIContext->SelectedState == State);
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

internal ui_element_cursor
GetElementCursorFromOffset(font *Font, char *Text, float Offset)
{
    ui_element_cursor Result = {};    
    
    stbtt_bakedchar *Glyphs = (stbtt_bakedchar *)Font->Glyphs;
    u32 FirstCharacter = Font->FirstGlyph;
    u32 LastCharacter = Font->FirstGlyph + Font->GlyphsSize;
    while(*Text)
    {
        u32 CurrentCode = (u32)(*Text);
        if (CurrentCode >= FirstCharacter && CurrentCode < LastCharacter) {
            Result.Offset += Glyphs[CurrentCode - FirstCharacter].xadvance;
        }
        if (Result.Offset > Offset)
        {
            break;
        }
        Result.Pos++;
        Text++;
    }

    return Result;
}

// NOTE(zoubir): typing goes to the selected edit box; it only appends,
// so the cursor sits after the last character
internal void
UISelectEditBox(ui_context *UIContext, ui_state *EditBox)
{
    UIContext->SelectedState = EditBox;
    UIContext->ElementCursorCharOffset = 0;
    UIContext->ElementCursor = {};
    if (EditBox->Font)
    {
        UIContext->ElementCursor =
            GetElementCursorFromOffset(EditBox->Font, EditBox->Text, 1e9f);
    }
}

// NOTE(zoubir): selects EditBox with all its text selected, like a field
// a program filled in for the player to keep or type over
internal void
UISelectAllText(ui_context *UIContext, ui_state *EditBox)
{
    UISelectEditBox(UIContext, EditBox);
    EditBox->AllSelected = true;
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
    char *Text = EditBox->Text;
    char *TextInput = Input->TextInput;
    u32 TextInputCount = Input->TextInputCount;    
    EditBox->Padding = V4(UI_PADDING, UI_PADDING, UI_PADDING, UI_PADDING);

    v2 ContainerOffset = UIContext->ContainerOffset;
    ui_container *Container = UIContextGetCurrentContainer(UIContext);
        
    // NOTE(zoubir): a box selected before it was first drawn had no font
    // to measure its caret with; place the caret now
    bool32 Unmeasured = !EditBox->Font;
    EditBox->Font = Font;
    if (Unmeasured && UIContext->SelectedState == EditBox)
    {
        UISelectEditBox(UIContext, EditBox);
    }
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
    bool32 Replaced = false;
    if (UIContext->SelectedState == EditBox && EditBox->AllSelected &&
        (Input->TextErase || Input->TextInputCount > 0))
    {
        EditBox->Text[0] = 0;
        EditBox->TextCount = 0;
        EditBox->AllSelected = false;
        UISelectEditBox(UIContext, EditBox);
        Replaced = true;
    }
    if (UIContext->SelectedState == EditBox)
    {
        if (Input->TextErase && EditBox->TextCount > 0 && !Replaced)
        {
            char *CharacterPtrToErase = &EditBox->Text[(EditBox->TextCount - 1)];
            char ErasedChar = *CharacterPtrToErase;
            *CharacterPtrToErase = '\0';
            EditBox->TextCount--;
            UIContext->ElementCursor.Pos--;
            UIContext->ElementCursor.Offset -=
                GetCharacterWidth(Font, ErasedChar);
        }
        
        if (Input->TextInputCount > 0)
        {
            float TextWidth = Width - (EditBox->Padding.X + EditBox->Padding.Z);
            for(u32 CurrentCharacter = 0;
                CurrentCharacter < TextInputCount;
                CurrentCharacter++)
            {
                char C = TextInput[CurrentCharacter];
                if (EditBox->TextCount >= MaxLength)
                {
                    break;
                }
                EditBox->Text[EditBox->TextCount++] = C;
                UIContext->ElementCursor.Pos++;
                float Offset = UIContext->ElementCursor.Offset +
                    GetCharacterWidth(Font, C);
                if (Offset > TextWidth)
                {
                    char ClippedCharacter = EditBox->Text[UIContext->ElementCursorCharOffset];
                    UIContext->ElementCursorCharOffset++;
                    //TODO(zoubir): a bug sometimes the 
                    // first character is larger than the
                    // erased character
                    Offset -= GetCharacterWidth(Font, ClippedCharacter);
                }
                UIContext->ElementCursor.Offset = Offset;
            }
            EditBox->Text[EditBox->TextCount] = '\0';
        }
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
