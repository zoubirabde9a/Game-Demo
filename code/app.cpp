/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */


#include "app.h"
#include "stdio.h"
#include "string.h"

#include "random.cpp"
#include "utility.cpp"

#include "asset.cpp"
#include "render.cpp"
#include "ui.cpp"
#include "world.cpp"
#include "entity.cpp"
#include "opengl.cpp"
#include "audio.cpp"
#include "sim/collision_rules.cpp"
#include "sim/animations.cpp"
#include "sim/monster_kinds.cpp"
#include "sim/spawn.cpp"
#include "sim/players.cpp"
#include "sim/arena.cpp"
#include "sim/abilities.cpp"
#include "sim/monster_population.cpp"
#include "sim/update.cpp"
#include "sim/simulate.cpp"
#include "client/draw_entities.cpp"
#include "client/play_events.cpp"
#include "ui/hud.cpp"
#include "client/keyboard_input.cpp"

#include "app_ui.h"



render_vertex VertexC(float X, float Y, float Z,
                      float U, float V)
 {
     
     render_vertex Result;
     Result.X = X;
     Result.Y = Y;
     Result.Z = Z;
     Result.Color = RGBA8_WHITE;
     Result.U = U;
     Result.V = V;
     
     return Result;
 }



//TODO(zoubir): check if its 3D compatible
inline v3
CenterCamera(v3 Position, float MinX, float MinY,
             float MaxX, float MaxY, u32 WindowWidth,
             u32 WindowHeight)
{
    v3 Result = Position;
    
    Result.X -= (float)(WindowWidth / 2);
    Result.Y -= (float)(WindowHeight / 2);
    
    Result.X = Minimum(Result.X, MaxX - WindowWidth);
    Result.Y = Minimum(Result.Y, MaxY - WindowHeight);
    
    Result.X = Maximum(Result.X, MinX);
    Result.Y = Maximum(Result.Y, MinY);
    
    return Result;
}


