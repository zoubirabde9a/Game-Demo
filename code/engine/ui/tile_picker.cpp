/* The tile picker: shows a tile sheet and lets the tile editor pick a
   tile from it. */

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
