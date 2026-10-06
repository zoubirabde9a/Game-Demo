/* Replica spawning: the local entity that mirrors one server entity,
   made with the same Add* functions the simulation uses, by entity type.
   Monster shots and hazards have no simulation of their own here, so
   their replicas are plain entities drawn from their ability. Included
   by replicas.cpp, which decides when a replica is made or remade. */

// NOTE(zoubir): a monster shot as AddMonsterShot dresses it, without the
// owner and ability the server used to fire it
internal world_entity *
AddShotReplica(app_state *AppState, world *World, memory_arena *Arena,
               v3 Position, u32 ShotStyle)
{
    world_entity *Shot = AddEntity(AppState, World, Arena,
                                   EntityType_MonsterShot, Position,
                                   AppState->FireBallCollision);
    Shot->Dimensions = V2((float)SHOT_FRAME_SIZE, (float)SHOT_FRAME_SIZE);
    Shot->Texture = {AssetType_MonsterShot, ShotStyle};
    Shot->ShadowTexture = {AssetType_Shadow};
    if (AppState->Monsters && ShotStyle < ArrayCount(AppState->Monsters->ShotAnimationSets))
    {
        Shot->AnimationSet = &AppState->Monsters->ShotAnimationSets[ShotStyle];
    }
    return Shot;
}

// NOTE(zoubir): a hazard as AddMonsterHazard dresses it: its look and
// size come from that monster kind's ability. 0 if the server named an
// ability this build does not have.
internal world_entity *
AddHazardReplica(app_state *AppState, world *World, memory_arena *Arena,
                 v3 Position, u32 Kind, u32 AbilityIndex)
{
    if (Kind >= MonsterKind_Count)
    {
        return 0;
    }
    monster_def *Def = GetMonsterDef((monster_kind)Kind);
    if (AbilityIndex >= Def->AbilityCount)
    {
        return 0;
    }
    monster_ability *Ability = &Def->Abilities[AbilityIndex];
    world_entity *Hazard = AddEntity(AppState, World, Arena,
                                     EntityType_MonsterHazard, Position,
                                     AppState->FireBallCollision);
    Hazard->MonsterKind = (monster_kind)Kind;
    Hazard->AbilityIndex = AbilityIndex;
    float Size = 2.f * Ability->Radius;
    Hazard->Dimensions = V2(Size, Size);
    Hazard->Texture = {AssetType_MonsterHazard, (u32)Ability->HazardStyle};
    if (AppState->Monsters)
    {
        Hazard->AnimationSet =
            &AppState->Monsters->HazardAnimationSets[Ability->HazardStyle];
    }
    return Hazard;
}

// NOTE(zoubir): 0 for types the client has no look for
internal world_entity *
SpawnReplica(app_state *AppState, world *World, memory_arena *Arena,
             net_entity_state *State)
{
    v3 Position = V3(State->X, State->Y, State->Z);
    world_entity *Result = 0;
    switch ((entity_type)State->Type)
    {
        case EntityType_Player:
        {
            Result = AddPlayer(AppState, World, Arena, Position);
        } break;
        case EntityType_Monster:
        {
            if (State->Variant < MonsterKind_Count)
            {
                Result = AddMonster(AppState, World, Arena, Position,
                                    (monster_kind)State->Variant);
            }
        } break;
        case EntityType_FireBall:
        {
            Result = AddFireBall(AppState, World, Arena, 0, Position,
                                 V3(State->VelX, State->VelY, 0.f));
        } break;
        case EntityType_Sword:
        {
            Result = AddSword(AppState, World, Arena, Position, 0,
                              (animation_direction)State->Facing);
        } break;
        case EntityType_MonsterShot:
        {
            Result = AddShotReplica(AppState, World, Arena, Position,
                                    State->Variant);
        } break;
        case EntityType_Kunai:
        {
            Result = AddKunai(AppState, World, Arena, Position,
                              V3(State->VelX, State->VelY, 0.f));
        } break;
        case EntityType_MonsterHazard:
        {
            Result = AddHazardReplica(AppState, World, Arena, Position,
                                      State->Variant, State->Ability);
        } break;
        default: break;
    }
    return Result;
}
