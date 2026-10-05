/* World hash: a fingerprint of the state the simulation decides, so two
   runs can be compared tick by tick. Replays record it every tick and
   check it when they play back (server/replay.cpp, tools/replay_main.cpp),
   and the determinism tests compare two runs of the same inputs.

   It reads values, never pointers, so two processes (or a Windows
   client and a Linux server built with the same float rules) running
   the same inputs get the same number. A field that changes what
   happens next belongs here; one only drawn does not. */

inline u32
HashWorldBytes(u32 Hash, void *Data, u32 Size)
{
    u8 *Bytes = (u8 *)Data;
    for(u32 Index = 0; Index < Size; Index++)
    {
        Hash = (Hash ^ Bytes[Index]) * 16777619u;
    }
    return Hash;
}

#define HashWorldValue(Hash, Value) HashWorldBytes(Hash, &(Value), sizeof(Value))

internal u32
HashWorldEntity(u32 Hash, world_entity *Entity)
{
    Hash = HashWorldValue(Hash, Entity->ID);
    Hash = HashWorldValue(Hash, Entity->Type);
    Hash = HashWorldValue(Hash, Entity->State);
    Hash = HashWorldValue(Hash, Entity->Flags);
    Hash = HashWorldValue(Hash, Entity->Position);
    Hash = HashWorldValue(Hash, Entity->GroundZ);
    Hash = HashWorldValue(Hash, Entity->Velocity);
    Hash = HashWorldValue(Hash, Entity->Direction);
    Hash = HashWorldValue(Hash, Entity->CastingDirection);
    Hash = HashWorldValue(Hash, Entity->AnimationState.DeltaTime);
    Hash = HashWorldValue(Hash, Entity->AnimationState.SlotIndex);
    Hash = HashWorldValue(Hash, Entity->AnimationState.CurrentType);
    Hash = HashWorldValue(Hash, Entity->DistanceRemaining);
    Hash = HashWorldValue(Hash, Entity->Hp);
    Hash = HashWorldValue(Hash, Entity->MaxHp);
    Hash = HashWorldValue(Hash, Entity->StatusTimers);
    Hash = HashWorldValue(Hash, Entity->StatusTickTimer);
    Hash = HashWorldValue(Hash, Entity->HitStop);
    Hash = HashWorldValue(Hash, Entity->ThrownBySlot);
    Hash = HashWorldValue(Hash, Entity->MonsterKind);
    Hash = HashWorldValue(Hash, Entity->AttackCooldown);
    Hash = HashWorldValue(Hash, Entity->WanderDirection);
    Hash = HashWorldValue(Hash, Entity->WanderTimer);
    Hash = HashWorldValue(Hash, Entity->AbilityPhase);
    Hash = HashWorldValue(Hash, Entity->AbilityIndex);
    Hash = HashWorldValue(Hash, Entity->AbilityTimer);
    Hash = HashWorldValue(Hash, Entity->AbilityCooldowns);
    Hash = HashWorldValue(Hash, Entity->EliteAffix);
    Hash = HashWorldValue(Hash, Entity->MonsterSerial);
    Hash = HashWorldValue(Hash, Entity->MovementCooldowns);
    Hash = HashWorldValue(Hash, Entity->Aim);
    Hash = HashWorldValue(Hash, Entity->ActionLock);
    Hash = HashWorldValue(Hash, Entity->Stagger);
    Hash = HashWorldValue(Hash, Entity->ActionCooldowns);
    Hash = HashWorldValue(Hash, Entity->JumpsUsed);
    Hash = HashWorldValue(Hash, Entity->AreaCooldowns);
    Hash = HashWorldValue(Hash, Entity->CastSpell);
    Hash = HashWorldValue(Hash, Entity->CastLeft);
    Hash = HashWorldValue(Hash, Entity->SpawnShield);
    Hash = HashWorldValue(Hash, Entity->ComboStep);
    Hash = HashWorldValue(Hash, Entity->ComboTimer);
    Hash = HashWorldValue(Hash, Entity->ComboTrail);
    Hash = HashWorldValue(Hash, Entity->RewindSerial);
    Hash = HashWorldValue(Hash, Entity->RewindCooldowns);
    Hash = HashWorldValue(Hash, Entity->PlayerIndex);
    Hash = HashWorldValue(Hash, Entity->OwnerSlot);
    Hash = HashWorldValue(Hash, Entity->TimeLeft);
    return Hash;
}

// NOTE(zoubir): every present entity, the player slots, the monster
// population's clock and random series, and the rewinds under way
internal u32
HashWorldState(app_state *AppState)
{
    u32 Hash = 2166136261u;
    world *World = &AppState->World;
    Hash = HashWorldValue(Hash, World->EntityCount);
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent)
        {
            Hash = HashWorldEntity(Hash, Entity);
        }
    }
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Hash = HashWorldValue(Hash, Slot->Active);
        Hash = HashWorldValue(Hash, Slot->Kills);
        Hash = HashWorldValue(Hash, Slot->Deaths);
        Hash = HashWorldValue(Hash, Slot->MonsterKills);
        Hash = HashWorldValue(Hash, Slot->RespawnTimer);
        Hash = HashWorldValue(Hash, Slot->DelayedInputCount);
        Hash = HashWorldValue(Hash, Slot->Xp);
        Hash = HashWorldValue(Hash, Slot->WardReady);
        for(u32 Talent = 0; Talent < TALENT_SLOTS; Talent++)
        {
            Hash = HashWorldValue(Hash, Slot->Ranks[Talent]);
        }
    }
    if (AppState->Monsters)
    {
        Hash = HashWorldValue(Hash, AppState->Monsters->RespawnTimer);
        Hash = HashWorldValue(Hash, AppState->Monsters->Series);
        Hash = HashWorldValue(Hash, AppState->Monsters->PendingDeathCount);
        Hash = HashWorldValue(Hash, AppState->Monsters->NextMonsterSerial);
    }
    time_rewind *Rewind = AppState->Rewind;
    if (Rewind)
    {
        Hash = HashWorldValue(Hash, Rewind->Clock);
        Hash = HashWorldValue(Hash, Rewind->FrameCount);
        Hash = HashWorldValue(Hash, Rewind->NextSerial);
        for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
        {
            rewind_cast *Cast = &Rewind->Casts[SlotIndex];
            Hash = HashWorldValue(Hash, Cast->Phase);
            Hash = HashWorldValue(Hash, Cast->PhaseLeft);
            Hash = HashWorldValue(Hash, Cast->AffectedCount);
        }
    }
    return Hash;
}
