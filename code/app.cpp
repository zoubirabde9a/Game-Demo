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
    FrameTimingStart(AppState);
    // NOTE(zoubir): the drawing clock, one a frame: water and lava frames,
    // ground shimmer, tree sway, offline weather. The simulation's own
    // counter left with SimulateTick, and this stood at 0 since
    AppState->UpdateID++;
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

    // Input. While a screen such as the connect screen or the options
    // menu is open, keys and clicks go to it; while a chat line is open,
    // the keys type into it and are gone from Input (ui/chat.cpp).
    bool32 Chatting = ChatTakesKeys(AppState, Input);
    bool32 KeysToUi = Chatting || ConnectScreenTakesInput(AppState) || AppState->OptionsOpen;
    player_input *LocalInput = &AppState->Players[AppState->LocalPlayerIndex].Input;
    *LocalInput = KeysToUi ? player_input{} : ReadKeyboardPlayerInput(Input, AppState);
    // NOTE(zoubir): a map vote comes from the menu, so even while it is
    // open; online it goes in the held buttons (client/vote_requests.cpp)
    if (!IsOnline(AppState->Online))
    {
        LocalInput->Vote = TakeOfflineVoteRequest(AppState);
    }
    if (!KeysToUi && Input->ButtonJ.Pressed)
    {
        PlaySound(AppState, {AssetType_BattleTheme});
    }

    // World: run it (locally, or from the server's snapshot), put the
    // camera on where the player is now, then draw it.
    app_input ServerInput = InputForServer(Input, AppState);
    UpdateOnlineSession(AppState->Online, &ServerInput, KeysToUi, LocalInput->Aim,
                        OnlineTalentBits(AppState, Input->DeltaTime) |
                        OnlineVoteBits(AppState, Input->DeltaTime) |
                        MoveNetButtons(LocalInput->Move),
                        LocalInput->Target, OnlineRoleRequest(AppState, Input->DeltaTime));
    RunWorldTick(AppState, &AppState->WorldArena, Input->DeltaTime);
    // NOTE(zoubir): the duel's final blow slows the rest of the frame's
    // effects with the world (client/final_blow.cpp)
    Input->DeltaTime *= RoundTimeScale(AppState);
    PlaySimEvents(AppState, Input->DeltaTime);
    UpdateRewindFx(AppState, Input->DeltaTime);
    FrameTimingMark(AppState, FramePart_Simulate);
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
    // NOTE(zoubir): while a time rewind shows, the world is drawn into a
    // texture and put on the window through its shader (client/rewind_fx/)
    BeginTimeWarp(RenderContext, AppState);
    // NOTE(zoubir): otherwise into a texture for the glow and colour grade
    // (client/world_grade.cpp)
    BeginWorldGrade(RenderContext, AppState, CameraOffset, &View, Window);
    BeginWorldPass(RenderContext, TransientArena, &AppState->World, &View);
    DrawTileMap(RenderContext, AppState, TextureProgram, CameraOffset, &View);
    FrameTimingMark(AppState, FramePart_Ground);
    DrawWorldEntities(RenderContext, AppState, &AppState->Assets,
                      TextureProgram, CameraOffset);
    FrameTimingMark(AppState, FramePart_Entities);
    RenderFlush(RenderContext);
    FrameTimingMark(AppState, FramePart_Flush);
    EndWorldGrade(RenderContext, AppState, TransientArena, &ProjectionMatrix, Window);
    EndTimeWarp(RenderContext, AppState, TransientArena, &ProjectionMatrix,
                CameraOffset, Window);
    FrameTimingMark(AppState, FramePart_Grade);

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
    FrameTimingMark(AppState, FramePart_Screens);
    FrameTimingEnd(AppState);
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
