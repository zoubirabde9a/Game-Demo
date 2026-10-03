/* Edit boxes: clicking selects one, typing appends to its text, and
   DrawEditBox draws the field and its cursor. */

internal void
DrawEditBox(ui_element *EditBox, ui_context *UIContext)
{
    ui_state *State = EditBox->State;
    render_context *RenderContext = UIContext->RenderContext;
    char *Text = EditBox->State->Text + UIContext->ElementCursorCharOffset;
    v4 Inner = UIInnerRect(EditBox, State->Padding);
    bool32 IsSelected = (UIContext->SelectedState == State);
    bool32 IsHighlighted = (UIContext->HighlightedElement == EditBox);

    DrawFilledRectangle(RenderContext, EditBox->X, EditBox->Y,
                        EditBox->Width, EditBox->Height, RGBA8_UI_FIELD, 0.f);
    DrawRectangle(RenderContext, EditBox->X, EditBox->Y,
                  EditBox->Width, EditBox->Height,
                  (IsSelected || IsHighlighted) ? RGBA8_UI_FOCUS :
                  RGBA8_UI_BORDER, 0.f);
    if (IsSelected)
    {
        DrawFilledRectangle(RenderContext,
                            Inner.X + UIContext->ElementCursor.Offset, Inner.Y,
                            1.f, Inner.W, RGBA8_UI_FOCUS, 0.f);
    }
    RenderText(RenderContext, Inner.X, Inner.Y, Inner.Z, Inner.W,
               UIInnerClip(EditBox, State->Padding), State->Font,
               RenderContext->TextureProgram, Text,
               TEXT_JUSTIFICATION_LEFT, RGBA8_WHITE, 0.f);
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
    EditBox->Padding = V4(4.f, 4.f, 4.f, 4.f);

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
    }
    if (UIContext->SelectedState == EditBox)
    {
        if (Input->TextErase && EditBox->TextCount > 0)
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
