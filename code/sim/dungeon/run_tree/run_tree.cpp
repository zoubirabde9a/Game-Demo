/* Run effects of the class tree (class_tree.cpp): the run talents a
   slot can hold (run_mods.cpp), the hash that rolls the wild slots, and
   what the run talents do to a hit, a heal and the threat made. TreeSeed
   rolls again as each new run starts (RerollRunTree, from
   StartNextRoundMap). */

#include "run_mods.cpp"

// NOTE(zoubir): TreeSeed's bits, all a snapshot carries (net/protocol.h)
#define RUN_SEED_MASK 0xffffu

inline u32
RunHash(u32 Value)
{
    Value ^= Value >> 16;
    Value *= 0x7feb352du;
    Value ^= Value >> 15;
    Value *= 0x846ca68bu;
    Value ^= Value >> 16;
    return Value;
}

#include "../class_tree.cpp"

// NOTE(zoubir): from StartNextRoundMap as a new run starts, and when a
// player joins: the wild slots roll again. Salt is anything that differs
// run to run (the map, the experience)
internal void
RerollRunTree(player_slot *Slot, u32 SlotIndex, u32 Salt)
{
    Slot->TreeSeed = RunHash(Slot->TreeSeed ^ RunHash(Salt + SlotIndex * 7919u + 1u)) & RUN_SEED_MASK;
}

// NOTE(zoubir): a share that multiplies a hit, kept above nothing
inline float
RunScale(float Share)
{
    float Result = Maximum(0.1f, 1.f + Share);
    return Result;
}

// NOTE(zoubir): from DungeonScaleDamage: the share of Attacker's hit on
// Target its second tree's conditions give (Execute, Opener, Bossbane or
// Packbane, Desperate, Cadence, Frenzy, Vanguard, Glory) and its allies'
// Anthem
internal float
RunDealtScale(app_state *AppState, player_slot *Attacker, world_entity *Target)
{
    float Result = 1.f + Attacker->RunAuraDamage;
    float Share = Target->MaxHp > 0.f ? Target->Hp / Target->MaxHp : 1.f;
    if (Share < RUN_EXECUTE_BELOW)
    {
        Result *= RunScale(RunEffectShare(Attacker, RunEffect_Execute));
    }
    if (Share > RUN_OPENER_ABOVE)
    {
        Result *= RunScale(RunEffectShare(Attacker, RunEffect_Opener));
    }
    dungeon_run *Run = AppState->Dungeon;
    bool32 Boss = Run && Run->BossSerial && Target->MonsterSerial == Run->BossSerial;
    Result *= RunScale(RunEffectShare(Attacker, Boss ? RunEffect_Bossbane : RunEffect_Packbane));
    world_entity *Self = Attacker->Entity;
    if (Self && Self->MaxHp > 0.f && Self->Hp < RUN_DESPERATE_BELOW * Self->MaxHp)
    {
        Result *= RunScale(RunEffectShare(Attacker, RunEffect_Desperate));
    }
    if ((Attacker->RunHits + 1) % RUN_CADENCE_EVERY == 0)
    {
        Result *= RunScale(RunEffectShare(Attacker, RunEffect_Cadence));
    }
    if (Attacker->RunFrenzySeconds > 0.f)
    {
        Result *= RunScale(RunEffectShare(Attacker, RunEffect_Frenzy));
    }
    if (Run && Run->FightingRoom && Run->MeterSeconds < RUN_VANGUARD_SECONDS)
    {
        Result *= RunScale(RunEffectShare(Attacker, RunEffect_Vanguard));
    }
    u32 Rooms = Minimum(AppState->DungeonRoomsCleared, (u32)RUN_GLORY_ROOMS);
    Result *= RunScale((float)Rooms * RunEffectShare(Attacker, RunEffect_Glory));
    return Result;
}

// NOTE(zoubir): from RoleTalentTakenScale: the share of a hit Player
// still takes after Last Breath and its allies' Aura
internal float
RunTakenScale(player_slot *Slot, world_entity *Player)
{
    float Result = 1.f - Slot->RunAuraArmor;
    if (Player && Player->MaxHp > 0.f && Player->Hp < RUN_DESPERATE_BELOW * Player->MaxHp)
    {
        Result *= 1.f - RunEffectShare(Slot, RunEffect_LastBreath);
    }
    Result = Maximum(0.2f, Result);
    return Result;
}

// NOTE(zoubir): the share of threat Attacker's damage makes after Menace
// or Unseen
inline float
RunThreatScale(player_slot *Attacker)
{
    float Result = RunScale(RunEffectShare(Attacker, RunEffect_Threat));
    return Result;
}

// NOTE(zoubir): from HealPlayer: the share of a heal Target gets after its
// own Receptive or Blood Pact
inline float
RunHealTakenScale(app_state *AppState, world_entity *Target)
{
    float Result = 1.f;
    if (Target->Type == EntityType_Player && Target->PlayerIndex < MAX_PLAYERS)
    {
        Result = Maximum(0.f, 1.f + RunEffectShare(&AppState->Players[Target->PlayerIndex],
                                                   RunEffect_HealTaken));
    }
    return Result;
}

// NOTE(zoubir): from HealPlayer: Overheal health went past Target's full
// bar from the healer in slot By; Overflow turns a share of it into a
// ward, up to a quarter of Target's health
internal void
OnRunOverheal(app_state *AppState, u32 By, world_entity *Target, float Overheal)
{
    if (By >= MAX_PLAYERS || !(Overheal > 0.f) || Target->Type != EntityType_Player ||
        Target->PlayerIndex >= MAX_PLAYERS)
    {
        return;
    }
    float Overflow = RunEffectShare(&AppState->Players[By], RunEffect_Overflow);
    if (Overflow > 0.f)
    {
        player_slot *Healed = &AppState->Players[Target->PlayerIndex];
        Healed->WardAbsorb = Minimum(Healed->WardAbsorb + Overflow * Overheal, 0.25f * Target->MaxHp);
        Healed->WardFull = Maximum(Healed->WardFull, Healed->WardAbsorb);
    }
}
