/* Duel perks: what sits in the duel tree's wild slots (talents.cpp). Each
   branch keeps its abilities where they always are; its other slots roll
   as each match starts, the minor ones (two ranks) from the branch's
   minor pool and its capstone (one rank) from its keystone pool, so no
   two matches offer quite the same tree. A keystone is a big effect, some
   with a cost.

   The roll is a hash of the match's DuelSeed (16 bits, on every player
   slot, which a snapshot carries), the branch and the slot, so everyone
   in a match sees the same tree and the client knows it from one number.
   The simulation rolls the seed the first time anyone has a point to
   spend in a new match (RollDuelSeed): before that nobody can learn
   anything, so the tree never changes under a point already spent.

   What each perk does in play is talents/effects.cpp and the code it
   names there; every number is a #define below. */

enum duel_perk
{
    DuelPerk_None,

    // NOTE(zoubir): Fire's minors, then its keystones
    DuelPerk_SwiftFlames,
    DuelPerk_Kindling,
    DuelPerk_Searing,
    DuelPerk_Scorch,
    DuelPerk_TwinFlame,
    DuelPerk_Pyre,
    DuelPerk_Overcharge,

    // NOTE(zoubir): Motion's
    DuelPerk_FleetFoot,
    DuelPerk_Quickstep,
    DuelPerk_KeenEdge,
    DuelPerk_Hunter,
    DuelPerk_Momentum,
    DuelPerk_Adrenaline,
    DuelPerk_Phantom,

    // NOTE(zoubir): Guard's
    DuelPerk_Ward,
    DuelPerk_ThickSkin,
    DuelPerk_Mending,
    DuelPerk_Steadfast,
    DuelPerk_SecondWind,
    DuelPerk_Thorns,
    DuelPerk_Bastion,

    DuelPerk_Count
};

struct duel_perk_def
{
    char *Name;
    char *Summary;
    // NOTE(zoubir): what one rank adds, for the tooltip
    char *PerRank;
    u32 Branch;
    bool32 Keystone;
};

global_variable duel_perk_def DuelPerkDefs[DuelPerk_Count] =
{
    {"", "", "", TalentBranch_Count, false},

    {"Swift Flames", "Fireballs fly faster and farther", "+15% fireball speed and range", TalentBranch_Fire},
    {"Kindling", "The fireball comes back sooner", "-12% fireball cooldown", TalentBranch_Fire},
    {"Searing", "Fireballs burn hotter", "+12% fireball damage", TalentBranch_Fire},
    {"Scorch", "A fireball hit slows its target", "+1.2 s slow on a fireball hit", TalentBranch_Fire},
    {"Twin Flame", "Each cast throws two fireballs, fanned", "a second fireball", TalentBranch_Fire, true},
    {"Pyre", "Fireballs hit 40% harder but come back 15% slower", "bigger, slower fireballs", TalentBranch_Fire, true},
    {"Overcharge", "Everything you deal hits 20% harder; you take 5% more", "more damage dealt and taken", TalentBranch_Fire, true},

    {"Fleet Foot", "You run faster", "+6% run speed", TalentBranch_Motion},
    {"Quickstep", "Blink comes back sooner", "-12% blink cooldown", TalentBranch_Motion},
    {"Keen Edge", "Sword cuts and kunai hit harder", "+10% sword and kunai damage", TalentBranch_Motion},
    {"Hunter", "The kunai comes back sooner", "-15% kunai cooldown", TalentBranch_Motion},
    {"Momentum", "Every hit you land on a player takes 0.4 s off blink, slam and the kunai", "cooldowns back on a hit", TalentBranch_Motion, true},
    {"Adrenaline", "Under half health you run 35% faster", "speed when hurt", TalentBranch_Motion, true},
    {"Phantom", "A blink leaves you untouchable for 0.6 s; blink waits 10% longer", "untouchable after a blink", TalentBranch_Motion, true},

    {"Ward", "A charge that takes one hit whole", "recharges sooner", TalentBranch_Guard},
    {"Thick Skin", "You take less damage", "-7% damage taken", TalentBranch_Guard},
    {"Mending", "Your wounds close as you fight", "+0.8 health a second", TalentBranch_Guard},
    {"Steadfast", "Stuns and freezes hold you for less", "-25% stun time", TalentBranch_Guard},
    {"Second Wind", "Once a round, a killing blow leaves you on 1 health, shielded 1 s", "cheat death once a round", TalentBranch_Guard, true},
    {"Thorns", "A player who hurts you takes 30% of it back", "damage back", TalentBranch_Guard, true},
    {"Bastion", "The shield heals 16 as it goes up; it waits 15% longer", "healing shield", TalentBranch_Guard, true},
};

