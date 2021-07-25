/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */
#include "ui.h"

#if 0
internal bool32
IsMouseOnUiBorder(i32 MouseX, i32 MouseY,
                  ui_element_border *Border)
{
    bool32 Result = false;
    switch(Border->Type)
    {
        case UI_ELEMENT_BORDER_RECTANGULAR:
        {
            Result =
                IsMouseOnRectangle(MouseX, MouseY,
                                       Border->X, Border->Y,
                                       Border->Width, Border->Height);
            break;
        }
    }
    return Result;
}
#endif

#if 0
internal void
UIContextAddRectangularBorder(ui_context *UIContext,
                              float X, float Y,
                              float Width, float Height)
{
    Assert(UIContext->ElementBordersCount + 1 < UIContext->AllocatedElementBordersCount);
    ui_element_border *NewBorder =
        &UIContext->AllocatedElementBorders[UIContext->ElementBordersCount];
    UIContext->ElementBordersCount++;
    NewBorder->Type = UI_ELEMENT_BORDER_RECTANGULAR;
    NewBorder->X = X;
    NewBorder->Y = Y;
    NewBorder->Width = Width;
    NewBorder->Height = Height;
}
#endif

inline ui_container *
UIContextGetCurrentContainer(ui_context *UIContext)
{
    Assert(UIContext->ContainersCount > 0);
    return &UIContext->AllocatedContainers[UIContext->ContainersCount - 1];
}

inline ui_element *
UIContextInsertElement(ui_context *UIContext)
{
    ui_element *NewElement =
        &UIContext->AllocatedElements[UIContext->ElementsCount++];
    return NewElement;
}

inline ui_element *
UIContextInsertElement(ui_context *UIContext,
                       ui_element_type Type, float X, float Y,
                       v4 DrawRect, ui_state *State,
                       float Width, float Height)
{
    ui_element *Element =
        UIContextInsertElement(UIContext);
    Element->Type = Type;
    Element->X = X;
    Element->Y = Y;
    Element->DrawRect = DrawRect;
    Element->State = State;
    Element->Width = Width;
    Element->Height = Height;
    
    return Element;
}

inline ui_state *
UIContextGetState(ui_context *UIContext, u32 StateIndex)
{
    ui_state *State =
        &UIContext->AllocatedStates[StateIndex];
    return State;
}

internal ui_context *
UIContextCreate(memory_arena *Arena, u32 NumberOfStates)
{
    //TODO(zoubir): is the memory initialized at zero ?
    ui_context *UIContext = AllocateStruct(Arena, ui_context);
        
    UIContext->AllocatedStates =
        AllocateArray(Arena, NumberOfStates, ui_state);
    UIContext->AllocatedStatesCount = NumberOfStates;    

    return UIContext;
}

internal void
UIBegin(render_context *RenderContext,
        memory_arena *TransientArena,
        app_input *Input, app_state *AppState,
        ui_context *UIContext,
        u32 NumberOfElements, u32 NumberOfContainers)
{
    Assert(UIContext->ContainersCount == 0);
    Assert(UIContext->ElementsCount == 0);
    Assert(UIContext->StatesCount == 0);
    
    UIContext->RenderContext = RenderContext;
    UIContext->Input = Input;
    UIContext->AppState = AppState;
    
    u32 NumberOfVertices = NumberOfElements * 6;
    SetupBatchRenderer(RenderContext, TransientArena, NumberOfElements);
    RenderBegin(RenderContext, NumberOfVertices, RENDER_ORDER_NO_ORDER);
    
    if (NumberOfElements > UIContext->AllocatedElementsCount)
    {
        UIContext->AllocatedElements =
            AllocateArray(TransientArena, NumberOfElements,
                                          ui_element);
        UIContext->AllocatedElementsCount = NumberOfElements;
    }
    if (NumberOfContainers > UIContext->AllocatedContainersCount)
    {
        UIContext->AllocatedContainers =
            AllocateArray(TransientArena, NumberOfContainers,
                                   ui_container);
        UIContext->AllocatedContainersCount = NumberOfContainers;
    }

    UIContext->ContainerOffset = V2(0.f);
    UIContext->HighlightedElement = 0;
}

