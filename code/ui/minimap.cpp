/* Minimap: the ground around the local player in the top-right corner,
   one texel per tile, with every player as a dot, landmarks as rings
   (client/landmark_pointer.cpp knows which are reached) and the map's
   name under it. The texture is repainted from TerrainAt and PropAt only when
   the player steps onto another tile or the map changes, and drawn as one
   quad, so it costs the UI pass a handful of batches. */

#define MINIMAP_TILES 64
#define MINIMAP_PIXELS 160.f

// NOTE(zoubir): one colour per terrain kind, in terrain_kind order. A
// kind added without a colour here shows as transparent
global_variable u32 MinimapGroundColors[TerrainKind_Count] =
{
    UI_RGBA( 86, 140,  60, 255), // Grass
    UI_RGBA(128,  96,  64, 255), // Dirt
    UI_RGBA( 92,  70,  48, 255), // Mud
    UI_RGBA( 88, 150, 200, 255), // Shallow water
    UI_RGBA( 40,  80, 150, 255), // Deep water
    UI_RGBA( 90,  90,  96, 255), // Rock
    UI_RGBA(150, 148, 140, 255), // Ash
    UI_RGBA( 70,  66,  70, 255), // Basalt
    UI_RGBA( 40,  36,  40, 255), // Basalt wall
    UI_RGBA(230, 100,  30, 255), // Lava
    UI_RGBA(230, 234, 240, 255), // Snow
    UI_RGBA(170, 210, 230, 255), // Ice
    UI_RGBA(150, 146, 136, 255), // Stone floor
    UI_RGBA( 70,  70,  78, 255), // Stone wall
};

global_variable u32 MinimapPropColors[TerrainProp_Count] =
{
    0,                           // None: the ground shows
    UI_RGBA( 40,  90,  40, 255), // Tree
    UI_RGBA(120, 120, 120, 255), // Boulder
    UI_RGBA( 90,  70,  50, 255), // Dead tree
};

struct minimap
{
    u32 Texture;
    // NOTE(zoubir): what the texture shows; repainted when either changes
    bool32 Painted;
    u32 MapId;
    i32 OriginX;
    i32 OriginY;
    u32 Pixels[MINIMAP_TILES * MINIMAP_TILES];
};

// NOTE(zoubir): row 0 of the texture is the top (smallest Y) row of tiles.
// Past the edge of a bounded map is drawn as empty, not as wall
internal void
PaintMinimap(minimap *Minimap, open_gl *OpenGL, map_def *Map)
{
    for(i32 Row = 0; Row < MINIMAP_TILES; Row++)
    {
        for(i32 Column = 0; Column < MINIMAP_TILES; Column++)
        {
            i32 X = Minimap->OriginX + Column;
            i32 Y = Minimap->OriginY + Row;
            if (Map->Kind == MapKind_Bounded &&
                (X < 0 || Y < 0 || X >= (i32)Map->Width || Y >= (i32)Map->Height))
            {
                Minimap->Pixels[Row * MINIMAP_TILES + Column] = UI_COLOR_FIELD;
                continue;
            }
            u32 Color = MinimapPropColors[PropAt(Map, X, Y)];
            if (!Color)
            {
                Color = MinimapGroundColors[TerrainAt(Map, X, Y)];
            }
            Minimap->Pixels[Row * MINIMAP_TILES + Column] = Color;
        }
    }
    if (!Minimap->Texture)
    {
        OpenGL->glGenTextures(1, &Minimap->Texture);
    }
    OpenGL->glBindTexture(GL_TEXTURE_2D, Minimap->Texture);
    OpenGL->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, MINIMAP_TILES,
                         MINIMAP_TILES, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         Minimap->Pixels);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    Minimap->Painted = true;
}

