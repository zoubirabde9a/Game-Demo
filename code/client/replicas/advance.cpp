/* Between snapshots, every frame: what runs on its own until the next
   snapshot says otherwise. Respawn countdowns, replicas gliding to their
   newest position (replica_smoothing.cpp), wind-up warnings filling in, an
   enrage burst playing out, the local player's cooldown bars running down,
   a hit-pause running out, and animation frames. Included by
   replicas.cpp; SyncReplicas calls AdvanceReplicas every frame. */

internal void
AdvanceReplicas(app_state *AppState, memory_arena *Arena, replica_table *Table,
                float DeltaTime, u32 LocalSlot)
{
    world *World = &AppState->World;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && IsDeadPlayer(Slot->Entity))
        {
            Slot->RespawnTimer = Maximum(0.f, Slot->RespawnTimer - DeltaTime);
        }
    }
    AdvanceSmoothing(&Table->Smoothing, DeltaTime);
    for(u32 Id = 0; Id < MAX_REPLICAS; Id++)
    {
        if (Table->LocalIndexPlusOne[Id])
        {
            world_entity *Replica =
                &World->Entities[Table->LocalIndexPlusOne[Id] - 1];
            if (Replica->IsPresent)
            {
                SmoothReplica(AppState, Arena, &Table->Smoothing, Id, Replica);
                // NOTE(zoubir): a warning fills in smoothly between snapshots
                if (Replica->Type == EntityType_Monster &&
                    Replica->AbilityPhase != AbilityPhase_Ready)
                {
                    Replica->AbilityTimer = Maximum(0.f, Replica->AbilityTimer - DeltaTime);
                }
                if (Replica->Type == EntityType_Monster && Replica->PhaseFlash > 0.f)
                {
                    Replica->PhaseFlash = Maximum(0.f, Replica->PhaseFlash - DeltaTime);
                }
                // NOTE(zoubir): the local player's cooldown bars run down
                // between snapshots in prediction (UpdatePlayer counts them
                // as it steps); counting here as well ran them twice as fast
            }
            // NOTE(zoubir): a monster in a hit-pause holds its frame, as
            // on the server (sim/hit.cpp)
            bool32 Frozen = Replica->HitStop > 0.f;
            Replica->HitStop = Maximum(0.f, Replica->HitStop - DeltaTime);
            Replica->HitFresh = Maximum(0.f, Replica->HitFresh - DeltaTime);
            if (Replica->IsPresent && Replica->AnimationSet && !Frozen)
            {
                AdvanceAnimation(&Replica->AnimationState,
                                 Replica->AnimationSet,
                                 Replica->AnimationType,
                                 Replica->AnimationDirection,
                                 DeltaTime,
                                 MoveCycleRate(Replica, Replica->AnimationType));
            }
        }
    }
}
