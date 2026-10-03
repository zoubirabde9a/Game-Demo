/* Fireball (left click): a piercing shot along the aim at any angle,
   hitting each target once (HandleCollision, collision.cpp). Roots the
   player for PLAYER_CAST_LOCK. One every PLAYER_FIREBALL_INTERVAL: a
   click sooner waits in the action queue and fires when it may, so fast
   clicking gives a steady stream rather than a spray. */

#define FIREBALL_SPEED 450.f
#define FIREBALL_HAND_HEIGHT 30.f
#define PLAYER_CAST_LOCK 0.05f
// NOTE(zoubir): about three a second; there was no limit, and each click
// restarted the cast animation
#define PLAYER_FIREBALL_INTERVAL 0.35f

inline bool32
CanCastFireBall(world_entity *Player)
{
    bool32 Result = Player->FireBallCooldown <= 0.f;
    return Result;
}

internal void
CastFireBall(app_state *AppState, world *World, memory_arena *Arena,
             world_entity *Player, v2 Dir, player_tick *Tick)
{
    v2 Start = Player->Position.XY + Dir * V2(32.f, 32.f);
    v2 Velocity = FIREBALL_SPEED * Dir;
    *Tick->AnimationType = AnimationType_Cast;
    world_entity *FireBall =
        AddFireBall(AppState, World, Arena, Player,
                    V3(Start.X, Start.Y, FIREBALL_HAND_HEIGHT),
                    V3(Velocity.X, Velocity.Y, 0.f));
    FireBall->AnimationSpeed = 1.f;
    FireBall->AnimationType = AnimationType_Move;
    FireBall->AnimationDirection = DominantFacing(Dir);

    EmitSound(&AppState->Events, AssetType_FireCast, Player->Position);
    Player->State = EntityState_Casting;
    Player->ActionLock = PLAYER_CAST_LOCK;
    Player->FireBallCooldown = PLAYER_FIREBALL_INTERVAL;
    Player->CastingDirection = Dir;
    Player->AnimationState.SlotIndex = 0;
}

