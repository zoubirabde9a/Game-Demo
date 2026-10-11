/* Stat talents (role_talents.cpp): a talent of a class's catalog whose
   every rank adds a share of one stat, wherever the class tree holds it
   (a wild slot rolls them too). Which stat a talent raises is the class's
   <Class>TalentStats row (role_kits/<class>_defs.cpp); talents with code
   of their own are RoleStat_None there. Each stat is read in one place:

   Damage     RoleTalentDealtScale        +4% damage dealt a rank
   Armor      RoleTalentTakenScale        -4% damage taken a rank
   Vitality   ApplyRoleToPlayer, here     +6% health a rank, on top of
                                          the level's (RoleMaxHealth)
   Haste      RoleSpellCooldown           -4% every class spell's cooldown
   Healing    HealPlayer                  +6% healing given a rank
   Swiftness  RunSpeedScale               +3% run speed a rank
   Lifesteal  OnRoleHit                   2% of the damage dealt back as
                                          health a rank

   A health talent changes the body's health the moment it is learned or
   reset (RefreshRoleHealth), keeping the health it is missing.

   The run talents' plain stats (run_tree/run_mods.cpp) add to the same seven
   through RoleStatShare. */

static_assert(RoleStat_Damage == RunEffect_Damage && RoleStat_Lifesteal == RunEffect_Leech &&
              RoleStat_Count == RunEffect_Leech + 1, "the run tree's stats are the class tree's");

// NOTE(zoubir): what one rank adds, by role_stat
global_variable float RoleStatPerRank[RoleStat_Count] =
{
    0.f, 0.04f, 0.04f, 0.06f, 0.04f, 0.06f, 0.03f, 0.02f,
};

// NOTE(zoubir): by player_role, then catalog talent
global_variable u8 *RoleTalentStats[PlayerRole_Count] =
{
    StrikerTalentStats, TankTalentStats, HealerTalentStats,
    RangerTalentStats, BerserkerTalentStats, ShadowbladeTalentStats,
    StormcallerTalentStats,
    DuelistTalentStats,
    FrostMageTalentStats,
    DruidTalentStats,
};

// NOTE(zoubir): the share of Stat that Slot's ranks add, 0 for none
internal float
RoleStatShare(player_slot *Slot, u32 Stat)
{
    float Result = 0.f;
    if (Slot && Slot->Role < PlayerRole_Count && Stat < RoleStat_Count)
    {
        u8 *Stats = RoleTalentStats[Slot->Role];
        for(u32 Index = 0; Index < CLASS_TREE_SLOTS; Index++)
        {
            u32 Rank = ClassTreeRank(Slot, Index);
            u32 Talent = Rank ? ClassContentTalent(ClassContentAt(Slot, Index)) : CLASS_TALENTS;
            if (Talent < CLASS_TALENTS && Stats[Talent] == Stat)
            {
                Result += RoleStatPerRank[Stat] * (float)Rank;
            }
        }
        // NOTE(zoubir): and the second tree's plain stats, which are the
        // same seven in the same order (run_tree/run_mods.cpp)
        Result += RunEffectShare(Slot, Stat);
    }
    return Result;
}

// NOTE(zoubir): what each level past the first adds to a class's health,
// as a share of its role's: 58% more at the top level. Not a tank's, whose
// health is already about twice the others' and grows with its tree. On
// the Aurora Rift and the Starless Deep the others fell twice as often as
// the tank and one fight in four wiped; with this, one in eleven
// (miscalance.bat, PROBE_MAP=rift, 16 seeds), and the first three
// levels, where the party is low, hardly change
#define CLASS_HEALTH_PER_LEVEL 0.02f

// NOTE(zoubir): Slot's health in a run: its role's, raised by its level
// (CLASS_HEALTH_PER_LEVEL) and by Vitality
internal float
RoleMaxHealth(player_slot *Slot)
{
    u32 Levels = Slot->Level > 1 ? Slot->Level - 1 : 0;
    float Growth = RoleKindOf(Slot->Role) == RoleKind_Tank ? 0.f :
        CLASS_HEALTH_PER_LEVEL * (float)Levels;
    float Result = GetRoleDef(Slot->Role)->MaxHp *
        (1.f + Growth + RoleStatShare(Slot, RoleStat_Vitality));
    return Result;
}

internal void
RefreshRoleHealth(app_state *AppState, player_slot *Slot)
{
    world_entity *Player = Slot->Entity;
    if (!IsDungeon(AppState) || !Player || !Player->IsPresent)
    {
        return;
    }
    float Missing = Player->MaxHp - Player->Hp;
    Player->MaxHp = RoleMaxHealth(Slot);
    if (Player->Hp > 0.f)
    {
        Player->Hp = Minimum(Player->MaxHp, Maximum(1.f, Player->MaxHp - Missing));
    }
}

// NOTE(zoubir): the share of a class spell's cooldown left after Haste
inline float
RoleStatCooldownScale(player_slot *Slot)
{
    float Result = Maximum(0.f, 1.f - RoleStatShare(Slot, RoleStat_Haste));
    return Result;
}

// NOTE(zoubir): Player's run speed from its class's Swiftness, for
// RunSpeedScale; 1 outside a run
internal float
RoleRunScale(app_state *AppState, world_entity *Player)
{
    float Result = 1.f;
    if (IsDungeon(AppState) && Player && Player->Type == EntityType_Player &&
        Player->PlayerIndex < MAX_PLAYERS)
    {
        Result += RoleStatShare(&AppState->Players[Player->PlayerIndex], RoleStat_Swiftness);
    }
    return Result;
}
