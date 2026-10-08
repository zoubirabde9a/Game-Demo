/* Boss wards (dungeon.cpp): a boss that raises Aurora Pylons
   (boss_scripts.cpp) takes nothing while one of them stands. The party
   has to turn from the boss and break the pylons, which sweep the room
   with beams of their own, while the boss keeps fighting. A hit on the
   warded boss shows Blocked; clients draw the ward as a shell of light
   with a beam from each pylon (client/dungeon/rift_fx.cpp), worked out
   from the pylons in the snapshot, so nothing new goes on the wire. */

// NOTE(zoubir): whether any Aurora Pylon stands in the world; only boss
// scripts raise them, so on a dungeon map they belong to the fight
inline bool32
AnyPylonStands(world *World)
{
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster &&
            Entity->MonsterKind == MonsterKind_AuroraPylon && Entity->Hp > 0.f)
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): Target is the fight's boss and a pylon wards it
internal bool32
IsWardedBoss(app_state *AppState, world_entity *Target)
{
    dungeon_run *Run = AppState->Dungeon;
    bool32 Result = Run && Run->BossSerial &&
        Target->Type == EntityType_Monster &&
        Target->MonsterSerial == Run->BossSerial &&
        AnyPylonStands(&AppState->World);
    return Result;
}

// NOTE(zoubir): the hit glances off: the shell glints and Blocked shows
inline void
WardDeflects(app_state *AppState, world_entity *Target)
{
    // NOTE(zoubir): as long as a shell's glint (BLOCK_FLASH_SECONDS)
    Target->BlockFlash = 0.2f;
    v3 Chest = Target->Position;
    Chest.Z += 16.f;
    EmitBurst(&AppState->Events, SimBurst_Blocked, SIM_NOBODY, Chest);
}
