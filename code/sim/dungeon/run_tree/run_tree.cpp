/* The second tree (role_talents.cpp): each class's other talent tree in
   a dungeon run, Talent_RunFirst on, beside the class tree. The two share
   the player's points (29 at the top level), so nobody fills both.

   Its twelve slots have the same shape for every class (TalentDefs in
   sim/progression/talents.cpp):

   tier 0   slot 0 fixed (3)        slot 1 wild (2)
   tier 1   slot 2 wild (2)         slot 3 fixed (3)
   tier 2   slot 4 fixed (3)        slot 5 wild (2)
   tier 3   slot 6 wild (2)         slot 7 fixed (3)
   tier 4   slot 8 fixed (3)        slot 9 wild (2)
   tier 5   slot 10 wild keystone   slot 11 the class's capstone

   A fixed slot is the same talent every run (RunTrees). A wild slot
   rolls one from the pool (run_mods.cpp) that fits the class's role
   kind: a minor talent, or for slot 10 a keystone with a cost. The roll
   is a hash of the player's TreeSeed, the class and the slot, so a
   different class rolls a different tree, and no minor wild slot gives
   an effect another slot already gives (a keystone may: its cost sets
   it apart). TreeSeed rolls again as
   each new run starts (RerollRunTree, from StartNextRoundMap); points in
   a wild slot stay there and buy whatever it rolled. */

#include "run_mods.cpp"

#define RUN_KEYSTONE_SLOT 10
// NOTE(zoubir): TreeSeed's bits, all a snapshot carries (net/protocol.h)
#define RUN_SEED_MASK 0xffffu

// NOTE(zoubir): by slot, whether it rolls
global_variable bool32 RunSlotWild[RUN_TALENTS] =
{
    false, true, true, false, false, true, true, false, false, true, true, false,
};

struct run_tree_def
{
    char *Name;
    // NOTE(zoubir): by slot, the fixed talent; RunMod_None in a wild slot
    u8 Fixed[RUN_TALENTS];
};

