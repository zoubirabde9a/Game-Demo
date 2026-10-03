/* Text labels: a line of text in a box, left-aligned or centred. */

internal void
DrawTextLabel(ui_element *TextLabel, ui_context *UIContext)
{
    ui_state *State = TextLabel->State;
    render_context *RenderContext = UIContext->RenderContext;
    render_program TextureProgram = RenderContext->TextureProgram;
    font *Font = State->Font;
    char *Text = TextLabel->State->Text;

    float PaddingX = 4.f;
    float PaddingY = 4.f;
    float PaddingZ = 4.f;
    float PaddingW = 4.f;

    Assert(PaddingX + PaddingY < TextLabel->Width);
    Assert(PaddingZ + PaddingW < TextLabel->Height);
    
    float TextPositionX = TextLabel->X + PaddingX;
    float TextPositionY = TextLabel->Y + PaddingZ;
    float TextWidth = TextLabel->Width - PaddingX - PaddingY;
    float TextHeight = TextLabel->Height - PaddingZ - PaddingW;

    v4 TextClip =
    {
        TextLabel->DrawRect.X + PaddingX,
        TextLabel->DrawRect.Y + PaddingZ,
        TextLabel->DrawRect.Z - PaddingX - PaddingY,
        TextLabel->DrawRect.W + PaddingZ - PaddingW,
    };

    u32 DrawColor = RGBA8_RED;
    if (UIContext->HighlightedElement == TextLabel)
    {
        DrawColor = RGBA8_YELLOW;
    }
    
    if (UIContext->SelectedState == State)
    {
        ui_element_cursor Cursor = UIContext->ElementCursor;
        DrawRectangle(RenderContext, TextLabel->X - 2, TextLabel->Y - 2,
                      TextLabel->Width + 4, TextLabel->Height + 4,
                      RGBA8_BLACK, 0.f);
        DrawRectangle(RenderContext, TextPositionX + Cursor.Offset, TextPositionY,
                      1, TextHeight,
                      DrawColor, 0.f);
    }    
    DrawRectangle(RenderContext, TextLabel->X, TextLabel->Y,
                  TextLabel->Width, TextLabel->Height,
                  DrawColor, 0.f);
    
    
    DrawRectangle(RenderContext, TextPositionX, TextPositionY,
                  TextWidth, TextHeight,
                  RGBA8_BLUE, 0.f);    
    RenderText(RenderContext, TextPositionX, TextPositionY,
               TextWidth, TextHeight, TextClip, Font, TextureProgram,
               Text, State->TextJustification, RGBA8_WHITE,
               0.f);
}

internal void
DoTextLabel(ui_state *TextLabel, app_state *AppState,
            ui_context *UIContext,
            float X, float Y, float Width, float Height,
            char *Text, u32 TextJustification)
{
    app_input *Input = UIContext->Input;
    bool32 MouseIsPressed = Input->LeftButton.Released;
    font *Font = AppState->DefaultFont;
    TextLabel->TextJustification = TextJustification;
    
    v2 ContainerOffset = UIContext->ContainerOffset;
    ui_container *Container = UIContextGetCurrentContainer(UIContext);
        
    TextLabel->Font = Font;
    X += ContainerOffset.X;
    Y += ContainerOffset.Y;

    CopyString(TextLabel->Text, ArrayCount(TextLabel->Text), Text);

    float ClipX = ContainerOffset.X;
    float ClipY = ContainerOffset.Y;
    float ClipWidth = Container->Width;
    float ClipHeight = Container->Height;
    
    v4 DrawRect = ClipRectangle(X, Y, Width, Height,
                  ClipX, ClipY, ClipWidth, ClipHeight);
    
    ui_element *ThisElement =
        UIContextInsertElement(UIContext, UIE_TextLabel,
                               X, Y, DrawRect, TextLabel, Width, Height);

    bool32 IsHighlighted = IsMouseOnRectangle(Input->MouseX, Input->MouseY,
                                              DrawRect.X, DrawRect.Y,
                                              DrawRect.Z, DrawRect.W);
    
    if (IsHighlighted)
    {
        UIContext->HighlightedElement = ThisElement;
    }
    
    bool32 IsPressed = IsHighlighted && MouseIsPressed;
    if (IsPressed)
    {
        UIContext->SelectedState = TextLabel;
    }
    
}

inline void
DoTextLabel(u32 StateIndex, app_state *AppState,
            ui_context *UIContext,
            float X, float Y, float Width, float Height,
            char *Text, u32 TextJustification)
{
    ui_state *TextLabel = UIContextGetState(UIContext, StateIndex);
    DoTextLabel(TextLabel, AppState, UIContext, X, Y,
                Width, Height, Text, TextJustification);
}
