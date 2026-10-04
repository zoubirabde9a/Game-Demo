/* Player actions: presses of the spawn actions (sword, fireball,
   spawn_actions.cpp) wait in the slot's action queue for the current
   animation or their interval, then run.
   Held movement keys are read here too, but never queued. */

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

    Tick->Jumping = !IsOnGround(Player);
    if (Tick->Jumping)
    {
        Player->State = EntityState_Jumping;
    }
    for(u32 Index = 0; Index < PlayerAction_Count; Index++)
    {
        player_spawn_action *Action = &PlayerSpawnActions[Index];
        if (WasPressed(Input, Action->Button))
        {
            AddPlayerDelayedInput(Slot, (player_action)Index, Action->Linger);
        }
    }
}

internal void
FinishPlayerActions(world_entity *Player, player_tick *Tick)
{
    for(u32 Index = 0; Index < PlayerAction_Count; Index++)
    {
        player_spawn_action *Action = &PlayerSpawnActions[Index];
        if (Player->State == Action->State &&
            IsAnimationFinished(Player->AnimationSet, &Player->AnimationState,
                                Action->Animation, *Tick->AnimationDirection))
        {
            Player->State = EntityState_Standing;
        }
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
                Consumed = CanStartSpawnAction(Player, Action->Type);
                if (Consumed)
                {
                    StartSpawnAction(AppState, World, Arena, Player,
                                     Action->Type, GetPlayerAim(Player), Tick);
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
