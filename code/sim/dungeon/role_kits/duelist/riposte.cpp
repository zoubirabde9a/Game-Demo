/* Duelist Riposte and Feint (role_kits/duelist.cpp, the R and C keys),
   the Guard branch's pair; a Duelist takes one of them.

   Feint is a quick step (Hasted for a moment) and a dodge: for
   FEINT_SECONDS the next blow on the Duelist misses whole and gives
   FEINT_TEMPO, with no counter. With Bait the dodge heals as a counter
   does.

   Riposte: the rapier held across
   for RIPOSTE_GUARD_SECONDS (BAIT_GUARD_SECONDS with Bait), a timer of the
   slot's that a ClassFlags bit shows every client.

   Every hit landing in the guard does nothing. The first one is a parry:
   COUNTER_TEMPO more Tempo, Riposte ready again in RIPOSTE_READY_SECONDS
   instead of its whole cooldown, and a counter owed, struck on the next
   tick (duelist/effects.cpp) on the attacker if it is within
   RIPOSTE_REACH, else the nearest foe in it: COUNTER_DAMAGE and a stun
   (with Bait it heals the Duelist too). Later hits in the same guard are
   cancelled without a second counter. A guard that parries nothing keeps
   the whole cooldown the cast spent.

   How a hit reaches it: every monster blow, shot and area hit on a player
   goes through ApplyHit (sim/hit.cpp), which asks DuelistParriesHit
   before it deals damage, shoves or stuns, so a parry cancels all of it,
   and a hit outside the guard is where Tempo is lost. That is the server
   only: a client predicting its own player runs no monsters and so no
   ApplyHit on itself, and the hook refuses a predicted slot anyway, so a
   parry is counted once per real hit. Damage that is not a blow (a burn
   or poison ticking, a boss clock's pulse) comes through DamageEntity
   alone: DuelistTakenScale cancels it in the guard but it neither parries
   nor takes Tempo. */

// NOTE(zoubir): Feint (C): a quick step and a dodge ready for the next
// blow (DuelistParriesHit)
internal void
StartFeint(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    if (DuelistUseKey(Slot, 2))
    {
        AddTempo(Slot, 1);
    }
    Slot->Duelist.FeintSeconds = FEINT_SECONDS;
    ApplyStatus(Player, StatusEffect_Hasted, FEINT_HASTE_SECONDS);
    v2 Aim = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Lunge),
              (u8)Player->PlayerIndex, DuelistBurstSpot(ChestOf(Player), 0), ATan2(Aim.Y, Aim.X));
    EmitSound(&AppState->Events, AssetType_SfxDash, Player->Position);
}

internal void
StartGuard(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    duelist_slot *Duel = &Slot->Duelist;
    bool32 Bait = RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Bait) > 0;
    if (DuelistUseKey(Slot, 1))
    {
        AddTempo(Slot, 1);
    }
    Duel->GuardSeconds = Bait ? BAIT_GUARD_SECONDS : RIPOSTE_GUARD_SECONDS;
    Duel->Parried = false;
    v2 Aim = NormalizeOr(GetPlayerAim(Player), V2(1.f, 0.f));
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Guard),
              (u8)Player->PlayerIndex, DuelistBurstSpot(ChestOf(Player), Bait ? 1 : 0),
              ATan2(Aim.Y, Aim.X));
    EmitSound(&AppState->Events, AssetType_SfxShield, Player->Position);
}

// NOTE(zoubir): from ApplyHit (sim/hit.cpp), before a hit of Damage by
// Source lands on Target: true when a Duelist's guard parried it, which
// cancels all of it. A Duelist off guard loses Tempo to it instead
internal bool32
DuelistParriesHit(app_state *AppState, world_entity *Target, world_entity *Source, float Damage)
{
    if (!IsDungeon(AppState) || Target->Type != EntityType_Player ||
        Target->PlayerIndex >= MAX_PLAYERS || Damage <= 0.f)
    {
        return false;
    }
    player_slot *Slot = &AppState->Players[Target->PlayerIndex];
    if (Slot->Role != PlayerRole_Duelist || Slot->Predicted)
    {
        return false;
    }
    duelist_slot *Duel = &Slot->Duelist;
    // NOTE(zoubir): a Feint's dodge takes the blow first, and is spent
    if (Duel->FeintSeconds > 0.f && Duel->GuardSeconds <= 0.f)
    {
        Duel->FeintSeconds = 0.f;
        AddTempo(Slot, FEINT_TEMPO);
        if (RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Bait))
        {
            HealPlayer(AppState, Target->PlayerIndex, Target, BAIT_HEAL_SHARE * Target->MaxHp);
        }
        v2 Aim = NormalizeOr(GetPlayerAim(Target), V2(1.f, 0.f));
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Parry),
                  (u8)Target->PlayerIndex, ChestOf(Target), ATan2(Aim.Y, Aim.X));
        return true;
    }
    if (Duel->GuardSeconds <= 0.f)
    {
        LoseTempo(AppState, Slot, Target);
        return false;
    }
    if (Duel->Parried)
    {
        return true;
    }
    Duel->Parried = true;
    Duel->CounterDue = true;
    Duel->CounterSlot = Duel->CounterSerial = 0;
    if (Source && Source->Type == EntityType_Monster)
    {
        Duel->CounterSlot = (u32)(Source - AppState->World.Entities);
        Duel->CounterSerial = Source->MonsterSerial;
    }
    AddTempo(Slot, COUNTER_TEMPO);
    Slot->RoleCooldowns[1] = Minimum(Slot->RoleCooldowns[1], RIPOSTE_READY_SECONDS);
    v2 Toward = Source ? NormalizeOr(Source->Position.XY - Target->Position.XY, GetPlayerAim(Target)) :
        NormalizeOr(GetPlayerAim(Target), V2(1.f, 0.f));
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Parry),
              (u8)Target->PlayerIndex, ChestOf(Target), ATan2(Toward.Y, Toward.X));
    EmitSound(&AppState->Events, AssetType_SfxShieldSlam, Target->Position);
    return true;
}

// NOTE(zoubir): the counter a parry owed, from UpdateDuelistEffects
internal void
StrikeCounter(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    duelist_slot *Duel = &Slot->Duelist;
    Duel->CounterDue = false;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    world_entity *Foe = Duel->CounterSerial ?
        FindMonsterBySerial(World, Duel->CounterSlot, Duel->CounterSerial) : 0;
    if (Foe && (!IsDuelistFoe(World, Foe, Room) ||
                Length(Foe->Position.XY - Player->Position.XY) >
                RIPOSTE_REACH + 0.5f * Foe->Dimensions.X))
    {
        Foe = 0;
    }
    if (!Foe)
    {
        Foe = NearestFoe(World, Player->Position.XY, RIPOSTE_REACH, Room, 0, 0);
    }
    if (!Foe)
    {
        return;
    }
    v2 Away = DuelistAway(Player, Foe);
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_DuelistFirst, DuelistBurst_Counter),
              (u8)Player->PlayerIndex, ChestOf(Foe), ATan2(Away.Y, Away.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    DuelistStrike(AppState, Slot, Player, Foe, COUNTER_DAMAGE, COUNTER_SHOVE, COUNTER_STUN);
    if (RoleRank(Slot, PlayerRole_Duelist, DuelistTalent_Bait))
    {
        HealPlayer(AppState, Player->PlayerIndex, Player, BAIT_HEAL_SHARE * Player->MaxHp);
    }
}