inline void
BeginContainer(ui_context *UIContext, float X, float Y,
               float Width, float Height)
{
    Assert(UIContext->AllocatedContainersCount > UIContext->ContainersCount);
    ui_container *NewContainer =
        &UIContext->AllocatedContainers[UIContext->ContainersCount++];
    NewContainer->X = X;
    NewContainer->Y = Y;
    NewContainer->Width = Width;
    NewContainer->Height = Height;
    
    UIContext->ContainerOffset.X += X;
    UIContext->ContainerOffset.Y += Y;
}
inline void
EndContainer(ui_context *UIContext)
{
    Assert(UIContext->ContainersCount > 0);
    ui_container *LastContainer =
        &UIContext->AllocatedContainers[UIContext->ContainersCount--];
    // C
    UIContext->ContainerOffset.X -= LastContainer->X;
    UIContext->ContainerOffset.Y -= LastContainer->Y;
}

inline v4
ClipRectangle1(v2 Pos0, v2 Pos1, v2 Clip0, v2 Clip1)
{

    v4 Result;
    
    if (Pos0.X < Clip0.X)
    {
        Result.X = Clip0.X;
    }
    else
    {
        Result.X = Pos0.X;
    }
    
    if (Pos0.Y < Clip0.Y)
    {
        Result.Y = Clip0.Y;
    }
    else
    {
        Result.Y = Pos0.Y;
    }

    if (Pos1.X > Clip1.X)
    {
        Result.Z = Clip1.X - Result.X;
    }
    else
    {
        Result.Z = Pos1.X - Pos0.X;
    }
    
    if (Pos1.Y > Clip1.Y)
    {
        Result.W = Clip1.Y - Result.Y;
    }
    else
    {
        Result.W = Pos1.Y - Pos0.Y;
    }

    return Result;
}

internal bool32
DoButton(ui_state *Button,
         app_state *AppState, ui_context *UIContext,
         float X, float Y,
         float Width, float Height, char *Text)
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
         float Width, float Height, char *Text)
{
    ui_state *Button = UIContextGetState(UIContext, StateIndex);
    return DoButton(Button, AppState, UIContext, X, Y, Width, Height, Text);
}

internal void
DrawButton(ui_element* Button, ui_context *UIContext)
{
    app_state *AppState = UIContext->AppState;
    render_context *RenderContext = UIContext->RenderContext;
    render_program TextureProgram = RenderContext->TextureProgram;
    loaded_texture ButtonTexture = AcquireTexture(AppState, "a");
    ui_state *State = Button->State;
    font *Font = State->Font;
    char *Text = State->Text;
    
    bool IsHighlighted = (UIContext->HighlightedElement ==
                          Button);
    float PaddingX = 4.f;
    float PaddingY = 4.f;
    float PaddingZ = 4.f;
    float PaddingW = 4.f;

    Assert(PaddingX + PaddingY < Button->Width);
    Assert(PaddingZ + PaddingW < Button->Height);
    
    float TextPositionX = Button->X + PaddingX;
    float TextPositionY = Button->Y + PaddingZ;
    float TextWidth = Button->Width - PaddingX - PaddingY;
    float TextHeight = Button->Height - PaddingZ - PaddingW;

    v4 TextClip =
        {
            Button->DrawRect.X + PaddingX,
            Button->DrawRect.Y + PaddingZ,
            Button->DrawRect.Z - PaddingX - PaddingY,
            Button->DrawRect.W + PaddingZ - PaddingW,
        };
                          
    u32 DrawColor = RGBA8_RED;
    
    if (IsHighlighted)
    {
        DrawColor = RGBA8_YELLOW;
    }
    
    DrawRectangle(RenderContext, Button->X, Button->Y,
                  Button->Width, Button->Height,
                  DrawColor, 0.f);
    #if 0
    DrawRectangle(RenderContext,
                  Button->DrawRect.X, Button->DrawRect.Y,
                  Button->DrawRect.Z, Button->DrawRect.W,
                  RGBA8_GREEN, 0.f);
    #endif
    DrawRectangle(RenderContext, TextPositionX, TextPositionY,
                  TextWidth, TextHeight,
                  RGBA8_BLUE, 0.f);
    RenderText(RenderContext, TextPositionX, TextPositionY,
               TextWidth, TextHeight, TextClip, Font, TextureProgram,
               Text, TEXT_JUSTIFICATION_MIDDLE, RGBA8_WHITE,
               0.f);
}

