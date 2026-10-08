/* Stormcaller's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/stormcaller.cpp has what they do). A
   class that has no spell on its first key yet (RoleSpells) is left out
   of the role picker and the bots, so until the kit lands the spell
   table is empty and the talents below are placeholders in the branch's
   shape. */

enum stormcaller_talent
{
    StormcallerTalent_Voltage,
    StormcallerTalent_LightningDash,
    StormcallerTalent_Conductor,
    StormcallerTalent_LiveWire,
    StormcallerTalent_EyeOfTheStorm,
    StormcallerTalent_Capacitor,
    StormcallerTalent_HighVoltage,
    StormcallerTalent_Grounding,
    StormcallerTalent_ArcField,
    StormcallerTalent_Quickening,
    StormcallerTalent_Tailwind,
    StormcallerTalent_Stormbringer,
};

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp):
// slot 1 will unlock the C spell, slot 4 the V spell
global_variable talent_def StormcallerTalentDefs[ROLE_TALENTS] =
{
    {"Voltage", "All your damage is higher", "+5% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Lightning Dash", "C: dash as lightning, hurting and slowing foes crossed",
     "a new spell", TalentBranch_Role, 0, 1, 2, 0},
    {"Conductor", "Chain Lightning jumps farther and loses less a jump", "+2 jumps",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Live Wire", "Overloading costs nothing and its nova is bigger", "safe overload",
     TalentBranch_Role, 1, 1, 1, 0},
    {"Eye of the Storm", "V: 8 s without overload, bolts striking foes round you",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Capacitor", "A big Thunderclap gives back Charge and readies Chain Lightning",
     "Charge back", TalentBranch_Role, 3, 0, 1, 0},
    {"High Voltage", "Your lightning hits harder", "+4% damage",
     TalentBranch_Role, 2, 1, 4, 0},
    {"Grounding", "You have more health", "+6% health",
     TalentBranch_Role, 3, 1, 4, 0},
    {"Arc Field", "Static Field grows and bites harder", "+10% radius, +25% damage",
     TalentBranch_Role, 4, 0, 4, 0},
    {"Quickening", "Every spell comes back sooner", "-4% cooldowns",
     TalentBranch_Role, 4, 1, 4, 0},
    {"Tailwind", "You run faster", "+3% run speed",
     TalentBranch_Role, 5, 0, 4, 0},
    {"Stormbringer", "A big Thunderclap calls three more bolts on foes near",
     "more bolts", TalentBranch_Role, 5, 1, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 StormcallerTalentStats[ROLE_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Vitality, RoleStat_None, RoleStat_Haste, RoleStat_Swiftness, RoleStat_None,
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click
global_variable role_spell StormcallerSpells[ROLE_KEYS] =
{
    {}, {}, {}, {}, {}, {}, {},
};
