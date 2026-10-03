/* SimulateTick: one step of the game rules for every entity. It reads
   each player slot's input, moves and fights, advances animations, and
   refills monsters. It never draws, plays sound or touches assets: sounds
   go to AppState->Events. The client calls it each frame before drawing;
   the server calls it on its fixed tick. */

internal void
SimulateTick(app_state *AppState, memory_arena *Arena, float DeltaTime)
{
    world *World = &AppState->World;

    // NOTE(zoubir): entities added during the tick (fireballs, swords,
    // respawned monsters) wait for the next one
    u32 EntityCount = World->EntityCount;
    for(u32 EntityIndex = 0;
        EntityIndex < EntityCount;
        EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (!Entity->IsPresent)
        {
            continue;
        }

        float AnimationSpeed = 1.f;
        animation_type AnimationType = AnimationType_Stand;
        animation_direction AnimationDirection =
            Entity->AnimationState.LastAnimationDirection;
        bool32 Animates = true;

        switch(Entity->Type)
        {
            case EntityType_Player:
            {
                player_slot *Slot = GetPlayerSlot(AppState, Entity);
                if (UpdateDeadPlayer(Slot, World, Arena, AppState, DeltaTime))
                {
                    Animates = false;
                    break;
                }
                UpdatePlayer(Slot, World, Arena, DeltaTime, AppState,
                             &AnimationSpeed, &AnimationType,
                             &AnimationDirection);
            } break;

            case EntityType_Sword:
            {
                Animates = UpdateSword(Entity, World, Arena, AppState,
                                       DeltaTime);
                AnimationDirection = Entity->AnimationDirection;
            } break;

            case EntityType_Familiar:
            {
                UpdateFamiliar(Entity, World, Arena, DeltaTime, AppState,
                               &AnimationSpeed, &AnimationType,
                               &AnimationDirection);
            } break;

            case EntityType_FireBall:
            {
                AnimationSpeed = Entity->AnimationSpeed;
                AnimationType = Entity->AnimationType;
                AnimationDirection = Entity->AnimationDirection;
                UpdateFireBall(Entity, World, Arena, DeltaTime, AppState);
            } break;

            case EntityType_Monster:
            {
                UpdateMonster(Entity, World, Arena, DeltaTime, AppState,
                              &AnimationSpeed, &AnimationType,
                              &AnimationDirection);
            } break;

            case EntityType_StaticObject:
            case EntityType_Tiled:
            case EntityType_Count:
            case EntityType_Invalid:
            {
                Animates = false;
            } break;
        }

        if (Animates && Entity->IsPresent && Entity->AnimationSet)
        {
            AdvanceAnimation(&Entity->AnimationState, Entity->AnimationSet,
                             AnimationType, AnimationDirection,
                             DeltaTime, AnimationSpeed);
        }
    }

    if (AppState->Monsters)
    {
        UpdateMonsterPopulation(AppState, World, Arena, AppState->Monsters,
                                DeltaTime);
    }
}
