/* Role talents (dungeon.cpp): each class's tree, the only one a dungeon
   run shows: two branches, Talent_RoleFirst on and Talent_RunFirst on,
   laid out and rolled in class_tree.cpp (docs/class-trees.md).

   A class's catalog of talents is role_kits/<class>_defs.cpp: the
   talents its tree can hold, by its own enum, with their numbers. A talent
   with code of its own is read in the kit through RoleRank, which finds it
   wherever the tree put it; a stat talent raises one stat a rank
   (role_stats.cpp). Some talents unlock a spell (role_spell.Unlock), and
   each branch offers two of those of which a player takes one.

   What a talent is depends on the player's class, so picking another
   class gives its points back (SetPlayerRole, roles.cpp). */

// NOTE(zoubir): the most talents a class's catalog holds
#define CLASS_TALENTS 20

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

// NOTE(zoubir): Slot's ranks in talent Index of Role's catalog; 0 when Slot
// plays another role
inline u32
RoleRank(player_slot *Slot, u32 Role, u32 Index)
{
    u32 Result = (Slot && Slot->Role == Role && Index < CLASS_TALENTS) ?
        ClassTalentRank(Slot, Index) : 0;
    return Result;
}

// NOTE(zoubir): the talent as Slot's panel shows it: a role slot is the
// row of Slot's role, the rest are TalentDefs
internal talent_def *
ShownTalentDef(player_slot *Slot, u32 Talent)
{
    talent_def *Result = &TalentDefs[Talent < Talent_Count ? Talent : 0];
    if (IsClassTalent(Talent))
    {
        Result = ShownClassTalentDef(Slot, Talent);
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
