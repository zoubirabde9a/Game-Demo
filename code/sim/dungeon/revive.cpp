/* Revives (encounters.cpp): during a fight a dead player lies downed
   where they fell (the fight keeps their respawn waiting). A living
   healer standing within REVIVE_RADIUS of them for REVIVE_SECONDS in a
   row brings them back there with REVIVE_HP_SHARE of their health. A
   healer who steps away loses the progress. A revived player is a
   moment out of reach (REVIVE_SHIELD_SECONDS), so a monster standing
   over the body cannot kill them again the same tick. A healer with the
   Miracle talent (role_talents.cpp) revives faster and with more health. */

#define REVIVE_RADIUS 60.f
#define REVIVE_SECONDS 3.f
#define REVIVE_HP_SHARE 0.4f
#define REVIVE_SHIELD_SECONDS 1.f

// NOTE(zoubir): a living healer within REVIVE_RADIUS of Body, the one
// with Miracle first; 0 for none
internal player_slot *
HealerNear(app_state *AppState, world_entity *Body)
{
    player_slot *Result = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        world_entity *Player = LivingPlayerInSlot(AppState, SlotIndex);
        if (Player && Slot->Role == PlayerRole_Healer &&
            Length(Player->Position.XY - Body->Position.XY) <= REVIVE_RADIUS &&
            (!Result || RoleRank(Slot, PlayerRole_Healer, HealerTalent_Miracle)))
        {
            Result = Slot;
        }
    }
    return Result;
}

// NOTE(zoubir): the seconds Healer takes to revive someone
inline float
ReviveSecondsFor(player_slot *Healer)
{
    float Result = RoleRank(Healer, PlayerRole_Healer, HealerTalent_Miracle) ?
        MIRACLE_SECONDS : REVIVE_SECONDS;
    return Result;
}

// NOTE(zoubir): the downed player stands up where they lie
internal void
RevivePlayer(app_state *AppState, player_slot *Slot, player_slot *Healer)
{
    world_entity *Player = Slot->Entity;
    Player->Hp = (RoleRank(Healer, PlayerRole_Healer, HealerTalent_Miracle) ?
                  MIRACLE_HP_SHARE : REVIVE_HP_SHARE) * Player->MaxHp;
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
        player_slot *Healer = HealerNear(AppState, Slot->Entity);
        if (!Healer)
        {
            Slot->ReviveSeconds = 0.f;
            continue;
        }
        // NOTE(zoubir): counted in shares of REVIVE_SECONDS, so the ring
        // clients draw from it fills whoever revives
        Slot->ReviveSeconds += DeltaTime * REVIVE_SECONDS / ReviveSecondsFor(Healer);
        if (Slot->ReviveSeconds >= REVIVE_SECONDS)
        {
            RevivePlayer(AppState, Slot, Healer);
        }
    }
}