internal void
DrawMinimap(render_context *RenderContext, app_state *AppState,
            u32 WindowWidth)
{
    world *World = &AppState->World;
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || !World->TileWidth || !AppState->OpenGL)
    {
        return;
    }
    if (!AppState->Minimap)
    {
        AppState->Minimap = AllocateStruct(&AppState->MemoryArena, minimap);
        *AppState->Minimap = {};
    }
    minimap *Minimap = AppState->Minimap;

    i32 Tile = (i32)World->TileWidth;
    i32 PlayerX = FloorDiv((i32)floorf(Player->Position.X), Tile);
    i32 PlayerY = FloorDiv((i32)floorf(Player->Position.Y), Tile);
    i32 OriginX = PlayerX - MINIMAP_TILES / 2;
    i32 OriginY = PlayerY - MINIMAP_TILES / 2;
    map_def *Map = GetMapDef((map_id)World->MapId);
    if (!Minimap->Painted || Minimap->MapId != World->MapId ||
        Minimap->OriginX != OriginX || Minimap->OriginY != OriginY)
    {
        Minimap->MapId = World->MapId;
        Minimap->OriginX = OriginX;
        Minimap->OriginY = OriginY;
        PaintMinimap(Minimap, AppState->OpenGL, Map);
    }

    float Size = MINIMAP_PIXELS;
    float Left = (float)WindowWidth - Size - UI_GAP_LARGE;
    float Top = UI_GAP_LARGE;
    float Scale = Size / (float)MINIMAP_TILES;
    DrawFilledRectangle(RenderContext, Left - 2.f, Top - 2.f, Size + 4.f,
                        Size + 4.f, UI_COLOR_PANEL, 0.f);
    // NOTE(zoubir): the texture pass reads row 0 at the top with these UVs
    BeginBatch(RenderContext, Minimap->Texture, 0.f,
               RenderContext->TextureProgram);
    RenderQuadTexture(RenderContext, Left, Top, Size, Size,
                      V4(0.f, 0.f, 1.f, 1.f), RGBA8_WHITE, 0.f);
    EndBatch(RenderContext);
    DrawRectangle(RenderContext, Left - 2.f, Top - 2.f, Size + 4.f,
                  Size + 4.f, UI_COLOR_BORDER, 0.f);

    // NOTE(zoubir): other players first, so the local one stays on top
    float TileSize = (float)Tile;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Other = Slot->Entity;
        if (!Slot->Active || !Other || SlotIndex == AppState->LocalPlayerIndex)
        {
            continue;
        }
        float DotX = (Other->Position.X / TileSize - (float)OriginX) * Scale;
        float DotY = (Other->Position.Y / TileSize - (float)OriginY) * Scale;
        if (DotX >= 0.f && DotY >= 0.f && DotX < Size && DotY < Size)
        {
            DrawFilledRectangle(RenderContext, Left + DotX - 2.f,
                                Top + DotY - 2.f, 4.f, 4.f, UI_COLOR_TEXT, 0.f);
        }
    }
    // NOTE(zoubir): landmarks on infinite maps, as rings: the pointer's
    // colour until this player has reached one, grey after
    if (Map->Kind == MapKind_Infinite)
    {
        i32 HomeX = FloorDiv(PlayerX, LANDMARK_REGION_TILES);
        i32 HomeY = FloorDiv(PlayerY, LANDMARK_REGION_TILES);
        for(i32 DY = -1; DY <= 1; DY++)
        {
            for(i32 DX = -1; DX <= 1; DX++)
            {
                landmark_spot Spot = GetRegionLandmark(Map, HomeX + DX, HomeY + DY);
                if (!Spot.Present)
                {
                    continue;
                }
                v3 Middle = GetLandmarkCenter(&Spot, Tile);
                float RingX = (Middle.X / TileSize - (float)OriginX) * Scale;
                float RingY = (Middle.Y / TileSize - (float)OriginY) * Scale;
                if (RingX >= 4.f && RingY >= 4.f &&
                    RingX < Size - 4.f && RingY < Size - 4.f)
                {
                    u32 Color = HasReachedLandmark(&LandmarkMemory, &Spot) ?
                        UI_COLOR_TEXT_MUTED : LANDMARK_POINTER_COLOR;
                    DrawRectangle(RenderContext, Left + RingX - 4.f,
                                  Top + RingY - 4.f, 8.f, 8.f, Color, 0.f);
                }
            }
        }
    }

    float Center = 0.5f * Size;
    DrawFilledRectangle(RenderContext, Left + Center - 3.f, Top + Center - 3.f,
                        6.f, 6.f, UI_COLOR_ACCENT, 0.f);

    // NOTE(zoubir): the map's name, and on infinite maps where you are
    char Text[64];
    if (Map->Kind == MapKind_Infinite)
    {
        snprintf(Text, sizeof(Text), "%s  %d, %d", Map->Name, PlayerX, PlayerY);
    }
    else
    {
        snprintf(Text, sizeof(Text), "%s", Map->Name);
    }
    UIText(RenderContext, AppState->Fonts.Small, Left + Size,
           Top + Size + UI_GAP_SMALL, Text, UI_COLOR_TEXT, UIAlign_Right);
}
