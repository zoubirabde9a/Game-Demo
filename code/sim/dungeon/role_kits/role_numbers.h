/* Role kit numbers (role_abilities.cpp): every role spell's reach, power
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
#define SHIELD_SLAM_COOLDOWN 14.f
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

// NOTE(zoubir): Damage (role_kits/striker.cpp): the meteor falls
// INFERNO_DELAY after the cast, then the ground burns
#define INFERNO_RADIUS 90.f
#define INFERNO_DELAY 0.6f
#define INFERNO_DAMAGE 32.f
#define INFERNO_BURN_SECONDS 3.f
#define INFERNO_BURN_PER_SECOND 10.f
#define INFERNO_BURN_TICK 0.5f
#define INFERNO_COOLDOWN 9.f
