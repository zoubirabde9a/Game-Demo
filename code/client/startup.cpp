/* Client startup: everything the first frame sets up before the game can
   run. Memory arenas, audio, OpenGL, assets and the fonts, the simulation
   with the local player and familiar, and the online session if a server
   address is configured. */

// NOTE(zoubir): developer builds: GAME_DUMMY=<monster kind number> puts
// a sturdy monster of that kind just right of the player when an offline
// map starts, so a scripted screenshot (misc\screenshot.bat) has
// something to hit in a known place
#define DUMMY_ENV "GAME_DUMMY"
#define DUMMY_OFFSET 40.f
#define DUMMY_HP 2000.f

internal void
AddDeveloperDummy(app_state *AppState, memory_arena *Arena, world_entity *Player)
{
#if APP_DEV
#pragma warning(push)
#pragma warning(disable: 4996)
    char *Value = getenv(DUMMY_ENV);
#pragma warning(pop)
    if (!Value || !Value[0])
    {
        return;
    }
    u32 Kind = (u32)atoi(Value);
    if (Kind >= MonsterKind_Count)
    {
        Kind = 0;
    }
    v3 Position = Player->Position;
    Position.X += DUMMY_OFFSET;
    world_entity *Dummy = AddMonster(AppState, &AppState->World, Arena,
                                     Position, (monster_kind)Kind);
    if (Dummy)
    {
        Dummy->MaxHp = Dummy->Hp = DUMMY_HP;
    }
#endif
}

// NOTE(zoubir): the local player at its map spawn, with its familiar
internal void
AddLocalPlayer(app_state *AppState, memory_arena *Arena)
{
    AppState->LocalPlayerIndex = 0;
    world_entity *Player =
        AddPlayerToSlot(AppState, &AppState->World, Arena,
                        AppState->LocalPlayerIndex,
                        PlayerSpawnPosition(&AppState->World,
                                            AppState->LocalPlayerIndex));
    AddFamiliar(AppState, &AppState->World, Arena, Player);
    AddDeveloperDummy(AppState, Arena, Player);
    ApplyDeveloperRole(AppState);
}

// NOTE(zoubir): offline only. Throws the world away and starts MapId
// fresh, monsters and all, as the map picker on the connect screen does
internal void
StartOfflineMap(app_state *AppState, u32 MapId)
{
    memory_arena *Arena = &AppState->WorldArena;
    RebuildWorldForMap(AppState, Arena, MapId);
    FillMonsterPopulation(AppState, &AppState->World, Arena,
                          AppState->Monsters);
    AddLocalPlayer(AppState, Arena);
}

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
    // NOTE(zoubir): a world uses well under 1 MB (300 KB for an infinite map);
    // the assets take 32 MB of the 64 after this
    SubArena(&AppState->WorldArena, MemoryArena, Megabytes(16));
    AppState->WorkQueue = Memory->WorkQueue;
    InitializeAudio(&AppState->AudioState);
    AppInitOpenGL(TransientArena, AppState, Thread, Memory);

    // NOTE(zoubir): a plain white texture nothing refers to by name; kept
    // because removing it would change the OpenGL ids later textures get.
    // Its pixels are scratch memory: it used to be malloc'd and then given
    // to LoadOpenglTexture, which VirtualFrees what it uploads, before the
    // free; now and then the VirtualFree took the heap's pages and the free
    // crashed (about one launch in fifteen)
    u32 WhiteWidth = 2 << 6;
    u32 WhiteHeight = (2 << 6) + 1;
    temporary_memory WhiteMemory = BeginTemporaryMemory(TransientArena);
    u8 *White = AllocateArray(TransientArena, WhiteWidth * WhiteHeight * 4, u8);
    memset(White, 255, WhiteWidth * WhiteHeight * 4);
    UploadOpenglTexture(OpenGL, WhiteWidth, WhiteHeight, GL_RGBA, White, TEXTURE_SOFT_FILTER);
    EndTemporaryMemory(WhiteMemory);

    InitializeAssets(Assets, OpenGL, AppState, MemoryArena);
    AddMonsterTextures(Assets, OpenGL, TransientArena);

    temporary_memory TempMem = BeginTemporaryMemory(TransientArena);
    AppState->TextureCache = TextureCacheCreate(MemoryArena, 8, 20 * 8);
    LoadUIFonts(&AppState->Fonts, OpenGL, MemoryArena);
    AppState->DefaultFont = AppState->Fonts.Body;
    AppState->UIContext = UIContextCreate(MemoryArena, UI_COUNT);

    // NOTE(zoubir): developer switch until the server picks the map:
    // GAME_MAP=keep (or any map name) chooses what offline play builds
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once at startup, never kept
#endif
    AppState->World.MapId = FindMapByName(getenv("GAME_MAP"), MapId_Arena);
#if APP_DEV
    // NOTE(zoubir): GAME_RULES=classic plays offline with every ability
    // unlocked, so a scripted screenshot can show the sword without
    // spending talent points first
    char *Rules = getenv("GAME_RULES");
    if (Rules && strcmp(Rules, "classic") == 0)
    {
        GameRules = ClassicRules;
    }
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    InitSimulation(AppState, &AppState->WorldArena, ConstantsArena);
    AddLocalPlayer(AppState, &AppState->WorldArena);
    AppState->Online = StartOnlineSession(MemoryArena, ONLINE_DEFAULT_SERVER);
    AppState->KeyboardLayout = ReadSavedKeyboardLayout();
    GlobalKeyboardLayout = (keyboard_layout)AppState->KeyboardLayout;
    EndTemporaryMemory(TempMem);
    AppState->IsInitialized = true;
}
