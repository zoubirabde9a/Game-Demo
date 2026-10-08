/* Role talents (dungeon.cpp): each class's own tree, the only one a
   dungeon run shows. The six slots are Talent_RoleFirst on in
   sim/progression/talents.cpp, the same shape for every class: two
   talents of two ranks on the first tier, two of one on the second, then
   one on the third and one on the fourth. What a slot is depends on the
   player's class, so picking another class gives its points back
   (SetPlayerRole, roles.cpp).

   Two slots unlock a spell (RoleSpells, role_abilities.cpp): the second
   slot of the first tier unlocks the class's C spell, and its second rank
   strengthens it; the third tier unlocks the V spell. A and R are the
   class's own from the start.

   Bulwark (tank)
     Iron Skin      takes less damage
     Intercept      C: leap to an ally; rank 2 wards the ally too
     Provoke        Taunt comes back sooner and reaches farther
     Bastion        Shield Slam's wall lasts longer and stuns longer
     Last Stand     V: heal and stand behind Shield Wall
     Shatter Armor  Sunder makes monsters take more, for longer
   Mender (healer)
     Swift Mending  Mending Bolt heals more
     Sanctuary      C: a healing circle; rank 2 widens it, heals more
     Deep Ward      Ward absorbs more
     Renewal        Mending Bolt also heals over a few seconds
     Radiance       V: heal and ward every ally round the healer
     Miracle        revives in half the time, with more health
   Striker (damage)
     Pyromancer     deals more damage
     Detonate       C: blow up Searing marks; rank 2, more a stack
     Wildfire       Meteor's ground burns longer, keeping marks alive
     Executioner    hits harder on monsters close to death
     Combustion     V: a few seconds of far more damage
     Overload       detonating a full mark gives back part of Detonate's
                    cooldown

   The kits read the ranks through RoleRank; the numbers per rank are
   here. */

enum tank_talent
{
    TankTalent_IronSkin,
    TankTalent_Intercept,
    TankTalent_Provoke,
    TankTalent_Bastion,
    TankTalent_LastStand,
    TankTalent_ShatterArmor,
};

enum healer_talent
{
    HealerTalent_SwiftMending,
    HealerTalent_Sanctuary,
    HealerTalent_DeepWard,
    HealerTalent_Renewal,
    HealerTalent_Radiance,
    HealerTalent_Miracle,
};

enum striker_talent
{
    StrikerTalent_Pyromancer,
    StrikerTalent_Detonate,
    StrikerTalent_Wildfire,
    StrikerTalent_Executioner,
    StrikerTalent_Combustion,
    StrikerTalent_Overload,
};

// NOTE(zoubir): the slots that unlock a spell, the same for every class:
// the C spell, then the V spell
#define ROLE_TALENT_C_SPELL 1
#define ROLE_TALENT_V_SPELL 4

// NOTE(zoubir): per rank, or once taken
#define IRON_SKIN_SHARE 0.06f
#define PROVOKE_COOLDOWN 1.5f
#define PROVOKE_REACH 0.15f
#define BASTION_WALL_SECONDS 2.f
#define BASTION_STUN 0.5f
// NOTE(zoubir): Intercept's second rank wards the ally it lands by
#define GUARDIAN_WARD 30.f
#define SHATTER_SHARE 0.1f
#define SHATTER_SECONDS 2.f
#define SWIFT_MENDING_SHARE 0.15f
#define DEEP_WARD_ABSORB 12.f
#define RENEWAL_SECONDS 4.f
#define RENEWAL_PER_SECOND 6.f
// NOTE(zoubir): Sanctuary's second rank
#define HALLOWED_RADIUS 1.3f
#define HALLOWED_HEAL 1.5f
#define MIRACLE_SECONDS 1.5f
#define MIRACLE_HP_SHARE 0.7f
#define PYROMANCER_SHARE 0.06f
// NOTE(zoubir): Detonate's second rank: this much more a Searing stack
#define SEARING_HEAT_PER_STACK 5.f
#define WILDFIRE_SECONDS 2.f
#define EXECUTIONER_BELOW 0.3f
#define EXECUTIONER_SHARE 0.35f
#define OVERLOAD_SECONDS 3.f

// NOTE(zoubir): each class's branch by slot; the branch, tier, column
// and ranks match the slot's row in TalentDefs
global_variable talent_def StrikerTalentDefs[ROLE_TALENTS] =
{
        {"Pyromancer", "All your damage is higher", "+6% damage",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Detonate", "C: blow up the Searing marks on a foe; in fire, every marked foe there",
         "rank 2: +5 a stack", TalentBranch_Role, 0, 1, 2, 0},
        {"Wildfire", "Meteor's ground burns longer, keeping marks alive", "+2 s of burning ground",
         TalentBranch_Role, 1, 0, 1, 0},
        {"Executioner", "Hit harder on monsters under 30% health", "+35% damage on them",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Combustion", "V: for 6 s every hit you land is 40% harder", "a new spell",
         TalentBranch_Role, 2, 0, 1, 0},
        {"Overload", "Detonating a full mark gives back 3 s of Detonate",
         "-3 s Detonate on a full mark", TalentBranch_Role, 3, 0, 1, 0},
};

