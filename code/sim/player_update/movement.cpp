/* Player movement: whether the player walks this tick (a swing or cast
   roots it for a moment), and the physics step: acceleration, ground
   friction and gravity into MoveEntity. */

// NOTE(zoubir): letting go of the keys faster than this raises a skid;
// a full walk is about 260
#define PLAYER_SKID_SPEED (70.f * PLAYER_MOVE_SCALE)
// NOTE(zoubir): share of its speed the player loses per second on plain
// ground; speed eases toward the keys' push over 1/PLAYER_DRAG seconds
#define PLAYER_DRAG 10.f
// NOTE(zoubir): how fast the player walks while a swing finishes
#define PLAYER_ACTION_MOVE_SCALE 0.7f

internal void
UpdatePlayerMoveState(app_state *AppState, world_entity *Player,
                      player_tick *Tick)
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
        // NOTE(zoubir): stopping from a run on the ground kicks up a skid
        if (!Tick->Jumping &&
            LengthSq(Player->Velocity.XY) > Square(PLAYER_SKID_SPEED))
        {
            v2 Heading = Player->Velocity.XY;
            EmitBurst(&AppState->Events, SimBurst_Skid, (u8)Player->PlayerIndex,
                      Player->Position, ATan2(Heading.Y, Heading.X));
        }
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
    // NOTE(zoubir): a long jump (combos.cpp) keeps its speed: the drag is
    // cut, and the keys' push with it so holding them does not speed up
    float DragScale = Player->LongJump ? LONG_JUMP_DRAG_SCALE : 1.f;
    DDPlayer *= DragScale * Tick->Acceleration * GetMoveSpeedScale(Player) *
        ACCELERATION_STEP;
    // Drag
    DDPlayer -= (DragScale * PLAYER_DRAG * GetGroundFriction(Player) * Player->Velocity);
    // Gravity
    DDPlayer.Z = -PLAYER_GRAVITY;

    float MaxDistance = 10000.f;
    MoveEntity(Player, World, Arena, DeltaTime, AppState, DDPlayer, &MaxDistance);
    if (IsOnGround(Player))
    {
        Player->LongJump = false;
    }
    FireAreaOnLanding(AppState, World, Player);
    UpdateVault(AppState, World, Player, Tick, DeltaTime);

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
