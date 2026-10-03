/* Player update: what one player does in a tick, driven by its slot's
   player_input (never the keyboard). UpdatePlayer reads as its steps:

   1. QueuePlayerActions: held directions set this tick's move; pressed
      buttons become queued actions (sword, fireball). Each waits a
      moment for the current animation, so a press during a swing is not
      lost. Sword and
      fireball take the aim (toward the cursor) at the moment of the press.
   2. FinishPlayerActions: a swing or cast whose animation ended frees
      the player.
   3. RunPlayerActionQueue: the first action that can run now does
      (StartSwordSwing or CastFireBall).
   4. UpdatePlayerMoveState: moving, or stopping when the keys let go. A
      swing or cast roots the player only for its short ActionLock, then
      the player walks (slower) while the animation finishes.
   5. UsePlayerAbilities: jump, shockwave and dash, with their cooldowns.
   6. PickPlayerAnimation: which animation to play; the body faces the
      aim, not the way it walks.
   7. MovePlayer: acceleration, ground friction and gravity into
      MoveEntity. */

#define PLAYER_ACCELERATION 56000.f
// NOTE(zoubir): how long a queued action waits for the current animation
#define PLAYER_ACTION_LINGER 0.15f
#define FIREBALL_SPEED 450.f
#define FIREBALL_HAND_HEIGHT 30.f
// NOTE(zoubir): how long a swing or cast roots the player, and how fast
// they walk for the rest of its animation
#define PLAYER_SWING_LOCK 0.08f
#define PLAYER_CAST_LOCK 0.05f
#define PLAYER_ACTION_MOVE_SCALE 0.7f

// NOTE(zoubir): what one tick of the player decides, handed between steps
struct player_tick
{
    bool32 Move;
    bool32 Jumping;
    float Acceleration;
    v3 DDPlayer;
    float *AnimationSpeedRate;
    animation_type *AnimationType;
    animation_direction *AnimationDirection;
};

// NOTE(zoubir): the sprite row closest to Dir, for any angle; X wins a
// perfect diagonal
inline animation_direction
DominantFacing(v2 Dir)
{
    animation_direction Result;
    if (Absolute(Dir.X) >= Absolute(Dir.Y))
    {
        Result = Dir.X < 0.f ? AnimationDirection_Left : AnimationDirection_Right;
    }
    else
    {
        Result = Dir.Y < 0.f ? AnimationDirection_Up : AnimationDirection_Down;
    }
    return Result;
}

// NOTE(zoubir): the player's aim; before any cursor input, the way it last
// walked (and Right for a player that never moved)
inline v2
GetPlayerAim(world_entity *Player)
{
    v2 Result = Player->Aim;
    if (LengthSq(Result) < 0.0001f)
    {
        Result = Player->Direction;
        float LengthSquared = LengthSq(Result);
        Result = LengthSquared > 0.0001f ?
            Result * (1.f / SquareRoot(LengthSquared)) : V2(1.f, 0.f);
    }
    return Result;
}

internal void
QueuePlayerActions(player_slot *Slot, player_tick *Tick)
{
    world_entity *Player = Slot->Entity;
    player_input *Input = &Slot->Input;
    bool32 Up = Input->Move.Y < 0.f;
    bool32 Down = Input->Move.Y > 0.f;
    bool32 Right = Input->Move.X > 0.f;
    bool32 Left = Input->Move.X < 0.f;

    v2 Dir = {};
    if (Up) Dir.Y = -1.f;
    if (Down) Dir.Y = 1.f;
    if (Right) Dir.X = 1.f;
    if (Left) Dir.X = -1.f;
    // NOTE(zoubir): held keys move this tick, they are not queued: a
    // queued move ran first and pushed a swing or cast back a tick
    if (Up || Down || Right || Left)
    {
        Player->Direction = Dir;
        Tick->Move = true;
    }

    if (LengthSq(Input->Aim) > 0.0001f)
    {
        Player->Aim = Input->Aim;
    }
    v2 Aim = GetPlayerAim(Player);

    Tick->Jumping = Player->Velocity.Z != 0.f;
    if (Tick->Jumping)
    {
        Player->State = EntityState_Jumping;
    }
    if (WasPressed(Input, PlayerButton_Attack))
    {
        AddPlayerDelayedInput(Slot, PDI_Attack, PLAYER_ACTION_LINGER, Aim);
    }
    if (WasPressed(Input, PlayerButton_Cast))
    {
        AddPlayerDelayedInput(Slot, PDI_Cast, PLAYER_ACTION_LINGER, Aim);
    }
}

