/* Buttons: DoButton handles the mouse and queues the button; DrawButton
   draws it at UIEnd. Selected marks the current choice in a group of
   buttons (the map picker), in the accent colour. */

internal bool32
DoButton(ui_state *Button,
         app_state *AppState, ui_context *UIContext,
         float X, float Y,
         float Width, float Height, char *Text, bool32 Selected = false)
{
    bool32 IsPressed = false;
   
    app_input *Input = UIContext->Input;
    render_context *RenderContext = UIContext->RenderContext;
    render_program TextureProgram = RenderContext->TextureProgram;
    
    bool32 MouseIsPressed = Input->LeftButton.Released;

    v2 ContainerOffset = UIContext->ContainerOffset;
    ui_container *Container = UIContextGetCurrentContainer(UIContext);

    Button->Font = AppState->DefaultFont;
    X += ContainerOffset.X;
    Y += ContainerOffset.Y;

    CopyString(Button->ButtonText, ArrayCount(Button->ButtonText), Text);
    Button->IsSelected = Selected;

    float ClipX = ContainerOffset.X;
    float ClipY = ContainerOffset.Y;
    float ClipWidth = Container->Width;
    float ClipHeight = Container->Height;

    v4 DrawRect = ClipRectangle(X, Y, Width, Height,
                  ClipX, ClipY, ClipWidth, ClipHeight);

    ui_element *ButtonElement =
        UIContextInsertElement(UIContext, UIE_Button,
                               X, Y, DrawRect, Button, Width, Height);
    

    bool32 IsHighlighted = IsMouseOnRectangle(Input->MouseX, Input->MouseY,
                           DrawRect.X, DrawRect.Y,
                           DrawRect.Z, DrawRect.W);
    
    if (IsHighlighted)
    {
        UIContext->HighlightedElement = ButtonElement;
    }
    
    IsPressed = IsHighlighted && MouseIsPressed;
    Button->IsPressed = IsPressed;
    
    if (IsPressed)
    {
        UIContext->SelectedState = Button;
    }
    
     //TODO(zoubir):  make sure the selected/ highlighted
    // element in
    // ui_context is updated 
    
    return IsPressed;
}

inline bool32
DoButton(u32 StateIndex,
         app_state *AppState, ui_context *UIContext,
         float X, float Y,
         float Width, float Height, char *Text, bool32 Selected = false)
{
    ui_state *Button = UIContextGetState(UIContext, StateIndex);
    return DoButton(Button, AppState, UIContext, X, Y, Width, Height, Text,
                    Selected);
}

internal void
DrawButton(ui_element* Button, ui_context *UIContext)
{
    render_context *RenderContext = UIContext->RenderContext;
    ui_state *State = Button->State;
    v4 Padding = V4(UI_PADDING, UI_PADDING, UI_PADDING, UI_PADDING);
    v4 Inner = UIInnerRect(Button, Padding);
    bool32 IsHighlighted = (UIContext->HighlightedElement == Button);
    bool32 IsSelected = State->IsSelected;

    DrawFilledRectangle(RenderContext, Button->X, Button->Y,
                        Button->Width, Button->Height,
                        (IsHighlighted || IsSelected) ?
                        UI_COLOR_CONTROL_HOT : UI_COLOR_CONTROL, 0.f);
    DrawRectangle(RenderContext, Button->X, Button->Y,
                  Button->Width, Button->Height,
                  (IsHighlighted || IsSelected) ?
                  UI_COLOR_ACCENT : UI_COLOR_BORDER, 0.f);
    RenderText(RenderContext, Inner.X, Inner.Y, Inner.Z, Inner.W,
               UIInnerClip(Button, Padding), State->Font,
               RenderContext->TextureProgram, State->ButtonText,
               TEXT_JUSTIFICATION_MIDDLE,
               IsSelected ? UI_COLOR_ACCENT : UI_COLOR_TEXT, 0.f);
}
