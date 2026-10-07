/* Role talents (dungeon.cpp): each role's own branch of the talent tree,
   shown as a fourth column in a dungeon run. The six slots are
   Talent_RoleFirst on in sim/progression/talents.cpp, the same shape for
   every role: two talents of two ranks on the first tier, two of one on
   the second, then one on the third and one on the fourth. What a slot
   is depends on the player's role, so picking another role gives its
   points back (SetPlayerRole, roles.cpp).

   Each branch deepens its role's part in the damage race (docs/
   dungeon-plan.md, "How the roles feed each other") as much as its
   own job, and ends in a capstone that rewards playing the rotation.

   Bulwark (tank)
     Iron Skin      takes less damage
     Provoke        Taunt comes back sooner and reaches farther
     Bastion        Shield Slam's wall lasts longer and stuns longer
     Guardian       Intercept wards the ally it lands by
     Rally          Shield Slam shields allies more, farther out
     Shatter Armor  Sunder makes monsters take more, for longer
   Mender (healer)
     Swift Mending  Mending Bolt heals more
     Deep Ward      Ward absorbs more
     Renewal        Mending Bolt also heals over a few seconds
     Hallowed Ground  Sanctuary is wider and heals more
     Inspiration    Ward's damage bonus doubles and reaches the allies
                    round the warded one
     Miracle        revives in half the time, with more health
   Striker (damage)
     Pyromancer     deals more damage
     Searing Heat   each Searing stack detonates for more
     Wildfire       Inferno's ground burns longer, keeping marks alive
     Executioner    hits harder on monsters close to death
     Cataclysm      Inferno is wider, marking more of a pack
     Overload       detonating a full mark gives back part of Detonate's
                    cooldown

   The kits read the ranks through RoleRank; the numbers per rank are
   here. */

enum tank_talent
{
    TankTalent_IronSkin,
    TankTalent_Provoke,
    TankTalent_Bastion,
    TankTalent_Guardian,
    TankTalent_Rally,
    TankTalent_ShatterArmor,
};

enum healer_talent
{
    HealerTalent_SwiftMending,
    HealerTalent_DeepWard,
    HealerTalent_Renewal,
    HealerTalent_HallowedGround,
    HealerTalent_Inspiration,
    HealerTalent_Miracle,
};

enum striker_talent
{
    StrikerTalent_Pyromancer,
    StrikerTalent_SearingHeat,
    StrikerTalent_Wildfire,
    StrikerTalent_Executioner,
    StrikerTalent_Cataclysm,
    StrikerTalent_Overload,
};

// NOTE(zoubir): per rank, or once taken
#define IRON_SKIN_SHARE 0.06f
#define PROVOKE_COOLDOWN 1.5f
#define PROVOKE_REACH 0.15f
#define BASTION_WALL_SECONDS 2.f
#define BASTION_STUN 0.5f
#define GUARDIAN_WARD 30.f
#define RALLY_TALENT_SHARE 0.4f
#define RALLY_TALENT_RADIUS 240.f
#define SHATTER_SHARE 0.1f
#define SHATTER_SECONDS 2.f
#define SWIFT_MENDING_SHARE 0.15f
#define DEEP_WARD_ABSORB 12.f
#define RENEWAL_SECONDS 4.f
#define RENEWAL_PER_SECOND 6.f
#define HALLOWED_RADIUS 1.3f
#define HALLOWED_HEAL 1.5f
#define INSPIRATION_SHARE 0.12f
#define MIRACLE_SECONDS 1.5f
#define MIRACLE_HP_SHARE 0.7f
#define PYROMANCER_SHARE 0.06f
#define SEARING_HEAT_PER_STACK 5.f
#define WILDFIRE_SECONDS 2.f
#define EXECUTIONER_BELOW 0.3f
#define EXECUTIONER_SHARE 0.35f
#define CATACLYSM_RADIUS 1.35f
#define OVERLOAD_SECONDS 3.f

// NOTE(zoubir): by player_role (Damage, Tank, Healer), then slot; the
// branch, tier, column and ranks match the slot's row in TalentDefs
global_variable talent_def RoleTalentDefs[PlayerRole_Count][ROLE_TALENTS] =
{
    {
        {"Pyromancer", "All your damage is higher", "+6% damage",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Searing Heat", "Each Searing stack detonates for more", "+5 a stack on Detonate",
         TalentBranch_Role, 0, 1, 2, 0},
        {"Wildfire", "Inferno's ground burns longer, keeping marks alive", "+2 s of burning ground",
         TalentBranch_Role, 1, 0, 1, 0},
        {"Executioner", "Hit harder on monsters under 30% health", "+35% damage on them",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Cataclysm", "Inferno covers a wider circle, marking more of a pack", "+35% Inferno radius",
         TalentBranch_Role, 2, 0, 1, 0},
        {"Overload", "Detonating a full mark gives back 3 s of Detonate",
         "-3 s Detonate on a full mark", TalentBranch_Role, 3, 0, 1, 0},
    },
    {
        {"Iron Skin", "You take less damage", "-6% damage taken",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Provoke", "Taunt comes back sooner and reaches farther",
         "-1.5 s Taunt cooldown, +15% reach", TalentBranch_Role, 0, 1, 2, 0},
        {"Bastion", "Shield Slam's wall holds longer, its stun lasts longer",
         "+2 s Shield Wall, +0.5 s stun", TalentBranch_Role, 1, 0, 1, 0},
        {"Guardian", "Intercept wards the ally you leap to", "a 30 damage ward",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Rally", "Shield Slam shields allies more, farther out",
         "allies take 40% less, out to 240", TalentBranch_Role, 2, 0, 1, 0},
        {"Shatter Armor", "Sunder bites deeper and lasts longer", "Sunder +10%, +2 s",
         TalentBranch_Role, 3, 0, 1, 0},
    },
    {
        {"Swift Mending", "Mending Bolt heals more", "+15% Mending Bolt healing",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Deep Ward", "Ward absorbs more damage", "+12 absorbed",
         TalentBranch_Role, 0, 1, 2, 0},
        {"Renewal", "Mending Bolt also heals 24 over 4 s", "healing over time",
         TalentBranch_Role, 1, 0, 1, 0},
        {"Hallowed Ground", "Sanctuary is wider and heals more", "+30% radius, +50% healing",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Inspiration", "Ward's damage bonus doubles and spreads to the allies near",
         "+24% damage while warded", TalentBranch_Role, 2, 0, 1, 0},
        {"Miracle", "Revive the fallen in 1.5 s, at 70% health", "faster, stronger revives",
         TalentBranch_Role, 3, 0, 1, 0},
    },
};

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
    return Result;
}
