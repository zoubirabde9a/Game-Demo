/* Revives (encounters.cpp): during a fight a dead player lies downed
   where they fell (the fight keeps their respawn waiting). A living
   healer standing within REVIVE_RADIUS of them for REVIVE_SECONDS in a
   row brings them back there with REVIVE_HP_SHARE of their health. A
   healer who steps away loses the progress. A revived player is a
   moment out of reach (REVIVE_SHIELD_SECONDS), so a monster standing
   over the body cannot kill them again the same tick. */

#define REVIVE_RADIUS 60.f
#define REVIVE_SECONDS 3.f
#define REVIVE_HP_SHARE 0.4f
#define REVIVE_SHIELD_SECONDS 1.f

// NOTE(zoubir): a living healer within REVIVE_RADIUS of Body
internal bool32
IsHealerNear(app_state *AppState, world_entity *Body)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && AppState->Players[SlotIndex].Role == PlayerRole_Healer &&
            Length(Player->Position.XY - Body->Position.XY) <= REVIVE_RADIUS)
        {
            return true;
        }
    }
    return false;
}

// NOTE(zoubir): the downed player stands up where they lie
internal void
RevivePlayer(app_state *AppState, player_slot *Slot)
{
    world_entity *Player = Slot->Entity;
    Player->Hp = REVIVE_HP_SHARE * Player->MaxHp;
    Player->Velocity = {};
    ZeroArray(Player->StatusTimers, StatusEffect_Count, float);
    Player->SpawnShield = REVIVE_SHIELD_SECONDS;
    Slot->RespawnTimer = 0.f;
    Slot->DelayedInputCount = 0;
    Slot->ReviveSeconds = 0.f;
    EmitBurst(&AppState->Events, SimBurst_Spawn, (u8)Player->PlayerIndex,
              Player->Position);
}

// NOTE(zoubir): once a tick while a fight lasts
internal void
UpdateRevives(app_state *AppState, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active || !Slot->Entity || !IsDeadPlayer(Slot->Entity))
        {
            Slot->ReviveSeconds = 0.f;
            continue;
        }
        if (!IsHealerNear(AppState, Slot->Entity))
        {
            Slot->ReviveSeconds = 0.f;
            continue;
        }
        Slot->ReviveSeconds += DeltaTime;
        if (Slot->ReviveSeconds >= REVIVE_SECONDS)
        {
            RevivePlayer(AppState, Slot);
        }
    }
}
