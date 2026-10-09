/* Spell numbers (ability_tooltip.cpp): what each dungeon class spell does
   in numbers, read from the same tuning constants the simulation uses
   (sim/dungeon/role_kits/), so a retuned spell shows its new numbers on
   the ability bar's card. One row per spell, found by its name. Amounts
   are before the class's damage share and its talents; the card applies
   those. A spell that grows with a resource (Rage, combo points, Icicles,
   Charge, Focus, Bloom) shows the hit it starts from. */

enum spell_number_kind
{
    SpellNumber_None,
    // NOTE(zoubir): Amount a hit; Hits > 1 for several hits; Seconds > 0
    // for damage spread over that long (Amount is the total)
    SpellNumber_Damage,
    SpellNumber_Heal,     // as Damage, for healing
    SpellNumber_Shield,   // Amount absorbed
    SpellNumber_Stun,     // Seconds
    SpellNumber_Freeze,   // Seconds
    SpellNumber_Root,     // Seconds (held in place)
    SpellNumber_Slow,     // Seconds; Amount the share slowed if known (0.3 = 30%), else 0
    SpellNumber_DamageUp, // Amount a share (0.4 = 40% more damage dealt) for Seconds
    SpellNumber_DamageCut,// Amount a share (0.9 = 90% less damage taken) for Seconds
    SpellNumber_Lasts,    // Seconds it lasts (a field, a channel), when no other part says it
};

struct spell_number
{
    u8 Kind;
    float Amount;
    float Seconds;
    u8 Hits;
};

#define SPELL_NUMBER_PARTS 3

struct spell_numbers
{
    char *Spell; // the role_spell's Name
    spell_number Part[SPELL_NUMBER_PARTS];
};

// NOTE(zoubir): Slowed scales a foe's walk by STATUS_SLOW_SCALE
// (sim/status_effects.cpp), whichever spell slowed it
#define SPELL_SLOW_SHARE (1.f - STATUS_SLOW_SCALE)

