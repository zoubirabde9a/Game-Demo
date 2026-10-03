/* The game, as the platform layer sees it: two entry points called every
   frame. AppUpdateAndRender reads as the frame's outline; each step lives
   in its module (read the top of each *_module.cpp for what it does).
   AppGetSoundSamples mixes the sound the frame asked for. */

// NOTE(zoubir): network code for the online session. The dedicated server
// includes it itself before this file; the browser build has no UDP.
// (__EMSCRIPTEN__ comes from the compiler; COMPILER_EMSCRIPTEN is not
// defined yet at this point.)
#if !defined(NET_CLIENT_H) && !defined(__EMSCRIPTEN__)
#include "net/protocol.cpp"
#include "net/socket.cpp"
#include "net/client.cpp"
#endif

#include "app.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "time.h"

#include "engine/engine_module.cpp"

// NOTE(zoubir): the ids of the immediate-mode widgets, used by client startup
#include "ui/ui_ids.h"

// NOTE(zoubir): one line per module; a module lists its own files, so a new
// file is added there, not here. Read the top of each for what it does.
#include "sim/sim_module.cpp"
#include "client/client_module.cpp"
#include "ui/ui_module.cpp"

#define UI_PASS_MAX_BATCHES 4096

extern "C" APP_UPDATE_AND_RENDER(AppUpdateAndRender)
{
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    transient_state *TransientState = (transient_state *)Memory->TransientStorage;
    memory_arena *TransientArena = &TransientState->MemoryArena;
    render_context *RenderContext = &Thread->RenderContext;
    open_gl *OpenGL = RenderContext->OpenGL;
    Assert(Memory->PermanentStorageSize > sizeof(app_state));
    Assert(&Input->controllers[0].terminator - &Input->controllers[0].buttons[0] ==
           ArrayCount(Input->controllers[0].buttons));

    if (!AppState->IsInitialized)
    {
        StartClient(AppState, TransientState, Memory, Thread);
    }
    LoadOpenglTexturesFromQueue(&AppState->Assets, OpenGL,
                                &AppState->OpenglTextureQueue);
    temporary_memory FrameMemory = BeginTemporaryMemory(TransientArena);

    // Screen: pixel coordinates, origin top left.
    mat4 ProjectionMatrix = OrthoMatrix(0.f, (float)Window->Width,
                                        (float)Window->Height, 0.f, 0.f, 100000.f);
    RenderContext->TextureProgram.ProjectionMatrix = &ProjectionMatrix;
    RenderContext->LineProgram.ProjectionMatrix = &ProjectionMatrix;
    OpenGL->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    OpenGL->glClearColor(1.0f, 0.5f, 0.5f, 1.0f);

    // Input, and the camera on the local player before it moves.
    // While a screen such as the connect screen is open, keys type into it.
    bool32 KeysToUi = ConnectScreenTakesInput(AppState);
    AppState->Players[AppState->LocalPlayerIndex].Input =
        KeysToUi ? player_input{} : ReadKeyboardPlayerInput(Input);
    if (!KeysToUi && Input->ButtonJ.Pressed)
    {
        PlaySound(AppState, {AssetType_BattleTheme});
    }
    v3 CameraOffset = UpdateCamera(AppState, Window);

    // World: run it (locally, or from the server's snapshot), then draw it.
    render_program TextureProgram = RenderContext->TextureProgram;
    BeginWorldPass(RenderContext, TransientArena, &AppState->World, Window);
    DrawTileMap(RenderContext, AppState, TextureProgram, CameraOffset, Window);
    UpdateOnlineSession(AppState->Online, Input, KeysToUi);
    RunWorldTick(AppState, &AppState->MemoryArena, Input->DeltaTime);
    PlaySimEvents(AppState);
    DrawWorldEntities(RenderContext, AppState, &AppState->Assets,
                      TextureProgram, CameraOffset);
    RenderFlush(RenderContext);

    // Screens over the world.
    ui_context *UIContext = AppState->UIContext;
    // NOTE(zoubir): room for this many draw batches (and 6 vertices each)
    // on top of the world. Text and monster telegraphs use one batch per
    // shape, and 200 ran out with a few monsters winding up at once.
    UIBegin(RenderContext, TransientArena, Input, AppState, UIContext,
            UI_PASS_MAX_BATCHES, 4);
    DrawMonsterTelegraphs(RenderContext, &AppState->World, CameraOffset);
    DrawHud(RenderContext, AppState, CameraOffset);
    DrawRespawnCountdown(RenderContext, AppState, Window->Width, Window->Height);
    if (Input->TabButton.EndedDown)
    {
        DrawScoreboard(RenderContext, AppState, Window->Width, Window->Height);
    }
    DoTileEditor(RenderContext, AppState, UIContext, Input, Window,
                 TextureProgram, CameraOffset);
    DoConnectScreen(RenderContext, AppState, UIContext, Input,
                    Window->Width, Window->Height);
    UIEnd(UIContext);

    EndTemporaryMemory(FrameMemory);
    EvictAssetsAsNecessary(OpenGL, &AppState->Assets);
}

extern "C" APP_GET_SOUND_SAMPLES(AppGetSoundSamples)
{
    app_state *AppState = (app_state *)Memory->PermanentStorage;
    transient_state *TransientState = (transient_state *)Memory->TransientStorage;
    memory_arena *Arena = &TransientState->MemoryArena;

    temporary_memory TempMem = BeginTemporaryMemory(Arena);
    OutputAudio(&AppState->AudioState, SoundBuffer, Arena);
    EndTemporaryMemory(TempMem);
}
