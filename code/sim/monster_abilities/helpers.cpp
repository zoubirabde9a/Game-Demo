/* Small shared helpers: the monster random series, arena bounds,
   staggered cooldowns at spawn, and switching ability phase. */

inline random_series *
GetMonsterSeries(app_state *AppState)
{
    random_series *Result = 0;
    if (AppState->Monsters)
    {
        Result = &AppState->Monsters->Series;
    }
    return Result;
}

inline float
MonsterRandomBetween(app_state *AppState, float Min, float Max)
{
    random_series *Series = GetMonsterSeries(AppState);
    float Result = Series ? RandomBetween(Series, Min, Max) : 0.5f * (Min + Max);
    return Result;
}

inline bool32
IsInsideArena(world *World, v2 Position, float Margin)
{
    float MapWidth = (float)(World->NumTilesX * World->TileWidth);
    float MapHeight = (float)(World->NumTilesY * World->TileHeight);
    bool32 Result = Position.X > Margin && Position.Y > Margin &&
        Position.X < MapWidth - Margin && Position.Y < MapHeight - Margin;
    return Result;
}

// NOTE(zoubir): a monster that just spawned does not fire everything at
// once; each ability starts part way through its cooldown
internal void
StaggerMonsterCooldowns(app_state *AppState, world_entity *Entity)
{
    monster_def *Def = GetMonsterDef(Entity->MonsterKind);
    for(u32 AbilityIndex = 0;
        AbilityIndex < Def->AbilityCount;
        AbilityIndex++)
    {
        float Cooldown = Def->Abilities[AbilityIndex].Cooldown;
        Entity->AbilityCooldowns[AbilityIndex] =
            MonsterRandomBetween(AppState, 0.3f * Cooldown, Cooldown);
    }
}

inline void
RestartMonsterAnimation(world_entity *Entity)
{
    Entity->AnimationState.SlotIndex = 0;
    Entity->AnimationState.DeltaTime = 0.f;
}

inline void
SetMonsterPhase(world_entity *Entity, ability_phase Phase, float Seconds)
{
    Entity->AbilityPhase = Phase;
    Entity->AbilityTimer = Seconds;
    RestartMonsterAnimation(Entity);
}
