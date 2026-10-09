/* Stormcaller's numbers, talents and spells, included by role_talents.cpp
   before the class tables (role_kits/stormcaller.cpp has what they do). A
   class that has no spell on its first key yet (RoleSpells) is left out
   of the role picker and the bots. */

// NOTE(zoubir): Charge, the Stormcaller's pressure gauge: spells fill it,
// it makes the lightning stronger from STORM_SUPERCHARGED on, and at
// STORM_CHARGE_MOST it overloads. Thunderclap is the vent
#define STORM_CHARGE_MOST 100.f
#define STORM_SUPERCHARGED 70.f
// NOTE(zoubir): after STORM_IDLE_SECONDS without dealing a hit, Charge
// drains this much a second
#define STORM_IDLE_SECONDS 4.f
#define STORM_CHARGE_DRAIN 10.f
// NOTE(zoubir): Supercharged, Spark and Chain Lightning jump to one more
// foe and hit this share harder
#define SUPERCHARGED_SHARE 0.2f
#define SUPERCHARGED_JUMPS 1

// NOTE(zoubir): the room a Stormcaller's lightning reaches is its own
// (or the room being fought from a doorway), and a bolt jumps to the
// nearest foe it has not struck within this of the last
#define STORM_JUMP_RADIUS 170.f
// NOTE(zoubir): a foe winding up on is still struck at the end of the
// wind-up within this of the Stormcaller, however it moved
#define STORM_HOLD_RANGE 640.f

// NOTE(zoubir): Spark (X): an instant bolt at the foe aimed at, the
// filler; it jumps SPARK_JUMPS times for SPARK_JUMP_SHARE of the one
// before
#define SPARK_RANGE 540.f
#define SPARK_DAMAGE 13.f
#define SPARK_JUMPS 1
#define SPARK_JUMP_SHARE 0.6f
#define SPARK_SHOVE 30.f
#define SPARK_CHARGE 7.f
#define SPARK_COOLDOWN 1.f

// NOTE(zoubir): Chain Lightning (A): after its cast (PlayerSpell_StormcallerA)
// a bolt at the foe it was pressed on, jumping CHAIN_JUMPS more times,
// each CHAIN_JUMP_SHARE of the one before and never on a foe twice;
// CHAIN_CHARGE for every foe struck
#define CHAIN_RANGE 540.f
#define CHAIN_DAMAGE 18.f
#define CHAIN_JUMPS 3
#define CHAIN_JUMP_SHARE 0.8f
#define CHAIN_SHOVE 50.f
#define CHAIN_CHARGE 4.f
#define CHAIN_COOLDOWN 6.f

// NOTE(zoubir): Static Field (R): a circle at the cursor for
// STATIC_FIELD_SECONDS; every STATIC_FIELD_TICK each foe inside takes
// STATIC_FIELD_TICK_DAMAGE and is slowed. A Spark or Chain Lightning hit
// on a foe inside arcs to every other foe inside for STATIC_ARC_SHARE
// of it, each foe arced to once a spell
#define STATIC_FIELD_RANGE 520.f
#define STATIC_FIELD_RADIUS 95.f
#define STATIC_FIELD_SECONDS 5.f
#define STATIC_FIELD_TICK 0.5f
#define STATIC_FIELD_TICK_DAMAGE 2.5f
#define STATIC_FIELD_SLOW_SECONDS 0.7f
#define STATIC_ARC_SHARE 0.4f
#define STATIC_FIELD_COOLDOWN 15.f

// NOTE(zoubir): Thunderclap (W): needs THUNDERCLAP_MIN_CHARGE; after its
// cast (PlayerSpell_StormcallerB) a bolt from the sky on the foe it was
// pressed on, THUNDERCLAP_DAMAGE plus THUNDERCLAP_PER_CHARGE for each
// point of Charge it spends (all of it). Spending STORM_SUPERCHARGED or
// more also stuns it and splashes the foes round it
#define THUNDERCLAP_RANGE 540.f
#define THUNDERCLAP_MIN_CHARGE 20.f
#define THUNDERCLAP_DAMAGE 14.f
#define THUNDERCLAP_PER_CHARGE 0.6f
#define THUNDERCLAP_SHOVE 60.f
#define THUNDERCLAP_STUN 1.f
#define THUNDERCLAP_SPLASH_RADIUS 80.f
#define THUNDERCLAP_SPLASH_SHARE 0.5f
#define THUNDERCLAP_COOLDOWN 8.f
// NOTE(zoubir): a Thunderclap refused for want of Charge says so this
// long (the HUD's shake), and not again before it is over
#define THUNDERCLAP_EMPTY_SECONDS 0.5f

// NOTE(zoubir): Overload: Charge reaching STORM_CHARGE_MOST sets off a
// nova round the Stormcaller, then Charge is 0, it loses
// OVERLOAD_HEALTH_SHARE of its health (never below 1) and every key it
// has waits at least OVERLOAD_GROUNDED seconds
#define OVERLOAD_RADIUS 130.f
#define OVERLOAD_DAMAGE 20.f
#define OVERLOAD_SHOVE 120.f
#define OVERLOAD_HEALTH_SHARE 0.1f
#define OVERLOAD_GROUNDED 2.f

// NOTE(zoubir): Lightning Dash (C, from the tree): a dash LIGHTNING_DASH_LENGTH
// along the aim, cut short by a wall, a pit, lava or a closed gate; the
// foes it passes within LIGHTNING_DASH_WIDTH take a hit and are slowed
#define LIGHTNING_DASH_LENGTH 280.f
#define LIGHTNING_DASH_STEP 8.f
#define LIGHTNING_DASH_WIDTH 30.f
#define LIGHTNING_DASH_DAMAGE 8.f
#define LIGHTNING_DASH_SLOW_SECONDS 1.5f
#define LIGHTNING_DASH_CHARGE 10.f
#define LIGHTNING_DASH_COOLDOWN 12.f
#define LIGHTNING_DASH_RANK2_COOLDOWN 9.f

