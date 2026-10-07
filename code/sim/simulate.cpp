/* SimulateTick: one step of the game rules for every entity. It reads
   each player slot's input, moves and fights, advances animations, and
   refills monsters. It never draws, plays sound or touches assets: sounds
   go to AppState->Events. The client calls it each frame before drawing;
   the server calls it on its fixed tick. */

// NOTE(zoubir): the round after a break, on the same map or the one a
// vote picked (setup.cpp, included later)
internal void StartNextRoundMap(app_state *AppState, memory_arena *Arena);

internal void
SimulateTick(app_state *AppState, memory_arena *Arena, float DeltaTime)
{
    // NOTE(zoubir): before anything holds a pointer into the old world
    if (AppState->RoundMapDue)
    {
        AppState->RoundMapDue = false;
        StartNextRoundMap(AppState, Arena);
    }
    world *World = &AppState->World;
    if (!AppState->Rewind)
    {
        AppState->Rewind = CreateTimeRewind(Arena);
    }
    time_rewind *Rewind = AppState->Rewind;
    // NOTE(zoubir): a world rewind holding or playing back stops
    // everything else (sim/time_rewind/). A frame it puts back may have
    // been taken while another rewind ran a body through someone, so the
    // units are still pulled apart after it
    if (Rewind->WorldFrozen)
    {
        UpdateRewinds(AppState, Rewind, World, Arena, DeltaTime);
        SeparateOverlappingUnits(AppState, World, Arena);
        return;
    }
    AdvanceRewindClock(Rewind, DeltaTime);
    UpdateTerrainEffects(World);
    // NOTE(zoubir): experience, and the talents asked for this tick, take
    // effect before anyone moves (sim/progression/)
    UpdateProgression(AppState, DeltaTime);
    // NOTE(zoubir): the break after a death, when nobody fights
    // (sim/round_break.cpp)
    UpdateRoundBreak(AppState, DeltaTime);
    // NOTE(zoubir): the duel's final blow plays in slow motion; the
    // break above counts real seconds, everything below the slowed ones
    DeltaTime *= RoundTimeScale(AppState);
    // NOTE(zoubir): a map vote passing moves everyone on the next tick
    // (sim/map_vote.cpp)
    UpdateMapVote(AppState, DeltaTime);
    // NOTE(zoubir): a dungeon run's rooms, gates and wipes (sim/dungeon/)
    UpdateDungeon(AppState, Arena, DeltaTime);

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
        // NOTE(zoubir): a monster in a hit-pause (sim/hit.cpp) skips its
        // update and animation this tick
        if (TickHitStop(Entity, DeltaTime))
        {
            continue;
        }
        // NOTE(zoubir): frozen by a rewind, which moves it itself
        if (IsTimeLocked(AppState, Entity))
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
                bool32 Waiting = Slot->WaitingForInput;
                Slot->WaitingForInput = false;
                if (UpdateDeadPlayer(Slot, World, Arena, AppState, DeltaTime))
                {
                    Animates = false;
                    break;
                }
                // NOTE(zoubir): its client's input for this tick is late
                // (server/input_queue.cpp): it stands this tick out, as
                // the client, which never stepped without an input, has it
                if (Waiting)
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

            case EntityType_Kunai:
            {
                UpdateKunai(Entity, World, Arena, DeltaTime, AppState);
            } break;

            case EntityType_MonsterShot:
            {
                UpdateMonsterShot(Entity, World, Arena, DeltaTime, AppState);
                AnimationType = AnimationType_Move;
                AnimationDirection = Entity->AnimationDirection;
            } break;

            case EntityType_MonsterHazard:
            {
                UpdateMonsterHazard(Entity, World, AppState, DeltaTime);
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
                             DeltaTime,
                             AnimationSpeed * MoveCycleRate(Entity, AnimationType));
        }
    }

    UpdateStatusEffects(AppState, World, DeltaTime);

    if (AppState->Monsters)
    {
        UpdateMonsterPopulation(AppState, World, Arena, AppState->Monsters,
                                DeltaTime);
    }

    // NOTE(zoubir): the rewinds move what they froze, and put it back at
    // the end of a playback, where something else may stand by now
    UpdateRewinds(AppState, Rewind, World, Arena, DeltaTime);

    // NOTE(zoubir): last, so nothing that moved or spawned this tick is
    // left inside a wall or another unit (what a rewind still holds is
    // outside time and blocks nothing, collision_rules.cpp)
    SeparateOverlappingUnits(AppState, World, Arena);

    // NOTE(zoubir): then the tick goes into the history
    RecordRewindHistory(AppState, Rewind, World, DeltaTime);
}
