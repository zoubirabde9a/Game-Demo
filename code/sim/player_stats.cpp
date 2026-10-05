/* Player stats: the numbers that decide how the player's body moves, how
   much it takes and what its basic attacks do, in one table, so tuning
   the feel happens here. The abilities keep their own tables
   (player_abilities/), but the dash, blink, sword and fireball rows read
   their speed, reach, damage, shove and timing from here; the sword's
   cuts (sword.cpp SwordCuts) are multiples of a plain cut.

   Walking (player_update/movement.cpp MovePlayer): the velocity goes
   straight toward the keys' direction at RunSpeed, at a fixed rate:
   RunSpeed / RunUpSeconds while speeding up, RunSpeed / StopSeconds while
   slowing down, stopping or turning. Faster than a run (a dash, a long
   jump, a hard knockback) the speed falls off by CarryDrag a second
   instead, so those carry as far as they always did. Ground friction
   (ice) scales both rates; slows and mud scale the top speed.

   At 60 ticks a second (player_feel_tests.cpp prints them): full speed in
   6 ticks, stopped in 5 ticks after a 9 unit skid, a full turn back in 10.
   It used to be 17 ticks to full speed, 22 to stop (23 units) and 21 to
   turn back. */

struct player_stats
{
    float MaxHp;
    // NOTE(zoubir): top walking speed, and the seconds from standing to
    // it and from it to standing
    float RunSpeed;
    float RunUpSeconds;
    float StopSeconds;
    // NOTE(zoubir): share of its speed a body faster than a run loses per
    // second (a dash eases back to a run in about a quarter second)
    float CarryDrag;
    // NOTE(zoubir): letting go of the keys faster than this raises a skid
    float SkidSpeed;
    // NOTE(zoubir): share of the run speed while a swing or cast finishes
    float ActionWalkScale;
    // NOTE(zoubir): seconds a shove staggers the player (ApplyHit): the
    // keys do not walk and only CarryDrag slows it, so a shove of 150
    // slides 12 units instead of 3 (15 with drag alone). Longer gains
    // little: the drag has taken most of the speed by then
    float StaggerSeconds;
    float DashSpeed;
    float DashCooldown;
    float BlinkReach;
    float BlinkCooldown;
    // NOTE(zoubir): seconds before a movement or area ability is ready
    // that its key already works; the time it was early is added to the
    // next cooldown, so the pace stays the same and a press a moment early
    // is not lost (CanUseEarly)
    float EarlyPressSeconds;
    // NOTE(zoubir): a plain sword cut's damage and shove (speed given to
    // the target); seconds the player is rooted from the start of a swing
    // or cast, and before the next may start
    float SwordDamage;
    float SwordShove;
    float SwordLock;
    float SwordInterval;
    float FireballDamage;
    float FireballSpeed;
    float FireballRange;
    float FireballLock;
    float FireballInterval;
};

global_variable player_stats PlayerStats =
{
    100.f,                          // MaxHp
    130.f * PLAYER_MOVE_SCALE,      // RunSpeed
    0.1f,                           // RunUpSeconds
    0.07f,                          // StopSeconds
    10.f,                           // CarryDrag
    70.f * PLAYER_MOVE_SCALE,       // SkidSpeed
    0.7f,                           // ActionWalkScale
    0.2f,                           // StaggerSeconds
    720.f * PLAYER_MOVE_SCALE,      // DashSpeed
    0.8f,                           // DashCooldown
    PLAYER_AIM_REACH,               // BlinkReach
    3.f,                            // BlinkCooldown
    0.1f,                           // EarlyPressSeconds
    25.f,                           // SwordDamage
    280.f,                          // SwordShove
    0.08f,                          // SwordLock
    0.18f,                          // SwordInterval
    25.f,                           // FireballDamage
    650.f,                          // FireballSpeed
    420.f,                          // FireballRange
    0.05f,                          // FireballLock
    0.35f,                          // FireballInterval
};

// NOTE(zoubir): a shove's stagger (ApplyHit, hit.cpp); a second shove
// restarts it, never shortens it
internal void
StaggerPlayer(world_entity *Player)
{
    Player->Stagger = Maximum(Player->Stagger, PlayerStats.StaggerSeconds);
}

// NOTE(zoubir): whether a press may use an ability with Cooldown seconds
// still to run (counted down this tick): ready, or nearly. The user adds
// the full cooldown on top of what was left, so nothing waits in between
// and client prediction has nothing new to replay
inline bool32
CanUseEarly(float Cooldown)
{
    bool32 Result = Cooldown <= PlayerStats.EarlyPressSeconds;
    return Result;
}
