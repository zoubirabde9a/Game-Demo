/* Tile editor (F3): a tile picker panel on the right, a preview of the
   picked tiles under the mouse, and painting them into the arena's tile
   map while the left button is held outside the panel. */

internal void
DoTileEditor(render_context *RenderContext, app_state *AppState,
             ui_context *UIContext, app_input *Input, app_window *Window,
             render_program TextureProgram, v3 CameraOffset)
{
    if (Input->ButtonF3.Pressed)
    {
        AppState->TileEditing = !AppState->TileEditing;
    }
    if (!AppState->TileEditing)
    {
        return;
    }

    world *World = &AppState->World;
    tile_map *TileMap = &World->TileMap;
    float PanelWidth = 300;
    float PanelHeight = 600;
    float PanelX = Window->Width - PanelWidth - 60;
    float PanelY = (Window->Height - PanelHeight) / 2;

    BeginContainer(UIContext, PanelX, PanelY, PanelWidth, PanelHeight);
    DrawRectangle(RenderContext, PanelX, PanelY, PanelWidth, PanelHeight,
                  RGBA8_YELLOW, 0.f);

    ui_state *Picker = &AppState->TilePickerWidget;
    DoTilePickerWidget(Picker, AppState, UIContext, 0.f, 0.f, 200.f, 200.f,
                       GetTexture(&AppState->Assets, AppState->OpenGL,
                                  AppState, {AssetType_TileMap}),
                       32, 32, 8, 200);

    // NOTE(zoubir): the tile under the mouse, in screen space
    float TileWidth = (float)World->TileWidth;
    float TileHeight = (float)World->TileHeight;
    float X = (float)((u32)(((float)Input->MouseX + CameraOffset.X) / TileWidth));
    float Y = (float)((u32)(((float)Input->MouseY + CameraOffset.Y) / TileHeight));
    X = X * TileWidth - CameraOffset.X;
    Y = Y * TileHeight - CameraOffset.Y;

    loaded_texture *PickerTexture = Picker->TileMap;
    if (PickerTexture)
    {
        for(u32 IndexY = 0; IndexY < Picker->SelectedTileCountY; IndexY++)
        {
            for(u32 IndexX = 0; IndexX < Picker->SelectedTileCountX; IndexX++)
            {
                u32 IndexInTexture = Picker->SelectedTileIndices[IndexX][IndexY];
                v4 Uvs = GetTextureUvsFromIndex(PickerTexture->Width,
                                                PickerTexture->Height,
                                                Picker->TileMapNumTilesX,
                                                Picker->TileMapNumTilesY,
                                                IndexInTexture);
                BeginBatch(RenderContext, PickerTexture->ID, 0.f, TextureProgram);
                RenderQuadTexture(RenderContext,
                                  X + IndexX * TileWidth, Y + IndexY * TileHeight,
                                  TileWidth, TileHeight, Uvs, RGBA8_WHITE, 0.f);
                EndBatch(RenderContext);

                if (Input->LeftButton.EndedDown &&
                    !IsMouseOnRectangle(Input->MouseX, Input->MouseY,
                                        PanelX, PanelY, PanelWidth, PanelHeight))
                {
                    u32 OffsetX = IndexX * World->TileWidth;
                    u32 OffsetY = IndexY * World->TileHeight;
                    u32 TileX = (u32)(CameraOffset.X + Input->MouseX + OffsetX) /
                        World->TileWidth;
                    u32 TileY = (u32)(CameraOffset.Y + Input->MouseY + OffsetY) /
                        World->TileHeight;
                    TileMap->Tiles[TileX + TileY * World->NumTilesX].Index =
                        IndexInTexture;
                }
            }
        }
    }
    DrawRectangle(RenderContext, X, Y,
                  (float)(Picker->SelectedTileCountX * World->TileWidth),
                  (float)(Picker->SelectedTileCountY * World->TileHeight),
                  RGBA8_YELLOW, 0.f);
    EndContainer(UIContext);
}
