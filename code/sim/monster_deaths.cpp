/* Monster deaths (monster_population.cpp): what a death effect does,
   run by the population the tick after the death, since DamageEntity
   cannot spawn entities itself. A Split kind pops smaller monsters out
   of the corpse; a Shatter kind bursts into shards
   (monster_abilities/waves_beams_shards.cpp). */

// NOTE(zoubir): children of a split spread evenly around the corpse,
// falling back to the corpse's own spot when the ring is blocked
internal void
SplitMonster(app_state *AppState, world *World, memory_arena *Arena,
             monster_death_record *Record, monster_def *Def)
{
    monster_def *ChildDef = GetMonsterDef(Def->SplitKind);
    entity_collision_volume_group *Volume = ChildDef->FlyHeight > 0.f ?
        AppState->BatCollision : AppState->PlayerCollision;
    float StartAngle = RandomBetween(&AppState->Monsters->Series, 0.f, 2.f * Pi32);
    for(u32 Child = 0; Child < Def->SplitCount; Child++)
    {
        float Angle = StartAngle + 2.f * Pi32 * (float)Child / (float)Def->SplitCount;
        v2 Out = V2(Cos(Angle), Sin(Angle));
        v3 Position = Record->Position;
        Position.XY += 20.f * Out;
        Position = OnGround(World, Position);
        if (!IsSpawnSpotFree(AppState, World, Position, Volume))
        {
            Position = OnGround(World, Record->Position);
            if (!IsSpawnSpotFree(AppState, World, Position, Volume))
            {
                continue;
            }
        }
        world_entity *Spawned = SpawnMonster(AppState, World, Arena, Position,
                                             Def->SplitKind);
        // NOTE(zoubir): an elite's children keep its affix
        ApplyEliteAffix(Spawned, Record->EliteAffix);
        // NOTE(zoubir): a little pop outward so the split reads
        Spawned->Velocity.XY = 180.f * Out;
    }
}

// NOTE(zoubir): in monster_abilities/waves_beams_shards.cpp, included later
internal void ShatterMonster(app_state *AppState, world *World, memory_arena *Arena,
                             monster_death_record *Record, monster_def *Def);

internal void
RunPendingMonsterDeaths(app_state *AppState, world *World, memory_arena *Arena,
                        monster_population *Population)
{
    for(u32 DeathIndex = 0;
        DeathIndex < Population->PendingDeathCount;
        DeathIndex++)
    {
        monster_death_record *Record = &Population->PendingDeaths[DeathIndex];
        monster_def *Def = GetMonsterDef(Record->Kind);
        switch(Def->DeathEffect)
        {
            case DeathEffect_Split:
            {
                SplitMonster(AppState, World, Arena, Record, Def);
            } break;

            case DeathEffect_Shatter:
            {
                ShatterMonster(AppState, World, Arena, Record, Def);
            } break;

            default:
            {
            } break;
        }
    }
    Population->PendingDeathCount = 0;
}
