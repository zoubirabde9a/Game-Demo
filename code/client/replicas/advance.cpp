/* Between snapshots, every frame: what runs on its own until the next
   snapshot says otherwise. Respawn countdowns, replicas gliding to their
   newest position (replica_smoothing.cpp), wind-up warnings filling in, an
   enrage burst playing out, the local player's cooldown bars running down,
   and animation frames. Included by replicas.cpp; SyncReplicas calls
   AdvanceReplicas every frame. */

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
                // NOTE(zoubir): cooldown bars run down between snapshots
                if (Replica->Type == EntityType_Player &&
                    Replica == AppState->Players[LocalSlot].Entity)
                {
                    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT; Index++)
                    {
                        float Full;
                        float *Seconds = PlayerCooldown(Replica, Index, &Full);
                        if (Seconds) *Seconds = Maximum(0.f, *Seconds - DeltaTime);
                    }
                }
            }
            if (Replica->IsPresent && Replica->AnimationSet)
            {
                AdvanceAnimation(&Replica->AnimationState,
                                 Replica->AnimationSet,
                                 Replica->AnimationType,
                                 Replica->AnimationDirection,
                                 DeltaTime, 1.f);
            }
        }
    }
}