//TODO(zoubir): remove the collision and tiles in
// world_position
extern "C" APP_UPDATE_AND_RENDER(AppUpdateAndRender)
{
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    transient_state *TransientState = (transient_state *)Memory->TransientStorage;
    memory_arena *MemoryArena = &AppState->MemoryArena;
    memory_arena *TransientArena = &TransientState->MemoryArena;
    memory_arena *ConstantsArena = &AppState->ConstantsArena;    
    
    assets *Assets = &AppState->Assets;
    audio_state *AudioState = &AppState->AudioState;
    render_context *RenderContext = &Thread->RenderContext;
    open_gl *OpenGL = RenderContext->OpenGL;

    static u32 VAO = 0;
    static u32 VBO = 0;
    static loaded_texture TextureL = {};
    
    if (!AppState->IsInitialized)
    {
        Platform = Memory->PlatformApi;
        AppState->OpenGL = RenderContext->OpenGL;
        
        InitializeArena(MemoryArena,
                        (memory_index *)(AppState + 1),
                        Memory->PermanentStorageSize - sizeof(app_state));
        
        InitializeArena(TransientArena,
                        (memory_index *)(TransientState + 1),
                        Memory->TransientStorageSize - sizeof(transient_state));
        SubArena(ConstantsArena, MemoryArena, Kilobytes(64));
        AppState->WorkQueue = Memory->WorkQueue;
        InitializeAudio(AudioState);        
        AppInitOpenGL(TransientArena, AppState, Thread, Memory);
        VAO = RenderContext->VAO;
        VBO = RenderContext->VBO;
#if 1
        u8 *C = (u8 *)malloc(1024 * 1024 * 4);
        for(u32 Index = 0;
            Index < 1024 * 1024 * 4;
            Index++)
        {
            C[Index] = 255;
        }
        TextureL = LoadOpenglTexture(Assets, OpenGL, 2 << 6, (2 << 6) + 1, GL_RGBA,
                                     (2 << 6) * ((2 << 6) + 1) * 4, C, TEXTURE_SOFT_FILTER);        
        free(C);
        
#endif
        InitializeAssets(Assets, OpenGL, AppState, MemoryArena);
                               
        temporary_memory TempMem = BeginTemporaryMemory(TransientArena);
        // Texture Loading        
        AppState->TextureCache = TextureCacheCreate(MemoryArena, 8, 20 * 8);

        AppState->DefaultFont =
            CreateFont(OpenGL, MemoryArena,
                       24.f, 512, 512,
                       "c:/windows/fonts/times.ttf");

        AppState->UIContext =
            UIContextCreate(MemoryArena, UI_COUNT);
        SetupCollisionVolumes(AppState, ConstantsArena);
        SetupAnimationSets(AppState, ConstantsArena);
        
        BuildArena(AppState, MemoryArena);
        SetupCollisionTable(AppState);

        AppState->Monsters =
            CreateMonsterPopulation(MemoryArena, MONSTER_POPULATION, 1337);
        FillMonsterPopulation(AppState, &AppState->World, MemoryArena,
                              AppState->Monsters);
        EndTemporaryMemory(TempMem);
        AppState->IsInitialized = true;
        
    }
    
    render_program *TextureProgram = &RenderContext->TextureProgram;
    render_program *LineProgram = &RenderContext->LineProgram;

    LoadOpenglTexturesFromQueue(Assets, OpenGL, &AppState->OpenglTextureQueue);
    
    ui_context *UIContext = AppState->UIContext;
    
    temporary_memory FrameTemporaryMemory =
        BeginTemporaryMemory(TransientArena);
    
    texture_cache *TextureCache = AppState->TextureCache;

//    loaded_texture *ZoubirTexture = GetTexture(Assets, OpenGL,
//                                               AppState, {AssetType_Zoubir});
    
    font *DefaultFont = AppState->DefaultFont;

    float Right = (float)Window->Width;
    float Left = 0.f;
    float Top = 0.f;
    float Bottom = (float)Window->Height;
    float Far = 100000.f;
    float Near = 0.f;

    //https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/opengl-perspective-projection-matrix
    mat4 ProjectionMatrix = OrthoMatrix(Left, Right, Bottom, Top, Near, Far);

#if 0
    static float A = 0.f;

    A+= 0.01f;

    mat4 ProjectionMatrix =
        PerspectiveMatrix(Window->Width / (float)Window->Height, -Pi32 * 0.6f, 0.f, 1.f) *
        TranslationMatrix(-0.5f, -0.5f, 0.f) *
        ScaleMatrix(-1.f / Window->Width * 2, 1.f / Window->Height * 2, 0.001f);
#endif
    
    TextureProgram->ProjectionMatrix = &ProjectionMatrix;
    LineProgram->ProjectionMatrix = &ProjectionMatrix;
    
    Assert(&Input->controllers[0].terminator - &Input->controllers[0].buttons[0] ==
           ArrayCount(Input->controllers[0].buttons));
    Assert(Memory->PermanentStorageSize > sizeof(app_state));

    for(int controllerIndex = 0;
        controllerIndex < ArrayCount(Input->controllers);
        controllerIndex++)
    {
        app_controller_input *thisController = GetController(Input, controllerIndex);

    }
//    glViewport(0, 0, Window->Width, Window->Height);
    
    
    OpenGL->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OpenGL->glClearColor(1.0f, 0.5f, 0.5f, 1.0f);


#if 1
    world *World = &AppState->World;
    tile_map *TileMap = &World->TileMap;
    world_entity *Player = GetLocalPlayer(AppState);
    AppState->Players[AppState->LocalPlayerIndex].Input =
        ReadKeyboardPlayerInput(Input);
    if (Input->ButtonJ.Pressed)
    {
        PlaySound(AppState, {AssetType_BattleTheme});
    }

    float TileMapWidth = (float)(World->NumTilesX * World->TileWidth);
    float TileMapHeight = (float)(World->NumTilesY * World->TileHeight);
    u32 WindowHalfWidth = Window->Width / 2;
    u32 WindowHalfHeight = Window->Height / 2;

    AppState->TargetCamera =
        CenterCamera(Player->Position,
                     0.f, 0.f,
                     TileMapWidth,
                     TileMapHeight,
                     Window->Width,
                     Window->Height);

    #if 0
    float LerpValue = 0.95f;
    float DeltaTime = Input->DeltaTime;
    v3 CameraOffset = AppState->CameraOffset =
        AppState->CameraOffset +
        (AppState->TargetCamera - AppState->CameraOffset) *
        LerpValue * DeltaTime;
    #else
    v3 CameraOffset = AppState->CameraOffset =
        AppState->TargetCamera;
    #endif
    
    CameraOffset.X = (float)((u32)CameraOffset.X);
    CameraOffset.Y = (float)((u32)CameraOffset.Y);
    
    v3 CameraDims = {(float)Window->Width,
                     (float)Window->Height,
                     1.f};
    v3 CameraGrabEntitySafetyMargin = {32, 32};
    v3 CameraMinPos = CameraOffset;
    v3 CameraMaxPos = CameraOffset + CameraDims;
    CameraMinPos -= CameraGrabEntitySafetyMargin;
    CameraMaxPos += CameraGrabEntitySafetyMargin;
    
    u32 MinChunkX;
    u32 MinChunkY;
    u32 MinChunkZ;
    
    u32 MaxChunkX;
    u32 MaxChunkY;
    u32 MaxChunkZ;
    
    rectangle3 Box = RectMinMax(CameraMinPos,
                                CameraMaxPos);
    GetChunksFromBox(World,
                     Box,
                     &MinChunkX, &MinChunkY, &MinChunkZ,
                     &MaxChunkX, &MaxChunkY, &MaxChunkZ);
    
    
    #if 1
    {
    u32 BatchesCount =(World->NumTilesX *
                       World->NumTilesY + World->EntityCount * 2) ;
    u32 VerticesCount = 6 * BatchesCount;
    SetupBatchRenderer(RenderContext, TransientArena, BatchesCount);
    RenderBegin(RenderContext, VerticesCount, RENDER_ORDER_BACK_TO_FRONT);
    
    loaded_texture *TileMapTexture = GetTexture(Assets, OpenGL,
                                                AppState, TileMap->Texture);
    // Drawing Tile Map
#if 1
    if (TileMapTexture)
    {
    BeginBatch(RenderContext, TileMapTexture->ID, 0.f, *TextureProgram);    
    u32 TextureTileNumX = TileMapTexture->Width / 32;
    u32 TextureTileNumY = TileMapTexture->Height / 32;
    for(u32 TileYIndex = 0;
        TileYIndex < World->NumTilesY;
        TileYIndex++)
    {
        for(u32 TileXIndex = 0;
            TileXIndex < World->NumTilesX;
            TileXIndex++)
        {
            tile *ThisTile =
                &TileMap->Tiles[TileXIndex +
                              TileYIndex * World->NumTilesX];
            
            v4 TileUvs = GetTextureUvsFromIndex(TileMapTexture->Width,
                                            TileMapTexture->Height,
                                            TextureTileNumX,
                                            TextureTileNumY,
                                            ThisTile->Index);
            RenderQuadTexture(RenderContext,
                              (float)TileXIndex * World->TileWidth -
                              CameraOffset.X,
                              (float)TileYIndex * World->TileHeight -
                              CameraOffset.Y,
                              (float)World->TileWidth,
                              (float)World->TileHeight,
                              TileUvs,
                              RGBA8_WHITE,
                              0.f);
        }
    }    
    #endif
            EndBatch(RenderContext);
    }
    }
    #endif
    
#if 1
    SimulateTick(AppState, MemoryArena, Input->DeltaTime);
    PlaySimEvents(AppState);
    DrawWorldEntities(RenderContext, AppState, Assets, *TextureProgram,
                      CameraOffset);
    RenderFlush(RenderContext);

#endif



//    EndTemporaryMemory(FrameTemporaryMemory);
//    FrameTemporaryMemory = BeginTemporaryMemory(TransientArena);
    

    if (Input->ButtonF3.Pressed)
    {
        AppState->TileEditing = !AppState->TileEditing;
    }
#if 1
    u32 NumberOfElements = 200;
    UIBegin(RenderContext, TransientArena, Input, AppState,
            UIContext, NumberOfElements, 4);
    DrawHud(RenderContext, AppState, CameraOffset);
    if (AppState->TileEditing)
    {
        float ContainerWidth = 300;
        float ContainerHeight = 600;
        float ContainerX = Window->Width - ContainerWidth - 60;
        float ContainerY = (Window->Height - ContainerHeight) / 2;
        
        BeginContainer(UIContext, ContainerX, ContainerY,
                       ContainerWidth, ContainerHeight);

        DrawRectangle(RenderContext, ContainerX, ContainerY,
                      ContainerWidth, ContainerHeight,
                      RGBA8_YELLOW, 0.f);

        ui_state *TilePickerWidget = &AppState->TilePickerWidget;
        DoTilePickerWidget(&AppState->TilePickerWidget, AppState,
                           UIContext, 0.f, 0.f, 200.f, 200.f,
                           GetTexture(Assets, OpenGL,
                                      AppState, {AssetType_TileMap}), 32, 32,
                           8, 200);
        float MouseXF = (float)Input->MouseX;
        float MouseYF = (float)Input->MouseY;
        
        float TileWidthF = (float)World->TileWidth;
        float TileHeightF = (float)World->TileHeight;

        float X = (float)((u32)((MouseXF + CameraOffset.X) /
                                TileWidthF));
        float Y = (float)((u32)((MouseYF + CameraOffset.Y) /
                                TileHeightF));
        X *= TileWidthF;
        Y *= TileHeightF;
        X -= CameraOffset.X;
        Y -= CameraOffset.Y;

        loaded_texture *TileMapTexture = AppState->TilePickerWidget.TileMap;
        if (TileMapTexture)
        {
        for(u32 IndexY = 0;
            IndexY < TilePickerWidget->SelectedTileCountY;
            IndexY++)
        {
            for(u32 IndexX = 0;
                IndexX < TilePickerWidget->SelectedTileCountX;
                IndexX++)
            {
                u32 IndexInTexture =
                    AppState->TilePickerWidget.SelectedTileIndices[IndexX]
                    [IndexY];
                u32 NumTilesX = AppState->TilePickerWidget.TileMapNumTilesX;
                u32 NumTilesY = AppState->TilePickerWidget.TileMapNumTilesY;
        
                v4 Uvs = GetTextureUvsFromIndex(TileMapTexture->Width,
                                                TileMapTexture->Height,
                                                NumTilesX, NumTilesY,
                                                IndexInTexture);

                u32 TileOffsetX = IndexX * World->TileWidth;
                u32 TileOffsetY = IndexY * World->TileHeight;
                
                BeginBatch(RenderContext, TileMapTexture->ID, 0.f, *TextureProgram);        
                RenderQuadTexture(RenderContext,
                                  X + IndexX * TileWidthF,
                                  Y + IndexY * TileHeightF,
                                  TileWidthF,
                                  TileHeightF,
                                  Uvs, RGBA8_WHITE, 0.f);
                EndBatch(RenderContext);
                if (Input->LeftButton.EndedDown &&
                    !IsMouseOnRectangle(Input->MouseX, Input->MouseY,
                                        ContainerX, ContainerY,
                                        ContainerWidth,
                                        ContainerHeight))
                {
                    u32 TileXIndex =
                        ((u32)(CameraOffset.X + Input->MouseX + TileOffsetX) /
                         World->TileWidth);
                    u32 TileYIndex =
                        ((u32)(CameraOffset.Y + Input->MouseY + TileOffsetY) /
                         World->TileHeight);
                    TileMap->Tiles[TileXIndex +
                                   (TileYIndex * World->NumTilesX)].Index =
                        IndexInTexture;
                }
            }
        }
        }
        float TilesRectangleWidth =
            (float)(TilePickerWidget->SelectedTileCountX
                    * World->TileWidth);
        float TilesRectangleHeight =
            (float)(TilePickerWidget->SelectedTileCountY *
                    World->TileHeight);
        DrawRectangle(RenderContext, X, Y,
                      TilesRectangleWidth,
                      TilesRectangleHeight,
                      RGBA8_YELLOW, 0.f);
        EndContainer(UIContext);
        
    }
    #if 0
    for(int Number = 0;
        Number < 9;
        Number++)
    {
        if (Input->NumbersButtons[Number].EndedDown)
        {
            ui_state *Button1 = UIContextGetState(UIContext, UI_BUTTON1);
            if (DoButton(Button1, AppState, UIContext, -100 + Number * 50.f, 10.f, 200.f,
                         30.f, "Hjglo"))
            {
        
            }
        }
    }
    if (DoButton(UI_BUTTON2, AppState, UIContext, 400.f, 0.f, 200.f,
                       30.f, "Hjglo"))
    {
        if (DoButton(UI_BUTTON3, AppState, UIContext, 200.f, 200.f,
                           33.f, 33.f, "Helgo"))
        {
            
        }        
    }

    DoEditBox(UI_EDITBOX1, AppState, UIContext, 200.f, 300.f,
              100.f, 33.f);

    DoEditBox(UI_EDITBOX2, AppState, UIContext, 240.f, 300.f,
              100.f, 33.f);
    //TODO(zoubir): remove all local_persist
    //bad for hot loading
    local_persist ui_state EditBox3 = {};
    DoEditBox(&EditBox3, AppState, UIContext, 240.f, 200.f,
              100.f, 33.f);
    
    local_persist ui_state TextLabel = {};
    DoTextLabel(&TextLabel, AppState, UIContext, 240.f, 400.f,
                100.f, 33.f, "Hi Sir", TEXT_JUSTIFICATION_MIDDLE);
    #endif
    
    UIEnd(UIContext);
    
    for(int buttonIndex = 0;
        buttonIndex < ArrayCount(Input->mouseButtons);
        buttonIndex++)
    {
        if (Input->mouseButtons[buttonIndex].EndedDown)
        {
//            RenderPlayer(Buffer, 100 * buttonIndex, 100);
        }
    }

#endif
    #endif
    EndTemporaryMemory(FrameTemporaryMemory);

    
    EvictAssetsAsNecessary(OpenGL, &AppState->Assets);

}

extern "C" APP_GET_SOUND_SAMPLES(AppGetSoundSamples)
{
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    transient_state *TransientState = (transient_state *)Memory->TransientStorage;
    memory_arena *Arena = &TransientState->MemoryArena;

    audio_state *AudioState = &AppState->AudioState;
    
    temporary_memory TempMem = BeginTemporaryMemory(Arena);
    OutputAudio(AudioState, SoundBuffer, Arena);
    EndTemporaryMemory(TempMem);
}

#if 0
int WINAPI WinMain(      
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{  
  return 0;
}
#endif
