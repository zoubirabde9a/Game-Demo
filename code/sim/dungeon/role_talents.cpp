/* Role talents (dungeon.cpp): each class's own tree, the only one a
   dungeon run shows. Its twelve slots are Talent_RoleFirst on in
   sim/progression/talents.cpp, the same shape for every class, six tiers
   deep (ROLE_TALENT_TIERS):

   tier 0   slot 0 (2 ranks)    slot 1 (2, the C spell)
   tier 1   slot 2 (1)          slot 3 (1)
   tier 2   slot 4 (1, V spell) slot 6 (4)
   tier 3   slot 5 (1)          slot 7 (4)
   tier 4   slot 8 (4)          slot 9 (4)
   tier 5   slot 10 (4)         slot 11 (1, the capstone)

   29 points in all, one per level from 2 to 30, so a class's tree fills
   at the dungeon's top level. What a slot is depends on the player's
   class, so picking another class gives its points back (SetPlayerRole,
   roles.cpp).

   Two slots unlock a spell (RoleSpells, role_abilities.cpp): slot 1
   unlocks the class's C spell, and its second rank strengthens it; slot 4
   unlocks the V spell. A and R are the class's own from the start.

   Slots 6, 7, 9 and 10 are stat talents (role_stats.cpp): each rank adds
   a share of one stat (damage, armor, health, cooldowns, healing, run
   speed or life steal), set per class in <class>_defs.cpp. Slot 8 changes
   one of the class's spells and slot 11 is its capstone; those two, like
   slots 0 to 5, have code of their own in the class's kit.

   Each class's talents, their numbers and what they do are in
   role_kits/<class>_defs.cpp; the kits read the ranks through RoleRank.

   Each class has a second tree beside this one, partly random:
   run_tree/run_tree.cpp. */

// NOTE(zoubir): the slots that unlock a spell, the same for every class:
// the C spell, then the V spell
#define ROLE_TALENT_C_SPELL 1
#define ROLE_TALENT_V_SPELL 4

// NOTE(zoubir): the stat a stat talent raises (role_stats.cpp)
enum role_stat
{
    RoleStat_None,
    RoleStat_Damage,
    RoleStat_Armor,
    RoleStat_Vitality,
    RoleStat_Haste,
    RoleStat_Healing,
    RoleStat_Swiftness,
    RoleStat_Lifesteal,
    RoleStat_Count
};

// NOTE(zoubir): each class's numbers, talents and spells; its branch by
// slot has the branch, tier, column and ranks of the slot's row in
// TalentDefs
#include "role_kits/striker_defs.cpp"
#include "role_kits/tank_defs.cpp"
#include "role_kits/healer_defs.cpp"
#include "role_kits/ranger_defs.cpp"
#include "role_kits/berserker_defs.cpp"
#include "role_kits/shadowblade_defs.cpp"
#include "role_kits/stormcaller_defs.cpp"
#include "role_kits/duelist_defs.cpp"
#include "role_kits/frostmage_defs.cpp"
#include "role_kits/druid_defs.cpp"

// NOTE(zoubir): by player_role, then slot
global_variable talent_def *RoleTalentDefs[PlayerRole_Count] =
{
    StrikerTalentDefs, TankTalentDefs, HealerTalentDefs,
    RangerTalentDefs, BerserkerTalentDefs, ShadowbladeTalentDefs,
    StormcallerTalentDefs,
    DuelistTalentDefs,
    FrostMageTalentDefs,
    DruidTalentDefs,
};

// NOTE(zoubir): the second tree's talents, rolls and hit conditions
#include "run_tree/run_tree.cpp"
#include "role_stats.cpp"

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
    if (IsRunTalent(Talent))
    {
        Result = ShownRunTalentDef(Slot, Talent);
    }
    return Result;
}

// NOTE(zoubir): the share of a hit a player takes after its role's
// talents (Iron Skin, Armor), for DungeonScaleDamage
internal float
RoleTalentTakenScale(player_slot *Slot, world_entity *Player)
{
    float Result = 1.f - IRON_SKIN_SHARE * (float)RoleRank(Slot, PlayerRole_Tank, TankTalent_IronSkin);
    Result *= Maximum(0.f, 1.f - RoleStatShare(Slot, RoleStat_Armor));
    Result *= ClassTakenScale(Slot, Player);
    Result *= RunTakenScale(Slot, Player);
    return Result;
}

// NOTE(zoubir): the share of a hit on Target a player deals after its
// role's talents (Pyromancer, Executioner, Damage)
internal float
RoleTalentDealtScale(player_slot *Slot, world_entity *Target)
{
    float Result = 1.f + PYROMANCER_SHARE *
        (float)RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Pyromancer);
    Result *= 1.f + RoleStatShare(Slot, RoleStat_Damage);
    if (RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Executioner) && Target->MaxHp > 0.f &&
        Target->Hp < EXECUTIONER_BELOW * Target->MaxHp)
    {
        Result *= 1.f + EXECUTIONER_SHARE;
    }
    Result *= ClassDealtScale(Slot, Target);
    return Result;
}
