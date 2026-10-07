/* Between snapshots, every frame: what runs on its own until the next
   snapshot says otherwise. Respawn countdowns and the round break's, replicas moving between
   their snapshots (replica_smoothing.cpp), wind-up warnings filling in, an
   enrage burst playing out, players' cast bars filling in, the local
   player's cooldown bars running down, a hit-pause running out, and
   animation frames. Included by replicas.cpp; SyncReplicas calls
   AdvanceReplicas every frame. */

internal void
AdvanceReplicas(app_state *AppState, memory_arena *Arena, replica_table *Table,
                float DeltaTime, u32 LocalSlot)
{
    world *World = &AppState->World;
    AppState->RoundBreak = Maximum(0.f, AppState->RoundBreak - DeltaTime);
    // NOTE(zoubir): the server runs the world slower while the duel's
    // final blow plays (sim/round_break.cpp), so the timers and animation
    // frames below do too. The render clock follows the server's ticks,
    // which keep their real pace
    float RealTime = DeltaTime;
    DeltaTime *= RoundTimeScale(AppState);
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (Slot->Active && Slot->Entity && IsDeadPlayer(Slot->Entity))
        {
            Slot->RespawnTimer = Maximum(0.f, Slot->RespawnTimer - DeltaTime);
        }
    }
    AdvanceSmoothing(&Table->Smoothing, RealTime, (float)NET_TICK_RATE);
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
                // NOTE(zoubir): so does a player's cast bar; the local
                // player's cast is stepped by prediction instead
                if (Replica->Type == EntityType_Player && IsPlayerCasting(Replica) &&
                    Replica->PlayerIndex != LocalSlot)
                {
                    Replica->CastLeft = Maximum(0.f, Replica->CastLeft - DeltaTime);
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
