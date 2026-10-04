/* Player movement: whether the player walks this tick (a swing or cast
   roots it for a moment), and the physics step: acceleration, ground
   friction and gravity into MoveEntity. */

// NOTE(zoubir): how fast the player walks while a swing finishes
#define PLAYER_ACTION_MOVE_SCALE 0.7f

internal void
UpdatePlayerMoveState(world_entity *Player, player_tick *Tick)
{
    // NOTE(zoubir): the fireball left when the cast began, so walking cuts
    // the rest of the cast animation; holding fire used to mean walking at
    // PLAYER_ACTION_MOVE_SCALE in the cast pose for good
    if (Player->State == EntityState_Casting && Player->ActionLock <= 0.f &&
        Tick->Move)
    {
        Player->State = EntityState_Moving;
    }

    if (Player->State == EntityState_Attacking ||
        Player->State == EntityState_Casting)
    {
        if (Player->ActionLock > 0.f)
        {
            Tick->Move = false;
        }
        else if (Tick->Move)
        {
            Tick->Acceleration *= PLAYER_ACTION_MOVE_SCALE;
        }
    }
    else if (Tick->Move)
    {
        Player->State = EntityState_Moving;
    }
    else if (Player->State == EntityState_Moving)
    {
        Player->State = EntityState_Stopping;
        Player->AnimationState.SlotIndex = 0;
    }
}

internal void
MovePlayer(app_state *AppState, world *World, memory_arena *Arena,
           world_entity *Player, float DeltaTime, player_tick *Tick)
{
    v3 DDPlayer = Tick->DDPlayer;
    float LengthSquared = LengthSq(DDPlayer);
    if (LengthSquared > 1.f)
    {
        DDPlayer *= 1.f / SquareRoot(LengthSquared);
    }
    DDPlayer *= Tick->Acceleration * GetMoveSpeedScale(Player) * ACCELERATION_STEP;
    // Drag
    DDPlayer -= (10.f * GetGroundFriction(Player) * Player->Velocity);
    // Gravity
    DDPlayer.Z = -PLAYER_GRAVITY;

    float MaxDistance = 10000.f;
    MoveEntity(Player, World, Arena, DeltaTime, AppState, DDPlayer, &MaxDistance);
    FireAreaOnLanding(AppState, World, Player);

    // NOTE(zoubir): in the air the jump frames show, unless a swing or a
    // cast is playing: those used to be replaced by the jump frame, so an
    // attack in the air had no animation
    bool32 Acting = *Tick->AnimationType == AnimationType_Attack ||
        *Tick->AnimationType == AnimationType_Cast;
    if (!Acting && Player->Velocity.Z > 0.f)
    {
        *Tick->AnimationType = AnimationType_JumpUp;
    }
    if (!Acting && Player->Velocity.Z < 0.f)
    {
        *Tick->AnimationType = AnimationType_JumpDown;
    }
}