internal void
FinishPlayerActions(world_entity *Player, player_tick *Tick)
{
    if (Player->State == EntityState_Attacking &&
        IsAnimationFinished(Player->AnimationSet, &Player->AnimationState,
                            AnimationType_Attack, *Tick->AnimationDirection))
    {
        Player->State = EntityState_Standing;
    }
    if (Player->State == EntityState_Casting &&
        IsAnimationFinished(Player->AnimationSet, &Player->AnimationState,
                            AnimationType_Cast, *Tick->AnimationDirection))
    {
        Player->State = EntityState_Standing;
    }
}

// NOTE(zoubir): a sword hitbox toward Dir (the aim); the player lunges a
// little that way
internal void
StartSwordSwing(app_state *AppState, world *World, memory_arena *Arena,
                world_entity *Player, v2 Dir, player_tick *Tick)
{
    Player->State = EntityState_Attacking;
    Player->ActionLock = PLAYER_SWING_LOCK;
    Player->CastingDirection = Dir;
    Player->AnimationState.SlotIndex = 0;
    Tick->Acceleration *= 0.6f;
    Tick->DDPlayer.XY = Dir;

    v3 SwordPosition = Player->Position + V3(16.f * Dir.X, 16.f * Dir.Y, 0.f);
    AddSword(AppState, World, Arena, SwordPosition, Player,
             DominantFacing(Dir));
    EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
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
    Player->CastingDirection = Dir;
    Player->AnimationState.SlotIndex = 0;
}

// NOTE(zoubir): runs the first queued action that can run; once one has,
// the rest only age and drop out when they have waited too long
internal void
RunPlayerActionQueue(app_state *AppState, world *World, memory_arena *Arena,
                     player_slot *Slot, float DeltaTime, player_tick *Tick)
{
    world_entity *Player = Slot->Entity;
    bool32 Halted = false;
    for(u32 Index = 0; Index < Slot->DelayedInputCount;)
    {
        player_delayed_input *Action = &Slot->DelayedInput[Index];
        bool32 Remove = false;
        if (Halted)
        {
            Action->TimeRemaining -= DeltaTime;
            Remove = Action->TimeRemaining <= 0.f;
        }
        else
        {
            bool32 Consumed = true;
            if (Action->TimeRemaining > 0.f)
            {
                switch (Action->Type)
                {
                    case PDI_Attack:
                    {
                        Consumed = Player->State != EntityState_Attacking;
                        if (Consumed)
                        {
                            StartSwordSwing(AppState, World, Arena, Player,
                                            Action->Dir, Tick);
                        }
                    } break;
                    case PDI_Cast:
                    {
                        CastFireBall(AppState, World, Arena, Player,
                                     Action->Dir, Tick);
                    } break;
                    default:
                    {
                        Consumed = false;
                    } break;
                }
            }
            if (Consumed)
            {
                Remove = true;
                Halted = true;
            }
            else
            {
                Action->TimeRemaining -= DeltaTime;
            }
        }

        if (Remove)
        {
            Slot->DelayedInput[Index] = Slot->DelayedInput[--Slot->DelayedInputCount];
        }
        else
        {
            Index++;
        }
    }
}