// NOTE(zoubir): Eye of the Storm (V, from the tree): for EYE_SECONDS
// Charge holds at the top without overloading and counts as
// Supercharged, and a bolt strikes a random foe of the room every
// EYE_BOLT_SECONDS
#define EYE_SECONDS 8.f
#define EYE_BOLT_SECONDS 0.6f
#define EYE_BOLT_DAMAGE 6.f
#define EYE_COOLDOWN 50.f

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
    StormcallerTalent_ChainLightning,
    StormcallerTalent_StaticField,
};

// NOTE(zoubir): per rank, or once taken
#define VOLTAGE_SHARE 0.05f
// NOTE(zoubir): Conductor: Chain Lightning jumps this many more times and
// keeps this share a jump
#define CONDUCTOR_JUMPS 2
#define CONDUCTOR_JUMP_SHARE 0.9f
// NOTE(zoubir): Live Wire: an overload costs nothing and its nova is this
// much wider and harder
#define LIVE_WIRE_SCALE 1.5f
// NOTE(zoubir): Capacitor: a Thunderclap spending STORM_SUPERCHARGED or
// more gives this much Charge back and readies Chain Lightning
#define CAPACITOR_CHARGE 30.f
// NOTE(zoubir): Arc Field, per rank: Static Field this much wider and its
// ticks this much harder
#define ARC_FIELD_RADIUS_SHARE 0.1f
#define ARC_FIELD_DAMAGE_SHARE 0.25f
// NOTE(zoubir): Stormbringer: a Thunderclap spending STORM_SUPERCHARGED or
// more calls this many bolts on the nearest other foes within this, each
// this share of it
#define STORMBRINGER_BOLTS 3
#define STORMBRINGER_RADIUS 250.f
#define STORMBRINGER_SHARE 0.5f

// NOTE(zoubir): the same shape as every class's branch (role_talents.cpp);
// slot 1 unlocks the C spell, slot 4 the V spell
global_variable talent_def StormcallerTalentDefs[CLASS_TALENTS] =
{
    {"Voltage", "All your damage is higher", "+5% damage",
     TalentBranch_Role, 0, 0, 2, 0},
    {"Lightning Dash", "C: dash as lightning, hurting and slowing foes crossed; rank 2, back sooner",
     "a new spell", TalentBranch_Role, 0, 1, 2, 0},
    {"Conductor", "Chain Lightning jumps twice more and loses less a jump", "+2 jumps, 90% a jump",
     TalentBranch_Role, 1, 0, 1, 0},
    {"Live Wire", "Overloading costs nothing, and its nova is wider and harder", "safe overload",
     TalentBranch_Role, 1, 1, 1, 0},
    {"Eye of the Storm", "V: 8 s without overload, bolts striking foes round you",
     "a new spell", TalentBranch_Role, 2, 0, 1, 0},
    {"Capacitor", "A Thunderclap of 70+ Charge gives 30 back and readies Chain Lightning",
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
    {"Stormbringer", "A Thunderclap of 70+ Charge calls three more bolts on foes near",
     "more bolts", TalentBranch_Role, 5, 1, 1, 0},
    {"Chain Lightning", "A: a bolt that leaps from foe to foe",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
    {"Static Field", "R: a field at the cursor that hurts, slows, and arcs your bolts to every foe inside",
     "a spell", TalentBranch_Role, 0, 0, 1, 0},
};

// NOTE(zoubir): the stat each slot raises (role_stats.cpp), RoleStat_None
// for a talent with code of its own
global_variable u8 StormcallerTalentStats[CLASS_TALENTS] =
{
    RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None, RoleStat_None,
    RoleStat_Damage, RoleStat_Vitality, RoleStat_None, RoleStat_Haste, RoleStat_Swiftness, RoleStat_None,
};

// NOTE(zoubir): in RoleKeys order: A, R, C, V, W, X, right click, G, T
global_variable role_spell StormcallerSpells[ROLE_KEYS] =
{
    {"Chain Lightning", CHAIN_COOLDOWN,
     "Chain Lightning: 0.6 s cast, a bolt that leaps foe to foe, never the same twice; Charge a foe",
     RoleAim_Foe, CHAIN_RANGE, 0},
    {"Static Field", STATIC_FIELD_COOLDOWN,
     "Static Field: a circle at the cursor for 5 s that shocks and slows; lightning arcs across it",
     RoleAim_Ground, STATIC_FIELD_RADIUS, StormcallerTalent_StaticField + 1},
    {"Lightning Dash", LIGHTNING_DASH_COOLDOWN,
     "Lightning Dash: dash along the aim as lightning, shocking and slowing foes crossed",
     RoleAim_None, 0.f, StormcallerTalent_LightningDash + 1},
    {"Eye of the Storm", EYE_COOLDOWN,
     "Eye of the Storm: 8 s of bolts on foes round you; Charge holds at the top without overloading",
     RoleAim_None, 0.f, StormcallerTalent_EyeOfTheStorm + 1},
    {"Thunderclap", THUNDERCLAP_COOLDOWN,
     "Thunderclap: needs 20 Charge; 0.5 s cast, a bolt from the sky that spends all Charge; 70+ stuns",
     RoleAim_Foe, THUNDERCLAP_RANGE, 0, "All Charge, 20 at least"},
    {"Spark", SPARK_COOLDOWN,
     "Spark: an instant bolt at a foe that jumps to one more; builds Charge",
     RoleAim_Foe, SPARK_RANGE, 0},
    {},
};
