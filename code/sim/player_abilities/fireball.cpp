/* Fireball (left click): a piercing shot along the aim at any angle,
   hitting each target once (HandleCollision, collision.cpp, calls
   FireBallHit, which lands FireBallHitRow through ApplyHit). Its numbers are in player_stats.cpp and its timing row
   in spawn_actions.cpp: a click sooner than the interval waits in the
   action queue and fires when it may, so fast clicking gives a steady
   stream rather than a spray. */

#define FIREBALL_HAND_HEIGHT 30.f

internal void
SpawnFireBall(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Player, v2 Dir, player_tick *Tick)
{
    v2 Start = Player->Position.XY + 32.f * Dir;
    v2 Velocity = PlayerStats.FireballSpeed * Dir;
    // NOTE(zoubir): hand height above the raised ground the player is on,
    // not above its jump, so a shot from a jump still hits who is below
    float Height = GroundHeightAt(World, Player->Position.XY) + FIREBALL_HAND_HEIGHT;
    world_entity *FireBall =
        AddFireBall(AppState, World, Arena, Player,
                    V3(Start.X, Start.Y, Height),
                    V3(Velocity.X, Velocity.Y, 0.f));
    FireBall->AnimationSpeed = 1.f;
    FireBall->AnimationType = AnimationType_Move;
    FireBall->AnimationDirection = DominantFacing(Dir);
}

// NOTE(zoubir): what a fireball does to each target it passes through,
// through ApplyHit like every other attack: a shove along its flight, a
// spark and the flinch and hit-pause that come with a hit. It used to take
// health and nothing else, so a fireball landed without a reaction
global_variable hit FireBallHitRow =
    {PlayerStats.FireballDamage, 200.f, 0.f, 120.f, 0.f, SimBurst_Impact};

internal void
FireBallHit(app_state *AppState, world *World, world_entity *FireBall,
            world_entity *Target)
{
    // NOTE(zoubir): it flies on past a player jumping over it, which still
    // counts as its one pass through that player
    if (IsJumpingClear(Target))
    {
        return;
    }
    v2 Away = NormalizeOr(FireBall->Velocity.XY,
                          Target->Position.XY - FireBall->Position.XY);
    u32 BySlot = FireBall->HasOwner ? FireBall->OwnerSlot : SIM_NOBODY;
    ApplyHit(AppState, World, Target, &FireBallHitRow, Away, FireBall, BySlot);
}
