/* Role kit numbers (dungeon.cpp, for role_abilities.cpp): every role spell's reach, power
   and cooldown before talents, in one place to tune. The role branch of
   the talent tree changes them by rank (role_talents.cpp has those
   numbers). */

// NOTE(zoubir): between fights the party gets its breath back: this
// share of its health a second (RestBetweenFights)
#define REST_SHARE_PER_SECOND 0.08f

// NOTE(zoubir): Tank (role_kits/tank.cpp)
#define TAUNT_RADIUS 260.f
#define TAUNT_COOLDOWN 8.f
// NOTE(zoubir): a taunt raises Shield Wall for this long (never cutting
// a longer one short), so the tank meets what it pulled behind it
#define TAUNT_WALL_SECONDS 2.f
#define SHIELD_SLAM_RADIUS 110.f
#define SHIELD_SLAM_DAMAGE 15.f
#define SHIELD_SLAM_SHOVE 260.f
#define SHIELD_SLAM_STUN 1.f
// NOTE(zoubir): threat each monster struck takes on, on top of the hit's
#define SHIELD_SLAM_THREAT 80.f
#define SHIELD_SLAM_COOLDOWN 10.f
// NOTE(zoubir): every monster the slam strikes is sundered: it takes
// SUNDER_SHARE more from everyone for SUNDER_SECONDS, as long as the
// slam's cooldown, so a tank who slams on cooldown keeps the boss
// sundered for the party
#define SUNDER_SHARE 0.15f
#define SUNDER_SECONDS 10.f
// NOTE(zoubir): the tank's fireball and kunai keep a sunder going: this
// much longer on a sundered monster, or a sunder this long on a clean one
#define SUNDERING_SHOT_SECONDS 3.f
#define SUNDERING_SHOT_FRESH 4.f
// NOTE(zoubir): the tank heals this share of its health for each monster
// the slam strikes, counting at most SHIELD_SLAM_HEAL_FOES; the bigger
// the pack it holds, the more it gets back
#define SHIELD_SLAM_HEAL_SHARE 0.05f
#define SHIELD_SLAM_HEAL_FOES 5
#define SHIELD_WALL_SECONDS 4.f
// NOTE(zoubir): allies this close to the slam take RALLY_SHARE less for
// RALLY_SECONDS (the tank itself has Shield Wall instead)
#define RALLY_RADIUS 170.f
#define RALLY_SHARE 0.25f
#define RALLY_SECONDS 4.f
#define INTERCEPT_RANGE 420.f
#define INTERCEPT_COOLDOWN 10.f
// NOTE(zoubir): how far short of the ally an intercept lands
#define INTERCEPT_LANDING_GAP 36.f
// NOTE(zoubir): Last Stand (V, from the tree): the tank heals this share
// of its health and stands behind Shield Wall this long
#define LAST_STAND_HEAL_SHARE 0.3f
#define LAST_STAND_SECONDS 6.f
#define LAST_STAND_COOLDOWN 40.f
// NOTE(zoubir): Shield Throw (W): the shield hits the foe the tank aims
// at, then bounces to the nearest foe within SHIELD_THROW_BOUNCE_RADIUS
// of the last one, up to SHIELD_THROW_BOUNCES times, each bounce
// SHIELD_THROW_BOUNCE_SHARE of the hit before it
#define SHIELD_THROW_RANGE 480.f
#define SHIELD_THROW_DAMAGE 34.f
#define SHIELD_THROW_SHOVE 120.f
#define SHIELD_THROW_THREAT 40.f
#define SHIELD_THROW_BOUNCES 2
#define SHIELD_THROW_BOUNCE_RADIUS 200.f
#define SHIELD_THROW_BOUNCE_SHARE 0.75f
#define SHIELD_THROW_COOLDOWN 6.f
// NOTE(zoubir): Shield Charge (X): the tank rushes to the foe it aims at
// within SHIELD_CHARGE_RANGE, landing SHIELD_CHARGE_LANDING_GAP short of
// it, and stuns it for SHIELD_CHARGE_STUN. A foe winding up an attack
// loses it: the wind-up ends and it recovers as if the attack had gone
// off, so the tank's job in a boss fight is to stop the big ones
#define SHIELD_CHARGE_RANGE 400.f
#define SHIELD_CHARGE_LANDING_GAP 40.f
#define SHIELD_CHARGE_DAMAGE 20.f
#define SHIELD_CHARGE_SHOVE 90.f
#define SHIELD_CHARGE_STUN 2.f
#define SHIELD_CHARGE_THREAT 60.f
#define SHIELD_CHARGE_COOLDOWN 12.f

// NOTE(zoubir): Shield Bash (right click): the shield driven into what
// is in front, every SHIELD_BASH_COOLDOWN: SHIELD_BASH_DAMAGE before the
// tank's 70% to each foe within SHIELD_BASH_REACH and SHIELD_BASH_HALF_ANGLE
// of the aim, with threat on top. A little damage, so a tank is never
// idle, never a damage role
#define SHIELD_BASH_REACH 72.f
#define SHIELD_BASH_HALF_ANGLE 0.9f
#define SHIELD_BASH_DAMAGE 5.f
#define SHIELD_BASH_SHOVE 70.f
#define SHIELD_BASH_THREAT 12.f
#define SHIELD_BASH_COOLDOWN 0.9f
// NOTE(zoubir): the low two bits of a tank's ClassFlags count its bashes,
// so every client sees one land and punches the shield forward
// (client/dungeon/role_looks.cpp)
#define TANK_FLAG_BASH_COUNT 0x3