global_variable spell_numbers SpellNumberTable[] =
{
    // NOTE(zoubir): Fire Mage (role_kits/role_numbers.h)
    {"Meteor", {{SpellNumber_Damage, INFERNO_DAMAGE},
                {SpellNumber_Damage, INFERNO_BURN_PER_SECOND * INFERNO_BURN_SECONDS,
                 INFERNO_BURN_SECONDS}}},
    {"Giant Fireball", {{SpellNumber_Damage, GIANT_FIREBALL_DAMAGE}}},
    {"Fireguard", {{SpellNumber_Shield, FIREGUARD_ABSORB},
                   {SpellNumber_Lasts, 0.f, FIREGUARD_SECONDS}}},
    {"Combustion", {{SpellNumber_DamageUp, COMBUSTION_SHARE, COMBUSTION_SECONDS}}},

    // NOTE(zoubir): Bulwark (role_kits/role_numbers.h, Shield Wall in sim/dungeon/dungeon.cpp)
    {"Taunt", {{SpellNumber_DamageCut, 1.f - SHIELD_WALL_SCALE, TAUNT_WALL_SECONDS}}},
    {"Shield Slam", {{SpellNumber_Damage, SHIELD_SLAM_DAMAGE},
                     {SpellNumber_Stun, 0.f, SHIELD_SLAM_STUN},
                     {SpellNumber_DamageCut, 1.f - SHIELD_WALL_SCALE, SHIELD_WALL_SECONDS}}},
    // NOTE(zoubir): Intercept moves the tank and pulls threat; its only
    // number (GUARDIAN_WARD) comes with the talent's second rank
    {"Intercept"},
    // NOTE(zoubir): Last Stand also heals LAST_STAND_HEAL_SHARE of the
    // tank's health, a share no part here can say
    {"Last Stand", {{SpellNumber_DamageCut, 1.f - SHIELD_WALL_SCALE, LAST_STAND_SECONDS}}},
    {"Shield Throw", {{SpellNumber_Damage, SHIELD_THROW_DAMAGE}}},
    {"Shield Charge", {{SpellNumber_Damage, SHIELD_CHARGE_DAMAGE},
                       {SpellNumber_Stun, 0.f, SHIELD_CHARGE_STUN}}},
    {"Shield Bash", {{SpellNumber_Damage, SHIELD_BASH_DAMAGE}}},

    // NOTE(zoubir): Mender (role_kits/role_numbers.h, the ward's bonus in dungeon.cpp)
    {"Mending Bolt", {{SpellNumber_Heal, MENDING_BOLT_HEAL}}},
    {"Ward", {{SpellNumber_Shield, WARD_ABSORB},
              {SpellNumber_DamageUp, WARD_EMPOWER_SHARE}}},
    {"Sanctuary", {{SpellNumber_Heal, SANCTUARY_HEAL_PER_SECOND * SANCTUARY_SECONDS,
                    SANCTUARY_SECONDS}}},
    {"Radiance", {{SpellNumber_Heal, RADIANCE_HEAL},
                  {SpellNumber_Shield, RADIANCE_WARD}}},
    {"Holy Fire", {{SpellNumber_Damage, HOLY_FIRE_DAMAGE}}},
    {"Smite Bolt", {{SpellNumber_Damage, SMITE_BOLT_DAMAGE}}},

    // NOTE(zoubir): Berserker (role_kits/berserker_defs.cpp)
    {"Leap", {{SpellNumber_Damage, LEAP_DAMAGE}, {SpellNumber_Stun, 0.f, LEAP_STUN}}},
    {"Whirlwind", {{SpellNumber_Damage, WHIRLWIND_DAMAGE, 0.f, WHIRLWIND_HITS}}},
    {"Berserk", {{SpellNumber_DamageUp, BERSERK_DEALT_SHARE, BERSERK_SECONDS},
                 {SpellNumber_DamageCut, BERSERK_TAKEN_SHARE, BERSERK_SECONDS}}},
    // NOTE(zoubir): the smallest real chop, on the least Rage it goes with
    {"Execute", {{SpellNumber_Damage, EXECUTE_DAMAGE + EXECUTE_PER_RAGE * EXECUTE_MIN_RAGE}}},
    {"Cleave", {{SpellNumber_Damage, CLEAVE_DAMAGE}}},

    // NOTE(zoubir): Druid (role_kits/druid_defs.cpp)
    {"Rejuvenation", {{SpellNumber_Heal, REJUVENATION_PER_SECOND * REJUVENATION_SECONDS,
                       REJUVENATION_SECONDS}}},
    {"Starfire", {{SpellNumber_Damage, STARFIRE_DAMAGE}}},
    {"Entangling Roots", {{SpellNumber_Damage, ROOTS_TICK_DAMAGE * ROOTS_SECONDS, ROOTS_SECONDS},
                          {SpellNumber_Root, 0.f, ROOTS_SECONDS}}},
    // NOTE(zoubir): no define for this, from role_kits/druid.h: the
    // channel's 3 s is in DRUID_CAST_B
    {"Tranquility", {{SpellNumber_Heal, TRANQUILITY_HEAL * (3.f / TRANQUILITY_TICK), 3.f}}},
    {"Regrowth", {{SpellNumber_Heal, REGROWTH_HEAL}}},
    {"Moonfire", {{SpellNumber_Damage, MOONFIRE_DAMAGE},
                  {SpellNumber_Damage, MOONFIRE_TICK_DAMAGE * MOONFIRE_SECONDS,
                   MOONFIRE_SECONDS}}},
    {"Wrath", {{SpellNumber_Damage, WRATH_DAMAGE}}},

    // NOTE(zoubir): Duelist (role_kits/duelist_defs.cpp)
    {"Lunge", {{SpellNumber_Damage, LUNGE_DAMAGE}}},
    {"Riposte", {{SpellNumber_Damage, COUNTER_DAMAGE},
                 {SpellNumber_Stun, 0.f, COUNTER_STUN},
                 {SpellNumber_Lasts, 0.f, RIPOSTE_GUARD_SECONDS}}},
    {"Perfect Form", {{SpellNumber_Lasts, 0.f, FORM_SECONDS}}},
    {"Heartseeker", {{SpellNumber_Damage, HEARTSEEKER_DAMAGE}}},
    {"Thrust", {{SpellNumber_Damage, THRUST_DAMAGE}}},

    // NOTE(zoubir): Frost Mage (role_kits/frostmage_defs.cpp)
    {"Blizzard", {{SpellNumber_Damage, BLIZZARD_TICK_DAMAGE * (BLIZZARD_SECONDS / BLIZZARD_TICK),
                   BLIZZARD_SECONDS},
                  {SpellNumber_Slow, SPELL_SLOW_SHARE, BLIZZARD_CHILL_SECONDS}}},
    {"Glacial Spike", {{SpellNumber_Damage, GLACIAL_SPIKE_DAMAGE},
                       {SpellNumber_Freeze, 0.f, GLACIAL_SPIKE_FREEZE}}},
    {"Ice Barrier", {{SpellNumber_Shield, ICE_BARRIER_ABSORB},
                     {SpellNumber_Lasts, 0.f, ICE_BARRIER_SECONDS}}},
    {"Frozen Orb", {{SpellNumber_Damage, FROZEN_ORB_DAMAGE, 0.f,
                     (u8)(FROZEN_ORB_SECONDS / FROZEN_ORB_TICK)},
                    {SpellNumber_Slow, SPELL_SLOW_SHARE, FROSTBOLT_CHILL_SECONDS},
                    {SpellNumber_Lasts, 0.f, FROZEN_ORB_SECONDS}}},
    {"Frost Nova", {{SpellNumber_Damage, FROST_NOVA_DAMAGE},
                    {SpellNumber_Root, 0.f, FROST_NOVA_ROOT}}},
    {"Frostbolt", {{SpellNumber_Damage, FROSTBOLT_DAMAGE},
                   {SpellNumber_Slow, SPELL_SLOW_SHARE, FROSTBOLT_CHILL_SECONDS}}},

    // NOTE(zoubir): Ranger (role_kits/ranger_defs.cpp)
    {"Volley", {{SpellNumber_Damage, VOLLEY_TICK_DAMAGE * (VOLLEY_SECONDS / VOLLEY_TICK),
                 VOLLEY_SECONDS},
                {SpellNumber_Slow, SPELL_SLOW_SHARE, VOLLEY_SLOW_SECONDS}}},
    {"Piercing Shot", {{SpellNumber_Damage, PIERCE_DAMAGE}}},
    {"Disengage", {{SpellNumber_Damage, TRAP_DAMAGE},
                   {SpellNumber_Root, 0.f, TRAP_ROOT_SECONDS}}},
    {"Rapid Fire", {{SpellNumber_Damage, RAPID_FIRE_DAMAGE, 0.f, RAPID_FIRE_ARROWS}}},
    {"Quick Shot", {{SpellNumber_Damage, QUICK_SHOT_DAMAGE},
                    {SpellNumber_DamageUp, MARK_SHARE, MARK_SECONDS}}},

    // NOTE(zoubir): Shadowblade (role_kits/shadowblade_defs.cpp, the guard in shadowblade.h)
    {"Shadowstep", {{SpellNumber_DamageCut, 1.f - SHADOWSTEP_GUARD_SCALE,
                     SHADOWSTEP_GUARD_SECONDS}}},
    {"Fan of Knives", {{SpellNumber_Damage, FAN_OF_KNIVES_DAMAGE}}},
    {"Shadow Dance", {{SpellNumber_Lasts, 0.f, DANCE_SECONDS}}},
    // NOTE(zoubir): the smallest real hit: it spends one combo point at least
    {"Eviscerate", {{SpellNumber_Damage, EVISCERATE_DAMAGE + EVISCERATE_PER_POINT}}},
    // NOTE(zoubir): the slow is DEADLY_THROW_SLOW_PER_POINT a combo point
    // spent; one point at least
    {"Deadly Throw", {{SpellNumber_Damage, DEADLY_THROW_DAMAGE},
                      {SpellNumber_Damage, BLADE_POISON_PER_SECOND * BLADE_POISON_SECONDS,
                       BLADE_POISON_SECONDS},
                      {SpellNumber_Slow, SPELL_SLOW_SHARE, DEADLY_THROW_SLOW_PER_POINT}}},
    // NOTE(zoubir): no define for this, from role_kits/shadowblade_defs.cpp:
    // Twin Strike's two cuts
    {"Twin Strike", {{SpellNumber_Damage, TWIN_STRIKE_DAMAGE, 0.f, 2},
                     {SpellNumber_Damage, BLADE_POISON_PER_SECOND * BLADE_POISON_SECONDS,
                      BLADE_POISON_SECONDS}}},

    // NOTE(zoubir): Stormcaller (role_kits/stormcaller_defs.cpp)
    {"Chain Lightning", {{SpellNumber_Damage, CHAIN_DAMAGE}}},
    {"Static Field", {{SpellNumber_Damage,
                       STATIC_FIELD_TICK_DAMAGE * (STATIC_FIELD_SECONDS / STATIC_FIELD_TICK),
                       STATIC_FIELD_SECONDS},
                      {SpellNumber_Slow, SPELL_SLOW_SHARE, STATIC_FIELD_SLOW_SECONDS}}},
    {"Lightning Dash", {{SpellNumber_Damage, LIGHTNING_DASH_DAMAGE},
                        {SpellNumber_Slow, SPELL_SLOW_SHARE, LIGHTNING_DASH_SLOW_SECONDS}}},
    {"Eye of the Storm", {{SpellNumber_Damage, EYE_BOLT_DAMAGE, 0.f,
                           (u8)(EYE_SECONDS / EYE_BOLT_SECONDS)},
                          {SpellNumber_Lasts, 0.f, EYE_SECONDS}}},
    // NOTE(zoubir): the stun comes only with STORM_SUPERCHARGED Charge spent
    {"Thunderclap", {{SpellNumber_Damage, THUNDERCLAP_DAMAGE},
                     {SpellNumber_Stun, 0.f, THUNDERCLAP_STUN}}},
    {"Spark", {{SpellNumber_Damage, SPARK_DAMAGE}}},
};