internal void
UpdatePlayerMoveState(world_entity *Player, player_tick *Tick)
{
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
UsePlayerAbilities(app_state *AppState, world *World, player_slot *Slot,
                   float DeltaTime, player_tick *Tick)
{
    world_entity *Player = Slot->Entity;
    player_input *Input = &Slot->Input;

    // NOTE(zoubir): only from the ground, pressing again mid-air used
    // to restart the jump and let the player fly
    if (WasPressed(Input, PlayerButton_Jump) && !Tick->Jumping)
    {
        Player->State = EntityState_Jumping;
        Player->Velocity.Z = 230.f;
        EmitSound(&AppState->Events, AssetType_ZoubirAudio, Player->Position);
    }

    Player->ShockwaveCooldown = Maximum(0.f, Player->ShockwaveCooldown - DeltaTime);
    Player->ShockwaveFlash = Maximum(0.f, Player->ShockwaveFlash - DeltaTime);
    if (WasPressed(Input, PlayerButton_Shockwave) &&
        Player->ShockwaveCooldown <= 0.f)
    {
        Player->ShockwaveCooldown = PLAYER_SHOCKWAVE_COOLDOWN;
        TriggerShockwave(AppState, World, Player);
        EmitSound(&AppState->Events, AssetType_FireCast, Player->Position);
    }

    Player->DashCooldown = Maximum(0.f, Player->DashCooldown - DeltaTime);
    if (WasPressed(Input, PlayerButton_Dash) &&
        Player->DashCooldown <= 0.f)
    {
        Player->DashCooldown = PLAYER_DASH_COOLDOWN;
        Tick->Acceleration *= 10;
        EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
    }
}

// NOTE(zoubir): the animation for the player's state. The body faces the
// aim; a swing or cast keeps the facing it started with so its animation
// is not cut by the cursor moving. Moving pushes the way the keys point.
internal void
PickPlayerAnimation(world_entity *Player, player_tick *Tick)
{
    if (Player->State == EntityState_Stopping &&
        IsAnimationFinished(Player->AnimationSet, &Player->AnimationState,
                            AnimationType_Stop, *Tick->AnimationDirection))
    {
        Player->State = EntityState_Standing;
    }

    v2 Facing = GetPlayerAim(Player);
    if (Player->State == EntityState_Attacking ||
        Player->State == EntityState_Casting)
    {
        Facing = Player->CastingDirection;
    }
    *Tick->AnimationDirection = DominantFacing(Facing);

    if (Player->State == EntityState_Stopping)
    {
        Assert(!Tick->Move);
        *Tick->AnimationType = AnimationType_Stop;
    }
    else if (Player->State == EntityState_Attacking)
    {
        *Tick->AnimationType = AnimationType_Attack;
    }
    else if (Player->State == EntityState_Casting)
    {
        *Tick->AnimationType = AnimationType_Cast;
    }
    else if (Tick->Move)
    {
        *Tick->AnimationType = AnimationType_Move;
    }
    else
    {
        *Tick->AnimationType = AnimationType_Stand;
    }
    if (Tick->Move)
    {
        Tick->DDPlayer.XY = Player->Direction;
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
    DDPlayer *= Tick->Acceleration * GetMoveSpeedScale(Player) * DeltaTime;
    // Drag
    DDPlayer -= (10.f * GetGroundFriction(Player) * Player->Velocity);
    // Gravity
    DDPlayer.Z = -1000.f;

    float MaxDistance = 10000.f;
    MoveEntity(Player, World, Arena, DeltaTime, AppState, DDPlayer, &MaxDistance);

    if (Player->Velocity.Z > 0.f)
    {
        *Tick->AnimationType = AnimationType_JumpUp;
    }
    if (Player->Velocity.Z < 0.f)
    {
        *Tick->AnimationType = AnimationType_JumpDown;
    }
}

internal void
UpdatePlayer(player_slot *Slot, world *World,
             memory_arena *Arena,
             float DeltaTime, app_state *AppState,
             float *AnimationSpeedRate,
             animation_type *AnimationType,
             animation_direction *AnimationDirection)
{
    world_entity *Player = Slot->Entity;
    *AnimationType = AnimationType_Stand;
    *AnimationDirection = Player->AnimationState.LastAnimationDirection;
    *AnimationSpeedRate = 1.f;

    Player->ActionLock = Maximum(0.f, Player->ActionLock - DeltaTime);
    player_tick Tick = {};
    Tick.Acceleration = PLAYER_ACCELERATION;
    Tick.AnimationSpeedRate = AnimationSpeedRate;
    Tick.AnimationType = AnimationType;
    Tick.AnimationDirection = AnimationDirection;

    QueuePlayerActions(Slot, &Tick);
    FinishPlayerActions(Player, &Tick);
    RunPlayerActionQueue(AppState, World, Arena, Slot, DeltaTime, &Tick);
    UpdatePlayerMoveState(Player, &Tick);
    UsePlayerAbilities(AppState, World, Slot, DeltaTime, &Tick);
    PickPlayerAnimation(Player, &Tick);
    MovePlayer(AppState, World, Arena, Player, DeltaTime, &Tick);
}