// NOTE(zoubir): the numbers, per rank for the minors
#define DUEL_SWIFT_FLAMES_SCALE 0.15f
#define DUEL_KINDLING_COOLDOWN 0.12f
#define DUEL_SEARING_DAMAGE 0.12f
#define DUEL_SCORCH_SLOW 1.2f
#define DUEL_TWIN_FLAME_SPREAD 0.12f
#define DUEL_PYRE_DAMAGE 0.4f
#define DUEL_PYRE_COOLDOWN 0.15f
#define DUEL_OVERCHARGE_DAMAGE 0.2f
#define DUEL_OVERCHARGE_TAKEN 0.05f
#define DUEL_FLEET_FOOT_SCALE 0.06f
#define DUEL_QUICKSTEP_COOLDOWN 0.12f
#define DUEL_KEEN_EDGE_DAMAGE 0.1f
#define DUEL_HUNTER_COOLDOWN 0.15f
#define DUEL_MOMENTUM_REFUND 0.4f
#define DUEL_ADRENALINE_SCALE 0.35f
#define DUEL_PHANTOM_SECONDS 0.6f
#define DUEL_PHANTOM_COOLDOWN 0.1f
global_variable float DuelWardSeconds[] = {0.f, 16.f, 10.f};
#define DUEL_THICK_SKIN_SHARE 0.07f
#define DUEL_MENDING_PER_SECOND 0.8f
#define DUEL_STEADFAST_SHARE 0.25f
#define DUEL_SECOND_WIND_SHIELD 1.f
#define DUEL_THORNS_SHARE 0.3f
#define DUEL_BASTION_HEAL 16.f
#define DUEL_BASTION_COOLDOWN 0.15f

// NOTE(zoubir): the wild slots of each branch, minors first, the
// keystone last; TalentBranch_Count past the end
global_variable u32 DuelWildSlots[TALENT_GAME_BRANCHES][4] =
{
    {Talent_FireWildA, Talent_FireWildB, Talent_FireKeystone, Talent_Count},
    {Talent_MotionWildA, Talent_MotionWildB, Talent_MotionKeystone, Talent_Count},
    {Talent_GuardWildA, Talent_GuardWildB, Talent_GuardWildC, Talent_GuardKeystone},
};
// NOTE(zoubir): the seed's bits, all a snapshot carries
#define DUEL_SEED_MASK 0xffffu

inline bool32
IsDuelWildTalent(u32 Talent)
{
    bool32 Result = false;
    for(u32 Branch = 0; Branch < TALENT_GAME_BRANCHES; Branch++)
    {
        for(u32 Index = 0; Index < ArrayCount(DuelWildSlots[0]); Index++)
        {
            Result |= DuelWildSlots[Branch][Index] == Talent;
        }
    }
    return Result;
}

inline u32
DuelHash(u32 X)
{
    X ^= X >> 16; X *= 0x7feb352du;
    X ^= X >> 15; X *= 0x846ca68bu;
    X ^= X >> 16;
    return X;
}

