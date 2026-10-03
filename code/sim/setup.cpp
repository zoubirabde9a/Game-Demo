/* InitSimulation: builds a ready-to-tick world with no players in it:
   collision shapes, animation tables, the arena and its monsters. Needs no
   window, textures or sound, so the game client and the dedicated server
   both call it. Players are added afterwards with AddPlayerToSlot. */

internal void
InitSimulation(app_state *AppState, memory_arena *MemoryArena,
               memory_arena *ConstantsArena)
{
    SetupCollisionVolumes(AppState, ConstantsArena);
    SetupAnimationSets(AppState, ConstantsArena);
    BuildArena(AppState, MemoryArena);
    SetupCollisionTable(AppState);

    AppState->Monsters =
        CreateMonsterPopulation(MemoryArena, MONSTER_POPULATION, 1337);
    FillMonsterPopulation(AppState, &AppState->World, MemoryArena,
                          AppState->Monsters);
}
