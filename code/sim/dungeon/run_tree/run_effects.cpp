/* Run effects (run_tree.cpp has the talents and RunEffectShare): what the
   second tree does on a hit, on a kill, when hurt and once a tick (what
   it does to a heal is in run_tree.cpp, as HealPlayer comes first). Included by role_abilities.cpp after the kits, as it heals
   through HealPlayer (role_kits/allies.cpp). The plain stats go through
   RoleStatShare instead, and the hit conditions through RunDealtScale. */

// NOTE(zoubir): Aura and Anthem from every ally put together stop here
#define RUN_AURA_MOST 0.15f

// NOTE(zoubir): from OnRoleHit: one more hit for Cadence
inline void
OnRunHit(player_slot *Attacker)
{
    Attacker->RunHits++;
}

// NOTE(zoubir): from DungeonScaleDamage: Attacker's hit is about to kill
// a monster: Feast heals, Shared Spoils heals the allies near, Reprisal
// takes time off the class spells, Frenzy starts
internal void
OnRunKill(app_state *AppState, player_slot *Attacker)
{
    world_entity *Self = Attacker->Entity;
    if (!Self || IsDeadPlayer(Self))
    {
        return;
    }
    float Feast = RunEffectShare(Attacker, RunEffect_Feast);
    if (Feast > 0.f)
    {
        HealPlayer(AppState, Self->PlayerIndex, Self, Feast * Self->MaxHp);
    }
    float Shared = RunEffectShare(Attacker, RunEffect_SharedFeast);
    for(u32 OtherIndex = 0; OtherIndex < MAX_PLAYERS && Shared > 0.f; OtherIndex++)
    {
        world_entity *Ally = AppState->Players[OtherIndex].Entity;
        if (OtherIndex != Self->PlayerIndex && AppState->Players[OtherIndex].Active && Ally &&
            Ally->IsPresent && !IsDeadPlayer(Ally) &&
            LengthSq(Ally->Position.XY - Self->Position.XY) <= RUN_AURA_REACH * RUN_AURA_REACH)
        {
            HealPlayer(AppState, Self->PlayerIndex, Ally, Shared * Ally->MaxHp);
        }
    }
    float Refund = RunEffectShare(Attacker, RunEffect_Refund);
    for(u32 Key = 0; Key < ROLE_KEYS && Refund > 0.f; Key++)
    {
        Attacker->RoleCooldowns[Key] = Maximum(0.f, Attacker->RoleCooldowns[Key] - Refund);
    }
    if (RunEffectShare(Attacker, RunEffect_Frenzy) != 0.f)
    {
        Attacker->RunFrenzySeconds = RUN_FRENZY_SECONDS;
    }
}

// NOTE(zoubir): from DungeonScaleDamage: Source took Damage off the
// player in Slot; Thorns owes it a share back, paid next tick
internal void
OnRunHurt(app_state *AppState, player_slot *Slot, world_entity *Source, float Damage)
{
    float Thorns = RunEffectShare(Slot, RunEffect_Thorns);
    if (Thorns <= 0.f || !Source || Source->Type != EntityType_Monster || !(Damage > 0.f))
    {
        return;
    }
    u32 MonsterSlot = (u32)(Source - AppState->World.Entities);
    if (MonsterSlot != Slot->RunThornsSlot || Source->MonsterSerial != Slot->RunThornsSerial)
    {
        Slot->RunThornsOwed = 0.f;
    }
    Slot->RunThornsSlot = MonsterSlot;
    Slot->RunThornsSerial = Source->MonsterSerial;
    Slot->RunThornsOwed += Thorns * Damage;
}

// NOTE(zoubir): once a tick, from UpdateDungeon: a new fight resets
// Cadence's count and Lifeline, Frenzy runs down, Thorns pays, Regen
// heals in a fight, Lifeline catches a player falling low, and each
// player gets its allies' Aura and Anthem
internal void
UpdateRunTrees(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        if (!Slot->Active)
        {
            continue;
        }
        if (Slot->RunFightSeen != Run->MeterFight)
        {
            Slot->RunFightSeen = Run->MeterFight;
            Slot->RunLifelineSpent = false;
            Slot->RunHits = 0;
        }
        Slot->RunFrenzySeconds = Maximum(0.f, Slot->RunFrenzySeconds - DeltaTime);
        Slot->RunAuraArmor = 0.f;
        Slot->RunAuraDamage = 0.f;
        world_entity *Self = Slot->Entity;
        if (!Self || !Self->IsPresent || IsDeadPlayer(Self))
        {
            Slot->RunThornsOwed = 0.f;
            continue;
        }
        if (Slot->RunThornsOwed > 0.f)
        {
            float Owed = Slot->RunThornsOwed;
            Slot->RunThornsOwed = 0.f;
            world_entity *Foe = FindMonsterBySerial(World, Slot->RunThornsSlot, Slot->RunThornsSerial);
            if (Foe && Foe->Hp > 0.f)
            {
                DamageEntity(AppState, World, Foe, Owed, Self);
            }
        }
        float Regen = RunEffectShare(Slot, RunEffect_Regen);
        if (Regen > 0.f && Run->FightingRoom)
        {
            HealPlayer(AppState, SlotIndex, Self, Regen * Self->MaxHp * DeltaTime);
        }
        float Lifeline = RunEffectShare(Slot, RunEffect_Lifeline);
        if (Lifeline > 0.f && Run->FightingRoom && !Slot->RunLifelineSpent && Self->MaxHp > 0.f &&
            Self->Hp < RUN_LIFELINE_BELOW * Self->MaxHp)
        {
            Slot->RunLifelineSpent = true;
            HealPlayer(AppState, SlotIndex, Self, Lifeline * Self->MaxHp);
        }
        for(u32 OtherIndex = 0; OtherIndex < MAX_PLAYERS; OtherIndex++)
        {
            player_slot *Other = &AppState->Players[OtherIndex];
            world_entity *Body = Other->Entity;
            if (OtherIndex == SlotIndex || !Other->Active || !Body || !Body->IsPresent ||
                IsDeadPlayer(Body) ||
                LengthSq(Body->Position.XY - Self->Position.XY) > RUN_AURA_REACH * RUN_AURA_REACH)
            {
                continue;
            }
            Slot->RunAuraArmor += RunEffectShare(Other, RunEffect_Aura);
            Slot->RunAuraDamage += RunEffectShare(Other, RunEffect_Anthem);
        }
        // NOTE(zoubir): a Berserker's Battle Shout, its own or one near,
        // outside the cap and never twice (role_kits/berserker.cpp)
        float Shout = 0.f;
        for(u32 OtherIndex = 0; OtherIndex < MAX_PLAYERS; OtherIndex++)
        {
            player_slot *Other = &AppState->Players[OtherIndex];
            world_entity *Body = Other->Entity;
            if (Other->Active && Other->Role == PlayerRole_Berserker &&
                Other->Berserker.ShoutSeconds > 0.f && Body && Body->IsPresent && !IsDeadPlayer(Body) &&
                LengthSq(Body->Position.XY - Self->Position.XY) <= BATTLE_SHOUT_REACH * BATTLE_SHOUT_REACH)
            {
                Shout = BATTLE_SHOUT_SHARE;
            }
        }
        Slot->RunAuraArmor = Minimum(RUN_AURA_MOST, Slot->RunAuraArmor);
        Slot->RunAuraDamage = Minimum(RUN_AURA_MOST, Slot->RunAuraDamage) + Shout;
    }
}