global_variable talent_def TankTalentDefs[ROLE_TALENTS] =
{
        {"Iron Skin", "You take less damage", "-6% damage taken",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Intercept", "C: leap to an ally and pull their foes onto you",
         "rank 2: ward the ally for 30", TalentBranch_Role, 0, 1, 2, 0},
        {"Provoke", "Taunt comes back sooner and reaches farther",
         "-1.5 s Taunt cooldown, +15% reach", TalentBranch_Role, 1, 0, 1, 0},
        {"Bastion", "Shield Slam's wall holds longer, its stun lasts longer",
         "+2 s Shield Wall, +0.5 s stun", TalentBranch_Role, 1, 1, 1, 0},
        {"Last Stand", "V: heal 30% of your health and stand behind Shield Wall for 6 s",
         "a new spell", TalentBranch_Role, 2, 0, 1, 0},
        {"Shatter Armor", "Sunder bites deeper and lasts longer", "Sunder +10%, +2 s",
         TalentBranch_Role, 3, 0, 1, 0},
};

global_variable talent_def HealerTalentDefs[ROLE_TALENTS] =
{
        {"Swift Mending", "Mending Bolt heals more", "+15% Mending Bolt healing",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Sanctuary", "C: a circle at the cursor that heals allies inside",
         "rank 2: +30% radius, +50% healing", TalentBranch_Role, 0, 1, 2, 0},
        {"Deep Ward", "Ward absorbs more damage", "+12 absorbed",
         TalentBranch_Role, 1, 0, 1, 0},
        {"Renewal", "Mending Bolt also heals 24 over 4 s", "healing over time",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Radiance", "V: heal and ward every ally around you", "a new spell",
         TalentBranch_Role, 2, 0, 1, 0},
        {"Miracle", "Revive the fallen in 1.5 s, at 70% health", "faster, stronger revives",
         TalentBranch_Role, 3, 0, 1, 0},
};

// NOTE(zoubir): the later classes' numbers, talents and spells
#include "role_kits/ranger_defs.cpp"
#include "role_kits/berserker_defs.cpp"
#include "role_kits/shadowblade_defs.cpp"

// NOTE(zoubir): by player_role, then slot
global_variable talent_def *RoleTalentDefs[PlayerRole_Count] =
{
    StrikerTalentDefs, TankTalentDefs, HealerTalentDefs,
    RangerTalentDefs, BerserkerTalentDefs, ShadowbladeTalentDefs,
};

// NOTE(zoubir): the later classes' talents in play (role_kits/class_kits.cpp)
internal float ClassDealtScale(player_slot *Slot, world_entity *Target);
internal float ClassTakenScale(player_slot *Slot, world_entity *Player);

// NOTE(zoubir): Slot's rank in slot Index of Role's branch; 0 when Slot
// plays another role
inline u32
RoleRank(player_slot *Slot, u32 Role, u32 Index)
{
    u32 Result = (Slot && Slot->Role == Role && Index < ROLE_TALENTS) ?
        Slot->Ranks[Talent_RoleFirst + Index] : 0;
    return Result;
}

// NOTE(zoubir): the talent as Slot's panel shows it: a role slot is the
// row of Slot's role, the rest are TalentDefs
internal talent_def *
ShownTalentDef(player_slot *Slot, u32 Talent)
{
    talent_def *Result = &TalentDefs[Talent < Talent_Count ? Talent : 0];
    if (IsRoleTalent(Talent))
    {
        u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
        Result = &RoleTalentDefs[Role][Talent - Talent_RoleFirst];
    }
    return Result;
}

// NOTE(zoubir): the share of a hit a player takes after its role's
// talents (Iron Skin), for DungeonScaleDamage
internal float
RoleTalentTakenScale(player_slot *Slot, world_entity *Player)
{
    float Result = 1.f - IRON_SKIN_SHARE * (float)RoleRank(Slot, PlayerRole_Tank, TankTalent_IronSkin);
    Result *= ClassTakenScale(Slot, Player);
    return Result;
}

// NOTE(zoubir): the share of a hit on Target a player deals after its
// role's talents (Pyromancer, Executioner)
internal float
RoleTalentDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + PYROMANCER_SHARE *
        (float)RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Pyromancer);
    if (RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Executioner) && Target->MaxHp > 0.f &&
        Target->Hp < EXECUTIONER_BELOW * Target->MaxHp)
    {
        Result *= 1.f + EXECUTIONER_SHARE;
    }
    Result *= ClassDealtScale(Slot, Target);
    return Result;
}
