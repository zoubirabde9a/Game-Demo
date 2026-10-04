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

// NOTE(zoubir): the shared state, the engine core and the simulation: what
// the dedicated server builds too
#include "app_sim.cpp"
// NOTE(zoubir): the rest of the engine: rendering, assets, sound, widgets
#include "engine/engine_module.cpp"

// NOTE(zoubir): the ids of the immediate-mode widgets, used by client startup
#include "ui/ui_ids.h"

// NOTE(zoubir): one line per module; a module lists its own files, so a new
// file is added there, not here. Read the top of each for what it does.
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
    RenderBeginFrame(RenderContext, &ProjectionMatrix, Input->DeltaTime);
    OpenGL->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // NOTE(zoubir): dark, so a window bigger than a small map shows a border
    OpenGL->glClearColor(0.05f, 0.06f, 0.08f, 1.0f);

    // Input. While a screen such as the connect screen is open, keys type
    // into it.
    bool32 KeysToUi = ConnectScreenTakesInput(AppState);
    player_input *LocalInput = &AppState->Players[AppState->LocalPlayerIndex].Input;
    *LocalInput = KeysToUi ? player_input{} : ReadKeyboardPlayerInput(Input, AppState);
    if (!KeysToUi && Input->ButtonJ.Pressed)
    {
        PlaySound(AppState, {AssetType_BattleTheme});
    }

    // World: run it (locally, or from the server's snapshot), put the
    // camera on where the player is now, then draw it.
    app_input ServerInput = InputForServer(Input, LocalInput, KeysToUi);
    UpdateOnlineSession(AppState->Online, &ServerInput, KeysToUi, LocalInput->Aim);
    RunWorldTick(AppState, &AppState->WorldArena, Input->DeltaTime);
    PlaySimEvents(AppState, Input->DeltaTime);
    // NOTE(zoubir): the world is drawn zoomed in (client/camera.cpp):
    // View is the window measured in world units, and WorldProjection
    // maps it onto the whole window. Screens over it use ProjectionMatrix
    app_window View = GetWorldView(AppState, Window);
    mat4 WorldProjection = OrthoMatrix(0.f, (float)Window->Width / AppState->WorldZoom,
                                       (float)Window->Height / AppState->WorldZoom, 0.f,
                                       0.f, 100000.f);
    RenderSetProjection(RenderContext, &WorldProjection);
    v3 CameraOffset = UpdateCamera(AppState, &View, Input, !KeysToUi);
    render_program TextureProgram = RenderContext->TextureProgram;
    BeginWorldPass(RenderContext, TransientArena, &AppState->World, &View);
    DrawTileMap(RenderContext, AppState, TextureProgram, CameraOffset, &View);
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
    // NOTE(zoubir): every overlay and screen, one call per line, drawn in
    // order; a new one is added there, not here
#include "client/screen_pass.inc"
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
