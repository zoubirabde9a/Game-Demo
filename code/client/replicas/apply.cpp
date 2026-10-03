/* Applying a snapshot: one entity's state (position, velocity, health,
   facing, animation, elite affix, status pips, ability) onto its replica,
   front-armoured monsters' facings, and every player's score. Included
   by replicas.cpp, whose SyncReplicas calls these on each new snapshot. */

internal void
ApplyStateToReplica(app_state *AppState, memory_arena *Arena,
                    world_entity *Replica, net_entity_state *State)
{
    v3 OldPosition = Replica->Position;
    Replica->Position = V3(State->X, State->Y, State->Z);
    Replica->Velocity = V3(State->VelX, State->VelY, 0.f);
    Replica->Hp = (float)State->Health;
    if (State->Facing < AnimationDirection_Count)
    {
        Replica->AnimationDirection = (animation_direction)State->Facing;
    }
    if (State->Animation < AnimationType_Count)
    {
        Replica->AnimationType = (animation_type)State->Animation;
    }
    Replica->EliteAffix = State->Affix;
    Replica->AbilityIndex = State->Ability;
    // NOTE(zoubir): status pips blink under 1 s left; the client does not
    // know the real time left, so active effects read as 1.5 s
    for(u32 Effect = 1; Effect < StatusEffect_Count; Effect++)
    {
        Replica->StatusTimers[Effect] =
            (State->Status & (1 << (Effect - 1))) ? 1.5f : 0.f;
    }
    // NOTE(zoubir): keeps the chunk lists right so RemoveEntity finds it
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena,
                              OldPosition, Replica);
}

// NOTE(zoubir): front-armoured monsters' facing, a whole turn in 256
// steps, back into the unit Direction their shell is drawn from
internal void
ApplySnapshotFacings(world *World, replica_table *Table, net_snapshot *Snapshot)
{
    for(u32 Index = 0; Index < Snapshot->FacingCount; Index++)
    {
        net_facing *Facing = &Snapshot->Facings[Index];
        u16 Id = Snapshot->Entities[Facing->EntityIndex].Id;
        if (Id < MAX_REPLICAS && Table->LocalIndexPlusOne[Id])
        {
            world_entity *Replica =
                &World->Entities[Table->LocalIndexPlusOne[Id] - 1];
            float Angle = (float)Facing->Angle * (2.f * Pi32 / 256.f);
            Replica->Direction = V2(Cos(Angle), Sin(Angle));
        }
    }
}

// NOTE(zoubir): player slots mirror the server's: a slot is active while
// the server lists its score, and points at that player's replica. The
// local slot stays active so the camera always has someone to follow.
internal void
ApplySnapshotScores(app_state *AppState, net_snapshot *Snapshot)
{
    bool32 Listed[MAX_PLAYERS] = {};
    for(u32 Index = 0; Index < Snapshot->ScoreCount; Index++)
    {
        net_score *Score = &Snapshot->Scores[Index];
        if (Score->Slot < MAX_PLAYERS)
        {
            player_slot *Slot = &AppState->Players[Score->Slot];
            Listed[Score->Slot] = true;
            Slot->Active = true;
            Slot->Kills = Score->Kills;
            Slot->Deaths = Score->Deaths;
            Slot->MonsterKills = Score->MonsterKills;
        }
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (!Listed[SlotIndex] && SlotIndex != AppState->LocalPlayerIndex)
        {
            AppState->Players[SlotIndex].Active = false;
            AppState->Players[SlotIndex].Entity = 0;
        }
    }
}