internal void
DrawEditBox(ui_element *EditBox, ui_context *UIContext)
{
    ui_state *State = EditBox->State;
    render_context *RenderContext = UIContext->RenderContext;
    render_program TextureProgram = RenderContext->TextureProgram;
    font *Font = State->Font;
    char *Text = EditBox->State->Text + UIContext->ElementCursorCharOffset;
    
    Assert(State->Padding.X + State->Padding.Y < EditBox->Width);
    Assert(State->Padding.Z + State->Padding.W < EditBox->Height);
    
    float TextPositionX = EditBox->X + State->Padding.X;
    float TextPositionY = EditBox->Y + State->Padding.Z;
    float TextWidth = EditBox->Width - State->Padding.X -
        State->Padding.Y;
    float TextHeight = EditBox->Height - State->Padding.Z -
        State->Padding.W;

    v4 TextClip =
    {
        EditBox->DrawRect.X + State->Padding.X,
        EditBox->DrawRect.Y + State->Padding.Z,
        EditBox->DrawRect.Z - State->Padding.X - State->Padding.Y,
        EditBox->DrawRect.W + State->Padding.Z - State->Padding.W,
    };

    u32 DrawColor = RGBA8_RED;
    if (UIContext->HighlightedElement == EditBox)
    {
        DrawColor = RGBA8_YELLOW;
    }
    
    if (UIContext->SelectedState == State)
    {
        ui_element_cursor Cursor = UIContext->ElementCursor;
        DrawRectangle(RenderContext, EditBox->X - 2, EditBox->Y - 2,
                      EditBox->Width + 4, EditBox->Height + 4,
                      RGBA8_BLACK, 0.f);
        DrawRectangle(RenderContext, TextPositionX + Cursor.Offset, TextPositionY,
                      1, TextHeight,
                      DrawColor, 0.f);
    }    
    DrawRectangle(RenderContext, EditBox->X, EditBox->Y,
                  EditBox->Width, EditBox->Height,
                  DrawColor, 0.f);
    
    
    DrawRectangle(RenderContext, TextPositionX, TextPositionY,
                  TextWidth, TextHeight,
                  RGBA8_BLUE, 0.f);    
    RenderText(RenderContext, TextPositionX, TextPositionY,
               TextWidth, TextHeight, TextClip, Font, TextureProgram,
               Text, TEXT_JUSTIFICATION_LEFT, RGBA8_WHITE,
               0.f);
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

internal void
DoEditBox(ui_state *EditBox, app_state *AppState,
          ui_context *UIContext,
          float X, float Y, float Width, float Height)
{
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
        UIContext->SelectedState = EditBox;
        float Offset = Input->MouseX - X;
        UIContext->ElementCursor =
            GetElementCursorFromOffset(Font, Text, Offset);
        
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
          float X, float Y, float Width, float Height)
{
    ui_state *EditBox = UIContextGetState(UIContext, StateIndex);
    DoEditBox(EditBox, AppState, UIContext, X, Y, Width, Height);
}

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

internal void
DrawTilePickerWidget(ui_element *Widget, ui_context *UIContext)
{
    ui_state *State = Widget->State;
    render_context *RenderContext = UIContext->RenderContext;
    render_program TextureProgram = RenderContext->TextureProgram;

    loaded_texture *Texture = State->TileMap;

    v4 *DrawRect = &Widget->DrawRect;
    float TileMapWidth = DrawRect->Z;
    float TileMapHeight = DrawRect->W;
    v4 Uvs = {0.f, 0.f, 1.f, 1.f};
    
    if (DrawRect->Z > Texture->Width)
    {
        TileMapWidth = (float)Texture->Width;
    }
    if (DrawRect->W > Texture->Height)
    {        
        TileMapHeight = (float)Texture->Height;
    }

    if (DrawRect->Z < Texture->Width)
    {
        Uvs.Z = DrawRect->Z / Texture->Width;
        Uvs.Z += (State->OffsetX * State->TileWidth) / (float)Texture->Width;
        Uvs.X += (State->OffsetX * State->TileWidth) / (float)Texture->Width;
    }
    if (DrawRect->W < Texture->Height)
    {
        Uvs.W = DrawRect->W / Texture->Height;
        Uvs.Y = 1.f - Uvs.W;
        Uvs.Y -= (State->OffsetY * State->TileHeight) / (float)Texture->Height;
        Uvs.W = 1.f;
        Uvs.W -= (State->OffsetY * State->TileHeight) / (float)Texture->Height;
    }
    
    BeginBatch(RenderContext, Texture->ID, 0.f, TextureProgram);
    RenderQuadTexture(RenderContext, DrawRect->X, DrawRect->Y,
                    TileMapWidth, TileMapHeight, Uvs,
                    RGBA8_WHITE, 0.f);
    EndBatch(RenderContext);

    
    v4 SelectedTileRect = {};
    SelectedTileRect = State->SelectedRectangle;
    #if 0
    if (State->NumTilesX > 0)
    {
        SelectedTileRect.X = Widget->X + (float)State->TileWidth * 
            (State->SelectedTileIndex % State->NumTilesX);
        SelectedTileRect.Y = Widget->Y + (float)State->TileHeight * 
            (State->SelectedTileIndex / State->NumTilesX);    
        SelectedTileRect.Z = (float)State->TileWidth;
        SelectedTileRect.W = (float)State->TileHeight;
    }
    #endif
    
    DrawRectangle(RenderContext, SelectedTileRect.X,
                  SelectedTileRect.Y,
                  SelectedTileRect.Z,
                  SelectedTileRect.W,
                  RGBA8_BLUE, 0.f);
    
    DrawRectangle(RenderContext, DrawRect->X, DrawRect->Y,
                  DrawRect->Z, DrawRect->W,
                  RGBA8_YELLOW, 0.f);    
}

internal void
DoTilePickerWidget(ui_state *State, app_state *AppState,
            ui_context *UIContext,
            float X, float Y, float Width, float Height,
                   loaded_texture *TileMap, u32 TileWidth, u32 TileHeight,
                   u32 TileMapNumTilesX, u32 TileMapNumTilesY)
{
    app_input *Input = UIContext->Input;
    bool32 MouseIsPressed = Input->LeftButton.Pressed;
    bool32 MouseIsReleased = Input->LeftButton.Released;
    
    State->TileMap = TileMap;
    State->TileMapNumTilesX = TileMapNumTilesX;
    State->TileMapNumTilesY = TileMapNumTilesY;
    State->TileWidth = TileWidth;
    State->TileHeight = TileHeight;
    State->NumTilesX = TileMap->Width / TileWidth;
    State->NumTilesY = TileMap->Height / TileHeight;
    
    State->OffsetX = 0;
    State->OffsetY = 20;
        
    v2 ContainerOffset = UIContext->ContainerOffset;
    ui_container *Container = UIContextGetCurrentContainer(UIContext);
        
    X += ContainerOffset.X;
    Y += ContainerOffset.Y;

    float ClipX = ContainerOffset.X;
    float ClipY = ContainerOffset.Y;
    float ClipWidth = Container->Width;
    float ClipHeight = Container->Height;
    
    v4 DrawRect = ClipRectangle(X, Y, Width, Height,
                  ClipX, ClipY, ClipWidth, ClipHeight);
    
    ui_element *ThisElement =
        UIContextInsertElement(UIContext, UIE_TilePickerWidget,
                               X, Y, DrawRect, State, Width, Height);

    i32 MouseX = Input->MouseX;
    i32 MouseY = Input->MouseY;    
    bool32 IsHighlighted = IsMouseOnRectangle(MouseX, MouseY,
                                              DrawRect.X, DrawRect.Y,
                                              DrawRect.Z, DrawRect.W);
    
    if (IsHighlighted)
    {
        UIContext->HighlightedElement = ThisElement;
    }
    
    bool32 IsPressed = IsHighlighted && MouseIsReleased;
    
    if (IsPressed)
    {
        UIContext->SelectedState = State;

#if 0        
        u32 SelectedTileX = (u32)(RelativeX / State->TileWidth);
        u32 SelectedTileY = (u32)(RelativeY / State->TileHeight);
        
        State->SelectedTileIndex = SelectedTileX +
            (State->NumTilesY - SelectedTileY - 1) *
            State->NumTilesX;

        float ClickPosX = (float)(SelectedTileX * State->TileWidth);
        float ClickPosY = (float)(SelectedTileY * State->TileHeight);

        v4 *SelectedRectangle = &State->SelectedRectangle;
        SelectedRectangle->X = X + ClickPosX;
        SelectedRectangle->Y = Y + ClickPosY;
        SelectedRectangle->Z = (float)TileWidth;
        SelectedRectangle->W = (float)TileHeight;
#endif
    }
    
    if (MouseIsPressed && IsHighlighted)
    {
        if (MouseX < X + TileMap->Width &&
            MouseY < Y + TileMap->Height)
        {
            State->MousePressedInside = true;
            float RelativeX = MouseX - X;
            float RelativeY = MouseY - Y;
            
            State->RelativeClickPosition.X = RelativeX;
            State->RelativeClickPosition.Y = RelativeY;
        }
        
    }
    
    if (State->MousePressedInside)
    {
        float RelativeMouseX = MouseX - X;
        float RelativeMouseY = MouseY - Y;

        float MinWidth = Minimum(Width, TileMap->Width);
        float MinHeight = Minimum(Height, TileMap->Height);
        
        v2 Min;
        Min.X = Minimum(State->RelativeClickPosition.X, RelativeMouseX);
        Min.Y = Minimum(State->RelativeClickPosition.Y, RelativeMouseY);
        v2 Max;
        Max.X = Maximum(State->RelativeClickPosition.X, RelativeMouseX);
        Max.Y = Maximum(State->RelativeClickPosition.Y, RelativeMouseY);

        Min.X = Maximum(0.f, Min.X);
        Min.Y = Maximum(0.f, Min.Y);

        Max.X = Minimum(MinWidth, Max.X);
        Max.Y = Minimum(MinHeight, Max.Y);

        u32 MinTileX = (u32)(Min.X / TileWidth);
        u32 MinTileY = (u32)(Min.Y / TileHeight);

        u32 MaxTileX = CeilFloatToUInt32(Max.X / TileWidth);
        u32 MaxTileY = CeilFloatToUInt32(Max.Y / TileHeight);
        u32 MinTilePosX = MinTileX * TileWidth;
        u32 MinTilePosY = MinTileY * TileHeight;
        
        u32 MaxTilePosX = MaxTileX * TileWidth;
        u32 MaxTilePosY = MaxTileY * TileHeight;
        
        v4 *SelectedRectangle = &State->SelectedRectangle;
        SelectedRectangle->X = X + MinTilePosX;
        SelectedRectangle->Y = Y + MinTilePosY;
        SelectedRectangle->Z = (float)(MaxTilePosX - MinTilePosX);
        SelectedRectangle->W = (float)(MaxTilePosY - MinTilePosY);
        
        if (MouseIsReleased)
        {
            MinTileX += State->OffsetX;
            MinTileY += State->OffsetY;
            MaxTileX += State->OffsetX;
            MaxTileY += State->OffsetY;

            State->MousePressedInside = false;

            State->SelectedTileCountX = MaxTileX - MinTileX;
            State->SelectedTileCountY = MaxTileY - MinTileY;
            
            Assert(State->SelectedTileCountX <= 12);
            Assert(State->SelectedTileCountY <= 12);
            
            for(u32 TileY = MinTileY;
                TileY < MaxTileY;
                TileY++)
            {
                for(u32 TileX = MinTileX;
                    TileX < MaxTileX;
                    TileX++)
                {
                    u32 TileIndex =
                        TileX + (TileMapNumTilesY - TileY - 1) *
                        TileMapNumTilesX;
                    State->SelectedTileIndices[TileX - MinTileX]
                        [TileY - MinTileY] =
                        TileIndex;                
                }
            }
        }    
    }
}

inline void
UIEnd(ui_context *UIContext)
{
    render_context *RenderContext = UIContext->RenderContext;
    
    ui_element *Elements = UIContext->AllocatedElements;
    for(u32 ElementIndex = 0;
        ElementIndex < UIContext->ElementsCount;
        ElementIndex++)
    {
        ui_element *Element = &Elements[ElementIndex];
        switch(Element->Type)
        {
            case UIE_Button:
            {
                DrawButton(Element, UIContext);
                break;
            }
            case UIE_EditBox:
            {
                DrawEditBox(Element, UIContext);
                break;
            }
            case UIE_TextLabel:
            {
                DrawTextLabel(Element, UIContext);
                break;
            }
            case UIE_TilePickerWidget:
            {
                DrawTilePickerWidget(Element, UIContext);
                break;
            }
            default:
            {
                InvalidCodePath;
                break;
            }
        }
    }
        
    RenderFlush(RenderContext);
    UIContext->ElementsCount = 0;
    UIContext->ContainersCount = 0;
}
