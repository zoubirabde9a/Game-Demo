/* Striker (the fire mage, damage)'s talents, included by role_talents.cpp before the class
   tables (role_kits/striker.cpp has what they do). */

enum striker_talent
{
    StrikerTalent_Pyromancer,
    StrikerTalent_SearingHeat,
    StrikerTalent_Wildfire,
    StrikerTalent_Executioner,
    StrikerTalent_Combustion,
    StrikerTalent_Overload,
    StrikerTalent_Kindling,
    StrikerTalent_EmberMantle,
    StrikerTalent_MoltenGround,
    StrikerTalent_QuickenedFlame,
    StrikerTalent_HeatShield,
    StrikerTalent_Cataclysm,
    StrikerTalent_Meteor,
    StrikerTalent_Fireguard,
    StrikerTalent_Detonate,
    StrikerTalent_FlameWave,
};

// NOTE(zoubir): per rank, or once taken
#define PYROMANCER_SHARE 0.06f
// NOTE(zoubir): Searing Heat, per rank: this much more burn a second
// and this much more explosion, a Searing stack
#define SEARING_HEAT_BURN 1.f
#define SEARING_HEAT_PER_STACK 5.f
#define WILDFIRE_SECONDS 2.f
#define EXECUTIONER_BELOW 0.3f
#define EXECUTIONER_SHARE 0.35f
// NOTE(zoubir): Overload: a full mark explodes this share harder
#define OVERLOAD_SHARE 0.5f
// NOTE(zoubir): Molten Ground: seconds of Slowed a burn of Meteor's
// ground leaves, per rank; it burns every INFERNO_BURN_TICK, so any rank
// slows a monster for as long as it stands in the fire, and more ranks
// keep it slow after it walks out
#define MOLTEN_GROUND_SLOW_SECONDS 1.f
// NOTE(zoubir): Cataclysm: a Giant Fireball's blast stuns everything it
// catches this long, which also holds a monster's wind-up
#define CATACLYSM_STUN_SECONDS 1.5f

global_variable talent_def StrikerTalentDefs[CLASS_TALENTS] =
{
        {"Pyromancer", "All your damage is higher", "+6% damage",
         TalentBranch_Role, 0, 0, 2, 0},
        {"Searing Heat", "Searing burns hotter and explodes harder",
         "+1 burn and +5 blast a stack", TalentBranch_Role, 0, 1, 2, 0},
        {"Wildfire", "Meteor's ground burns longer, keeping marks alive", "+2 s of burning ground",
         TalentBranch_Role, 1, 0, 1, 0},
        {"Executioner", "Hit harder on monsters under 30% health", "+35% damage on them",
         TalentBranch_Role, 1, 1, 1, 0},
        {"Combustion", "V: for 6 s every hit you land is 40% harder", "a new spell",
         TalentBranch_Role, 2, 0, 1, 0},
        {"Overload", "A full Searing mark explodes harder",
         "+50% blast on a full mark", TalentBranch_Role, 3, 0, 1, 0},
        {"Kindling", "Your fire burns hotter", "+4% damage",
         TalentBranch_Role, 2, 1, 4, 0},
        {"Ember Mantle", "More health", "+6% health",
         TalentBranch_Role, 3, 1, 4, 0},
        {"Molten Ground", "Meteor's burning ground slows the monsters in it",
         "+1 s of slow after the fire", TalentBranch_Role, 4, 0, 4, 0},
        {"Quickened Flame", "Every spell comes back sooner", "-4% cooldowns",
         TalentBranch_Role, 4, 1, 4, 0},
        {"Heat Shield", "You take less damage", "-4% damage taken",
         TalentBranch_Role, 5, 0, 4, 0},
        {"Cataclysm", "Giant Fireball's blast stuns every monster it catches",
         "1.5 s stun", TalentBranch_Role, 5, 1, 1, 0},
    {"Meteor", "A: a 1 s cast, then a meteor at the cursor that marks and leaves burning ground",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Fireguard", "C: a shield of fire that takes the next 50 damage",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Detonate", "W: every Searing mark you laid near you blows at once, 30% harder",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Flame Wave", "Right click: a cone of fire in front that leaves two Searing on each foe",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 StrikerTalentStats[CLASS_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Vitality, RoleStat_None, RoleStat_Haste, RoleStat_Armor, RoleStat_None,
};
