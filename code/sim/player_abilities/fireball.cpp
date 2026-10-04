/* Fireball (left click): a piercing shot along the aim at any angle,
   hitting each target once (HandleCollision, collision.cpp, calls
   FireBallHit). Its numbers are in player_stats.cpp and its timing row
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

internal void
FireBallHit(app_state *AppState, world *World, world_entity *FireBall,
            world_entity *Target)
{
    DamageEntity(AppState, World, Target, PlayerStats.FireballDamage, FireBall);
}