#define RUN_FIXED(A, B, C, D, E, Cap) \
    {RunMod_##A, 0, 0, RunMod_##B, RunMod_##C, 0, 0, RunMod_##D, RunMod_##E, 0, 0, RunMod_##Cap}

// NOTE(zoubir): by player_role
global_variable run_tree_def RunTrees[PlayerRole_Count] =
{
    {"Ashbringer", RUN_FIXED(KindledWrath, CinderSkin, FireWithin, Pyroclasm, Smoulder, PhoenixHeart)},
    {"Iron Vanguard", RUN_FIXED(Menacing, Stoneform, Retaliation, SteadyHeart, ShieldBrother, LivingFortress)},
    {"Grace", RUN_FIXED(Bountiful, SpiritWard, Serenity, Hymn, Sanctified, SaintsVigil)},
    {"Wildstalker", RUN_FIXED(Patience, BigGame, Trailwise, KillingRhythm, Trophy, ApexPredator)},
    {"Bloodrage", RUN_FIXED(CorneredBeast, Gorge, ThickHide, Savagery, RedMist, UndyingFury)},
    {"Nightfall", RUN_FIXED(Assassinate, Shroud, Ambush, QuickHands, Slip, DeathMark)},
    {"Tempest", RUN_FIXED(StaticBuild, StormFront, Conductor, Grounded, Surge, EyeOfTheStorm)},
    {"Flourish", RUN_FIXED(Footwork, Precision, Measured, CoupDeGrace, Panache, PerfectForm)},
    {"Rime", RUN_FIXED(ShatterPoint, GlacialSkin, DeepWinter, ColdCalculation, Permafrost, AbsoluteZero)},
    {"Grove", RUN_FIXED(Overgrowth, Barkskin, WildBloom, Verdant, Moonlit, HeartOfTheWild)},
};
#undef RUN_FIXED

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

// NOTE(zoubir): whether Mod's first effect is already one of Taken's
inline bool32
RunEffectTaken(u8 *Taken, u32 Count, u32 Mod)
{
    bool32 Result = false;
    for(u32 Index = 0; Index < Count && !Result; Index++)
    {
        Result = RunModDefs[Taken[Index]].Effect[0] == RunModDefs[Mod].Effect[0];
    }
    return Result;
}

// NOTE(zoubir): every slot's talent for Seed and Role into Out: the fixed
// ones, then each wild slot in turn from what is left of the pool
internal void
RollRunTree(u32 Seed, u32 Role, u8 *Out)
{
    u32 Kind = 1u << RoleKindOf(Role);
    u8 Taken[RUN_TALENTS];
    u32 TakenCount = 0;
    for(u32 Index = 0; Index < RUN_TALENTS; Index++)
    {
        Out[Index] = RunTrees[Role].Fixed[Index];
        if (Out[Index])
        {
            Taken[TakenCount++] = Out[Index];
        }
    }
    for(u32 Index = 0; Index < RUN_TALENTS; Index++)
    {
        if (!RunSlotWild[Index])
        {
            continue;
        }
        bool32 Keystone = Index == RUN_KEYSTONE_SLOT;
        u8 Pool[RunMod_WildLast - RunMod_WildFirst + 1];
        u32 PoolCount = 0;
        // NOTE(zoubir): a second pass lets a repeated effect in, so a
        // slot is never empty
        for(u32 Pass = 0; Pass < 2 && PoolCount == 0; Pass++)
        {
            for(u32 Mod = RunMod_WildFirst; Mod <= RunMod_WildLast; Mod++)
            {
                run_mod_def *Def = &RunModDefs[Mod];
                if ((Def->Kinds & Kind) && (Def->Keystone != 0) == Keystone &&
                    (Pass == 1 || Keystone || !RunEffectTaken(Taken, TakenCount, Mod)))
                {
                    Pool[PoolCount++] = (u8)Mod;
                }
            }
        }
        u32 Pick = PoolCount ?
            RunHash(Seed ^ RunHash(Role * 977u + Index * 131u + 1u)) % PoolCount : 0;
        Out[Index] = PoolCount ? Pool[Pick] : (u8)RunMod_KeenEdge;
        Taken[TakenCount++] = Out[Index];
    }
}

// NOTE(zoubir): Slot's tree as its seed and class roll it, kept on the
// slot until either changes
inline u8 *
RunTreeOf(player_slot *Slot)
{
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    if (Slot->RunRolledRole != Role + 1 || Slot->RunRolledSeed != Slot->TreeSeed)
    {
        RollRunTree(Slot->TreeSeed, Role, Slot->RunRolled);
        Slot->RunRolledRole = Role + 1;
        Slot->RunRolledSeed = Slot->TreeSeed;
    }
    return Slot->RunRolled;
}

// NOTE(zoubir): the talent in slot Index of Slot's second tree
inline u32
RunModAtSlot(player_slot *Slot, u32 Index)
{
    u32 Result = Index < RUN_TALENTS ? RunTreeOf(Slot)[Index] : RunMod_None;
    return Result;
}

// NOTE(zoubir): what Slot's second tree adds to Effect: every rank of
// every talent with it, times its amount a rank; 0 outside a run's class
internal float
RunEffectShare(player_slot *Slot, u32 Effect)
{
    float Result = 0.f;
    if (!Slot)
    {
        return Result;
    }
    u8 *Tree = 0;
    for(u32 Index = 0; Index < RUN_TALENTS; Index++)
    {
        u32 Rank = Slot->Ranks[Talent_RunFirst + Index];
        if (!Rank)
        {
            continue;
        }
        if (!Tree)
        {
            Tree = RunTreeOf(Slot);
        }
        run_mod_def *Def = &RunModDefs[Tree[Index]];
        for(u32 Part = 0; Part < 2; Part++)
        {
            if (Def->Effect[Part] == Effect)
            {
                Result += Def->PerRank[Part] * (float)Rank;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): from StartNextRoundMap as a new run starts, and when a
// player joins: the wild slots roll again. Salt is anything that differs
// run to run (the map, the experience)
internal void
RerollRunTree(player_slot *Slot, u32 SlotIndex, u32 Salt)
{
    Slot->TreeSeed = RunHash(Slot->TreeSeed ^ RunHash(Salt + SlotIndex * 7919u + 1u)) & RUN_SEED_MASK;
}

// NOTE(zoubir): the talent as the panel shows it: the slot's shape from
// TalentDefs with the rolled talent's name and line. What a rank adds is
// the tooltip's to write from the effects (RunModDefs), so PerRank is
// empty. A few calls' worth are kept, as the panel holds one at a time
internal talent_def *
ShownRunTalentDef(player_slot *Slot, u32 Talent)
{
    local_persist talent_def Defs[4];
    local_persist u32 Next;
    talent_def *Result = &Defs[Next++ % ArrayCount(Defs)];
    *Result = TalentDefs[Talent];
    run_mod_def *Mod = &RunModDefs[RunModAtSlot(Slot, Talent - Talent_RunFirst)];
    Result->Name = Mod->Name;
    Result->Summary = Mod->Summary;
    Result->PerRank = "";
    return Result;
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
// Packbane, Desperate, Cadence, Frenzy) and its allies' Anthem
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
