/* Player movement: whether the player walks this tick (a swing or cast
   roots it for a moment, a shove staggers it), and the physics step:
   the walk toward the keys' direction, the drag on anything faster than
   a run (or staggered), and gravity into MoveEntity. The numbers are in player_stats.cpp. */

// NOTE(zoubir): speed past the top walking speed by more than this share
// counts as carried (a dash, a long jump, a knockback): the drag eases it
// down instead of the walk taking over at once
#define PLAYER_CARRY_MARGIN 1.05f

internal void
UpdatePlayerMoveState(app_state *AppState, world_entity *Player,
                      player_tick *Tick)
{
    // NOTE(zoubir): a shoved player slides; the keys wait for the stagger
    if (Player->Stagger > 0.f)
    {
        Tick->Move = false;
    }
    // NOTE(zoubir): the fireball left when the cast began, so walking cuts
    // the rest of the cast animation; holding fire used to mean walking at
    // ActionWalkScale in the cast pose for good
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
            Tick->Acceleration *= PlayerStats.ActionWalkScale;
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
            LengthSq(Player->Velocity.XY) > Square(PlayerStats.SkidSpeed))
        {
            v2 Heading = Player->Velocity.XY;
            EmitBurst(&AppState->Events, SimBurst_Skid, (u8)Player->PlayerIndex,
                      Player->Position, ATan2(Heading.Y, Heading.X));
        }
    }
}

// NOTE(zoubir): the sideways acceleration for this tick. At walking speed
// the velocity goes straight to the keys' (Push, at most length 1, times
// TopSpeed) at a capped rate, landing on it exactly rather than easing in;
// speeding up uses the run-up rate, anything that slows or turns the body
// the stopping rate. Faster than that the speed is carried: the keys push
// and the drag pulls, which settles at TopSpeed as before.
internal v2
PlayerWalkAcceleration(world_entity *Player, v2 Push, float TopSpeed,
                       float DragScale, float DeltaTime)
{
    v2 Velocity = Player->Velocity.XY;
    float Friction = GetGroundFriction(Player);
    float Drag = DragScale * PlayerStats.CarryDrag * Friction;
    v2 Result;
    if (Player->LongJump || Player->Stagger > 0.f ||
        LengthSq(Velocity) > Square(PLAYER_CARRY_MARGIN * TopSpeed))
    {
        Result = Drag * (TopSpeed * Push - Velocity);
    }
    else
    {
        v2 Change = TopSpeed * Push - Velocity;
        float Rate = DotProduct(Change, Velocity) >= 0.f ?
            PlayerStats.RunSpeed / PlayerStats.RunUpSeconds :
            PlayerStats.RunSpeed / PlayerStats.StopSeconds;
        float MaxChange = Friction * Rate * DeltaTime;
        float ChangeLength = Length(Change);
        if (ChangeLength > MaxChange)
        {
            Change *= MaxChange / ChangeLength;
        }
        Result = Change * (1.f / DeltaTime);
    }
    return Result;
}

internal void
MovePlayer(app_state *AppState, world *World, memory_arena *Arena,
           world_entity *Player, float DeltaTime, player_tick *Tick)
{
    // NOTE(zoubir): staggered, the keys do not push; a dash this tick has
    // already ended the stagger (UseMovementAbilities)
    v2 Push = Player->Stagger > 0.f ? V2(0.f, 0.f) : Tick->DDPlayer.XY;
    float LengthSquared = LengthSq(Push);
    if (LengthSquared > 1.f)
    {
        Push *= 1.f / SquareRoot(LengthSquared);
    }
    // NOTE(zoubir): a long jump (combos.cpp) keeps its speed: the drag is
    // cut, and the keys' push with it so holding them does not speed up
    float DragScale = Player->LongJump ? LONG_JUMP_DRAG_SCALE : 1.f;
    // NOTE(zoubir): top speed as it always was, push over drag: slows and
    // mud lower it, ice keeps it (less push, less drag)
    float TopSpeed = PlayerStats.RunSpeed * Tick->Acceleration *
        GetMoveSpeedScale(Player) / GetGroundFriction(Player);
    v3 DDPlayer = {};
    if (DeltaTime > 0.f)
    {
        DDPlayer.XY = PlayerWalkAcceleration(Player, Push, TopSpeed,
                                             DragScale, DeltaTime);
    }
    DDPlayer.Z = -PLAYER_GRAVITY;
    if (Tick->Hover)
    {
        Player->Velocity.Z = 0.f;
        DDPlayer.Z = 0.f;
    }

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
