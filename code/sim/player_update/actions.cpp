/* Player actions: sword and fireball presses wait in the slot's action
   queue for the current animation (or the fireball interval), then run.
   Held movement keys are read here too, but never queued. */

// NOTE(zoubir): how long a queued action waits for the current animation;
// longer than a whole swing (6 frames of 0.03 s, animations.cpp), so a
// click anywhere in a swing chains the next one. It was 0.15 s, and clicks
// in a swing's first moments were dropped
#define PLAYER_ACTION_LINGER 0.25f

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

    float AimSquared = LengthSq(Input->Aim);
    if (AimSquared > 0.0001f)
    {
        float AimLength = SquareRoot(AimSquared);
        Player->Aim = Input->Aim * (1.f / AimLength);
        Player->AimReach = Minimum(1.f, AimLength);
    }

    Tick->Jumping = Player->Velocity.Z != 0.f;
    if (Tick->Jumping)
    {
        Player->State = EntityState_Jumping;
    }
    if (WasPressed(Input, PlayerButton_Attack))
    {
        AddPlayerDelayedInput(Slot, PDI_Attack, PLAYER_ACTION_LINGER);
    }
    if (WasPressed(Input, PlayerButton_Cast))
    {
        // NOTE(zoubir): a click during the fireball interval waits it out
        AddPlayerDelayedInput(Slot, PDI_Cast,
                              Maximum(PLAYER_ACTION_LINGER,
                                      PLAYER_FIREBALL_INTERVAL));
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
                                            GetPlayerAim(Player), Tick);
                        }
                    } break;
                    case PDI_Cast:
                    {
                        Consumed = CanCastFireBall(Player);
                        if (Consumed)
                        {
                            CastFireBall(AppState, World, Arena, Player,
                                         GetPlayerAim(Player), Tick);
                        }
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