// NOTE(zoubir): Healer (role_kits/healer.cpp)
#define SANCTUARY_RADIUS 110.f
#define SANCTUARY_SECONDS 5.f
#define SANCTUARY_HEAL_PER_SECOND 8.f
#define SANCTUARY_COOLDOWN 14.f
#define WARD_ABSORB 36.f
#define WARD_COOLDOWN 10.f
// NOTE(zoubir): the allies this close to the warded one absorb this
// share of the ward too
#define WARD_SPLASH_RADIUS 150.f
#define WARD_SPLASH_SHARE 0.5f
#define MENDING_BOLT_HEAL 34.f
#define MENDING_BOLT_RANGE 500.f
#define MENDING_BOLT_COOLDOWN 2.2f
#define HEAL_THREAT_SHARE 0.5f
// NOTE(zoubir): a healer's fireball heals the most hurt ally this share of
// the damage it dealt (Smite)
#define SMITE_SHARE 1.5f
// NOTE(zoubir): Radiance (V, from the tree): every living player this
// close to the healer heals this much and holds a ward at least this big
#define RADIANCE_RADIUS 260.f
#define RADIANCE_HEAL 30.f
#define RADIANCE_WARD 15.f
#define RADIANCE_COOLDOWN 18.f
// NOTE(zoubir): Holy Fire (W): a bolt of light at the foe the healer
// aims at; the damage it deals heals the most hurt ally like a fireball's
// (Smite)
#define HOLY_FIRE_RANGE 500.f
#define HOLY_FIRE_DAMAGE 48.f
#define HOLY_FIRE_SHOVE 60.f
#define HOLY_FIRE_COOLDOWN 4.f

// NOTE(zoubir): Smite Bolt (right click): a quick bolt of light at a foe,
// SMITE_BOLT_DAMAGE before the healer's 50%, healing the most hurt ally
// for it like Holy Fire (Smite); the healer's filler between heals
#define SMITE_BOLT_RANGE 460.f
#define SMITE_BOLT_DAMAGE 7.f
#define SMITE_BOLT_SHOVE 20.f
#define SMITE_BOLT_COOLDOWN 1.f

// NOTE(zoubir): an attack spell (Shield Throw, Holy Fire) aimed at no
// foe goes for the one nearest the cursor within this of it
#define ATTACK_PICK_RADIUS 140.f

// NOTE(zoubir): Damage (role_kits/striker.cpp): Meteor (A) winds up for
// its cast (PlayerSpell_Meteor, sim/player_casts.cpp), falls
// INFERNO_DELAY after it, then the ground burns
#define INFERNO_RADIUS 90.f
#define INFERNO_DELAY 0.4f
#define INFERNO_DAMAGE 32.f
#define INFERNO_BURN_SECONDS 3.f
#define INFERNO_BURN_PER_SECOND 10.f
#define INFERNO_BURN_TICK 0.5f
#define INFERNO_COOLDOWN 9.f
// NOTE(zoubir): Searing marks: each kunai, fireball or Inferno blast a
// striker lands adds a stack, up to SEARING_MOST, and the mark fades
// SEARING_SECONDS after its last stack; burning ground keeps it alive
#define SEARING_MOST 3
#define SEARING_SECONDS 6.f
// NOTE(zoubir): Detonate (E) blows up the marks on the monster under the
// cursor (or the marked one nearest the cursor within
// DETONATE_PICK_RADIUS of it): DETONATE_DAMAGE plus DETONATE_PER_STACK a
// stack, so a full mark hits for 72 before the role's 35%. A monster
// standing in burning ground takes every other marked monster in that
// fire with it
#define DETONATE_RANGE 560.f
#define DETONATE_PICK_RADIUS 140.f
#define DETONATE_DAMAGE 12.f
#define DETONATE_PER_STACK 20.f
#define DETONATE_COOLDOWN 7.f
// NOTE(zoubir): Giant Fireball (R) winds up for its cast
// (PlayerSpell_GiantFireball), then flies slowly along the aim and blows
// up on the first monster it reaches, on leaving its room or at the end
// of its flight, hitting and marking everything in the blast
#define GIANT_FIREBALL_SPEED 260.f
#define GIANT_FIREBALL_RANGE 700.f
#define GIANT_FIREBALL_TOUCH 34.f
#define GIANT_FIREBALL_RADIUS 110.f
#define GIANT_FIREBALL_DAMAGE 55.f
#define GIANT_FIREBALL_COOLDOWN 8.f
// NOTE(zoubir): Combustion (V, from the tree): this much more damage for
// this long
#define COMBUSTION_SHARE 0.4f
#define COMBUSTION_SECONDS 6.f
#define COMBUSTION_COOLDOWN 30.f