// NOTE(zoubir): the perk Seed puts in Talent's slot, DuelPerk_None for a
// slot that is no wild one. The branch's minors are shuffled by the seed
// and dealt to its minor slots in order, so none repeats in a branch
internal u32
DuelPerkAt(u32 Seed, u32 Talent)
{
    u32 Result = DuelPerk_None;
    for(u32 Branch = 0; Branch < TALENT_GAME_BRANCHES && !Result; Branch++)
    {
        u32 Minors[DuelPerk_Count];
        u32 Keystones[DuelPerk_Count];
        u32 MinorCount = 0;
        u32 KeystoneCount = 0;
        for(u32 Perk = 1; Perk < DuelPerk_Count; Perk++)
        {
            if (DuelPerkDefs[Perk].Branch == Branch)
            {
                if (DuelPerkDefs[Perk].Keystone)
                {
                    Keystones[KeystoneCount++] = Perk;
                }
                else
                {
                    Minors[MinorCount++] = Perk;
                }
            }
        }
        u32 Hash = DuelHash((Seed & DUEL_SEED_MASK) * 2654435761u + Branch * 40503u + 1u);
        for(u32 Index = MinorCount; Index > 1; Index--)
        {
            Hash = DuelHash(Hash + Index);
            u32 Other = Hash % Index;
            u32 Swap = Minors[Index - 1];
            Minors[Index - 1] = Minors[Other];
            Minors[Other] = Swap;
        }
        u32 MinorSlot = 0;
        for(u32 Index = 0; Index < ArrayCount(DuelWildSlots[0]); Index++)
        {
            u32 Wild = DuelWildSlots[Branch][Index];
            if (Wild == Talent_Count)
            {
                continue;
            }
            bool32 Keystone = TalentDefs[Wild].MaxLevel == 1;
            u32 Perk = Keystone ? Keystones[DuelHash(Hash ^ 0x5bd1e995u) % KeystoneCount] :
                Minors[MinorSlot++ % MinorCount];
            if (Wild == Talent)
            {
                Result = Perk;
            }
        }
    }
    return Result;
}

// NOTE(zoubir): Slot's ranks in Perk, wherever this match's tree put it;
// 0 when the tree has it nowhere
internal u32
DuelPerkRank(player_slot *Slot, u32 Perk)
{
    u32 Result = 0;
    if (Slot)
    {
        u32 Branch = DuelPerkDefs[Perk].Branch;
        for(u32 Index = 0; Index < ArrayCount(DuelWildSlots[0]) && Branch < TALENT_GAME_BRANCHES; Index++)
        {
            u32 Wild = DuelWildSlots[Branch][Index];
            if (Wild != Talent_Count && Slot->Ranks[Wild] &&
                DuelPerkAt(Slot->DuelSeed, Wild) == Perk)
            {
                Result += Slot->Ranks[Wild];
            }
        }
    }
    return Result;
}

// NOTE(zoubir): the slot this match's tree put Perk in, Talent_Count for
// none (tests, and the bots' notes)
internal u32
DuelPerkTalent(u32 Seed, u32 Perk)
{
    u32 Result = Talent_Count;
    u32 Branch = DuelPerkDefs[Perk].Branch;
    for(u32 Index = 0; Index < ArrayCount(DuelWildSlots[0]) && Branch < TALENT_GAME_BRANCHES; Index++)
    {
        u32 Wild = DuelWildSlots[Branch][Index];
        if (Wild != Talent_Count && DuelPerkAt(Seed, Wild) == Perk)
        {
            Result = Wild;
        }
    }
    return Result;
}

// NOTE(zoubir): a seed whose tree holds Perk, for tests and the probe
internal u32
DuelSeedWith(u32 Perk, u32 From = 1)
{
    u32 Result = 0;
    for(u32 Seed = From; Seed <= DUEL_SEED_MASK && !Result; Seed++)
    {
        if (DuelPerkTalent(Seed, Perk) != Talent_Count)
        {
            Result = Seed;
        }
    }
    return Result;
}

// NOTE(zoubir): the talent as the panel shows it: the slot's shape with
// this match's perk in it. A few calls' worth are kept, as the panel holds
// one at a time
internal talent_def *
ShownDuelTalentDef(player_slot *Slot, u32 Talent)
{
    local_persist talent_def Defs[4];
    local_persist u32 Next;
    talent_def *Result = &Defs[Next++ % ArrayCount(Defs)];
    *Result = TalentDefs[Talent];
    duel_perk_def *Perk = &DuelPerkDefs[DuelPerkAt(Slot->DuelSeed, Talent)];
    Result->Name = Perk->Name;
    Result->Summary = Perk->Summary;
    Result->PerRank = Perk->PerRank;
    return Result;
}

// NOTE(zoubir): the match's duel tree rolled
// once, before the first point of the match goes in: nobody can have
// learned from it before. It is made from the ticks played, so a replay
// rolls the same tree and every match its own
internal void
RollDuelTree(app_state *AppState)
{
    if (!AppState->DuelSeed && !AppState->Dungeon)
    {
        AppState->DuelSeed = (DuelHash(AppState->DuelTicks * 2654435761u + 977u) &
                              DUEL_SEED_MASK) | 1u;
    }
}
