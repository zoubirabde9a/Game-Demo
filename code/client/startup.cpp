/* Client startup: everything the first frame sets up before the game can
   run. Memory arenas, audio, OpenGL, assets and the font, the simulation
   with the local player and familiar, and the online session if a server
   address is configured. */

internal void
StartClient(app_state *AppState, transient_state *TransientState,
            app_memory *Memory, thread_context *Thread)
{
    memory_arena *MemoryArena = &AppState->MemoryArena;
    memory_arena *TransientArena = &TransientState->MemoryArena;
    memory_arena *ConstantsArena = &AppState->ConstantsArena;
    render_context *RenderContext = &Thread->RenderContext;
    open_gl *OpenGL = RenderContext->OpenGL;
    assets *Assets = &AppState->Assets;

    Platform = Memory->PlatformApi;
    AppState->OpenGL = OpenGL;
    InitializeArena(MemoryArena, (memory_index *)(AppState + 1),
                    Memory->PermanentStorageSize - sizeof(app_state));
    InitializeArena(TransientArena, (memory_index *)(TransientState + 1),
                    Memory->TransientStorageSize - sizeof(transient_state));
    SubArena(ConstantsArena, MemoryArena, Kilobytes(64));
    AppState->WorkQueue = Memory->WorkQueue;
    InitializeAudio(&AppState->AudioState);
    AppInitOpenGL(TransientArena, AppState, Thread, Memory);

    // NOTE(zoubir): a plain white texture nothing refers to by name; kept
    // because removing it would change the OpenGL ids later textures get
    u32 WhiteWidth = 2 << 6;
    u32 WhiteHeight = (2 << 6) + 1;
    u8 *White = (u8 *)malloc(WhiteWidth * WhiteHeight * 4);
    memset(White, 255, WhiteWidth * WhiteHeight * 4);
    LoadOpenglTexture(Assets, OpenGL, WhiteWidth, WhiteHeight, GL_RGBA,
                      WhiteWidth * WhiteHeight * 4, White, TEXTURE_SOFT_FILTER);
    free(White);

    InitializeAssets(Assets, OpenGL, AppState, MemoryArena);
    AddMonsterTextures(Assets, OpenGL, TransientArena);

    temporary_memory TempMem = BeginTemporaryMemory(TransientArena);
    AppState->TextureCache = TextureCacheCreate(MemoryArena, 8, 20 * 8);
    AppState->DefaultFont = CreateFont(OpenGL, MemoryArena, 24.f, 512, 512,
                                       "c:/windows/fonts/times.ttf");
    AppState->UIContext = UIContextCreate(MemoryArena, UI_COUNT);

    // NOTE(zoubir): developer switch until the server picks the map:
    // GAME_MAP=keep (or any map name) chooses what offline play builds
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once at startup, never kept
#endif
    AppState->World.MapId = FindMapByName(getenv("GAME_MAP"), MapId_Arena);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    InitSimulation(AppState, MemoryArena, ConstantsArena);
    AppState->LocalPlayerIndex = 0;
    world_entity *Player =
        AddPlayerToSlot(AppState, &AppState->World, MemoryArena,
                        AppState->LocalPlayerIndex,
                        PlayerSpawnPosition(&AppState->World,
                                            AppState->LocalPlayerIndex));
    AddFamiliar(AppState, &AppState->World, MemoryArena, Player);
    AppState->Online = StartOnlineSession(MemoryArena);
    EndTemporaryMemory(TempMem);
    AppState->IsInitialized = true;
}
