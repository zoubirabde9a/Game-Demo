/* Prediction: while online, the local player's replica moves the moment a
   key is held instead of a round trip later. Every frame's held buttons
   are recorded with the input tick the client sent them under. Between
   snapshots the replica is moved by this frame's input. When a snapshot
   arrives the replica has just been put where the server had it, after
   the input the server acknowledges (InputTick); the inputs the server
   has not applied yet are replayed on top, so the replica lands where the
   server will have it once those inputs arrive.

   Movement, facing (the aim toward the cursor), jump, dash and blink are
   predicted: they move only the player, so they happen the frame they
   are pressed. Snapshots carry no vertical speed, so each input keeps
   the player's vertical speed after it, and a replay starts from the
   one the server last acknowledged rather than from zero (which pulled
   a jump down between snapshots); the jumps spent go the same way, so a
   replayed double jump is still allowed. Each recorded input keeps which buttons went down on it,
   worked out the same way the server does, and the cooldowns the server
   sends say whether dash and blink are ready; UpdatePlayer counts them
   down as it steps, replays included. Attacks, shockwaves and jumps
   change what other players see, so they wait for the server.

   When the replay lands somewhere other than where the player was drawn a
   frame ago, the difference is kept as DrawError and the player is drawn
   that far off its predicted position, shrinking each frame, so a
   correction slides in over ~100 ms instead of snapping. Movement and
   collision always run from the predicted position; the offset is added
   back only after them. A jump longer than PREDICTION_SNAP_DISTANCE (a
   respawn) snaps. */

#define MAX_PREDICTED_INPUTS 128
// NOTE(zoubir): a correction farther than this is a teleport, not an error
#define PREDICTION_SNAP_DISTANCE (4.f * ARENA_TILE_SIZE)
// NOTE(zoubir): share of DrawError removed per second; at 60 fps a
// correction is under a tenth of its size after 100 ms
#define PREDICTION_BLEND_RATE 20.f

struct predicted_input
{
    u32 Tick;
    u16 Buttons;
    // NOTE(zoubir): buttons that went down on this input (were not held on
    // the one before), as the server works them out
    u16 Pressed;
    v2 Aim;
    float DeltaTime;
    // NOTE(zoubir): the player's vertical speed and jumps spent once this
    // input was applied
    float VelocityZAfter;
    u32 JumpsUsedAfter;
};

// NOTE(zoubir): a ring of the inputs the server has not acknowledged yet,
// oldest at First
struct prediction_history
{
    predicted_input Inputs[MAX_PREDICTED_INPUTS];
    u32 First;
    u32 Count;
    // NOTE(zoubir): last frame's predicted position; the player was drawn
    // DrawError away from it
    bool32 HasShown;
    v2 Predicted;
    v2 DrawError;
    u16 LastButtons;
    // NOTE(zoubir): VelocityZAfter and JumpsUsedAfter of the newest input
    // the server applied
    float AckedVelocityZ;
    u32 AckedJumpsUsed;
};

// NOTE(zoubir): the server turns held net buttons into a move direction
// the same way, in GameApplyInput (server/sim_game.cpp)
inline v2
MoveFromNetButtons(u16 Buttons)
{
    v2 Result = {};
    if (Buttons & NetButton_Left) Result.X -= 1.f;
    if (Buttons & NetButton_Right) Result.X += 1.f;
    if (Buttons & NetButton_Up) Result.Y -= 1.f;
    if (Buttons & NetButton_Down) Result.Y += 1.f;
    return Result;
}

inline predicted_input *
GetPredictedInput(prediction_history *History, u32 Index)
{
    predicted_input *Result =
        &History->Inputs[(History->First + Index) % MAX_PREDICTED_INPUTS];
    return Result;
}

// NOTE(zoubir): when the history is full the oldest input is dropped; the
// next snapshot corrects whatever that costs
internal void
RecordPredictedInput(prediction_history *History, u32 Tick, u16 Buttons,
                     float DeltaTime, v2 Aim = {})
{
    if (History->Count == MAX_PREDICTED_INPUTS)
    {
        History->First = (History->First + 1) % MAX_PREDICTED_INPUTS;
        History->Count--;
    }
    predicted_input *Input = GetPredictedInput(History, History->Count++);
    Input->Tick = Tick;
    Input->Buttons = Buttons;
    Input->Pressed = Buttons & ~History->LastButtons;
    History->LastButtons = Buttons;
    Input->Aim = Aim;
    Input->DeltaTime = DeltaTime;
}

internal void
DropAcknowledgedInputs(prediction_history *History, u32 InputTick)
{
    while (History->Count > 0 &&
           GetPredictedInput(History, 0)->Tick <= InputTick)
    {
        History->AckedVelocityZ = GetPredictedInput(History, 0)->VelocityZAfter;
        History->AckedJumpsUsed = GetPredictedInput(History, 0)->JumpsUsedAfter;
        History->First = (History->First + 1) % MAX_PREDICTED_INPUTS;
        History->Count--;
    }
}

// NOTE(zoubir): one frame of the local player's movement, by the same
// UpdatePlayer the server runs; returns false when there is no live
// local player to move
internal bool32
PredictLocalStep(app_state *AppState, memory_arena *Arena,
                 predicted_input *Input)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Player = Slot->Entity;
    if (!Slot->Active || !Player || !Player->IsPresent ||
        IsDeadPlayer(Player))
    {
        return false;
    }

    Slot->Input = {};
    Slot->Input.Move = MoveFromNetButtons(Input->Buttons);
    Slot->Input.Aim = Input->Aim;
    Slot->Input.Pressed = ((u32)Input->Pressed >> PLAYER_BUTTON_NET_SHIFT) &
        (PlayerButton_Jump | PlayerButton_Dash | PlayerButton_Blink);
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    UpdatePlayer(Slot, &AppState->World, Arena, Input->DeltaTime, AppState,
                 &AnimationSpeed, &AnimationType, &AnimationDirection);
    Input->VelocityZAfter = Player->Velocity.Z;
    Input->JumpsUsedAfter = Player->JumpsUsed;
    Player->AnimationType = AnimationType;
    Player->AnimationDirection = AnimationDirection;
    return true;
}

// NOTE(zoubir): moves the local player's drawn position, keeping the
// world's chunk lists right
internal void
SetLocalPlayerXY(app_state *AppState, memory_arena *Arena,
                 world_entity *Player, v2 XY)
{
    v3 OldPosition = Player->Position;
    Player->Position.XY = XY;
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena,
                              OldPosition, Player);
}

// NOTE(zoubir): call after SyncReplicas. On a new snapshot the replica
// sits where the server had it, so replay every input it has not applied;
// otherwise move it by this frame's input, the newest in the history.
internal void
PredictLocalPlayer(app_state *AppState, memory_arena *Arena,
                   prediction_history *History, bool32 NewSnapshot,
                   u32 InputTick, float DeltaTime)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Player = Slot->Entity;
    bool32 Moved = true;
    if (NewSnapshot)
    {
        DropAcknowledgedInputs(History, InputTick);
        if (Player && Player->Position.Z > 0.f)
        {
            Player->Velocity.Z = History->AckedVelocityZ;
            Player->JumpsUsed = History->AckedJumpsUsed;
        }
        for(u32 Index = 0; Index < History->Count && Moved; Index++)
        {
            Moved = PredictLocalStep(AppState, Arena,
                                     GetPredictedInput(History, Index));
        }
    }
    else
    {
        // NOTE(zoubir): last frame drew the player DrawError off its
        // predicted position; step from the predicted one
        if (Player && History->HasShown)
        {
            SetLocalPlayerXY(AppState, Arena, Player, History->Predicted);
        }
        if (History->Count > 0)
        {
            Moved = PredictLocalStep(AppState, Arena,
                                     GetPredictedInput(History, History->Count - 1));
        }
    }

    if (!Moved || !Player || !Player->IsPresent || IsDeadPlayer(Player))
    {
        History->HasShown = false;
        History->DrawError = {};
        return;
    }

    v2 Predicted = Player->Position.XY;
    if (NewSnapshot)
    {
        History->DrawError = History->HasShown ?
            (History->Predicted + History->DrawError) - Predicted :
            V2(0.f, 0.f);
        if (LengthSq(History->DrawError) >
            Square(PREDICTION_SNAP_DISTANCE))
        {
            History->DrawError = {};
        }
    }
    float Keep = 1.f - PREDICTION_BLEND_RATE * DeltaTime;
    History->DrawError *= (Keep > 0.f) ? Keep : 0.f;
    if (LengthSq(History->DrawError) < Square(0.01f))
    {
        History->DrawError = {};
    }
    SetLocalPlayerXY(AppState, Arena, Player, Predicted + History->DrawError);
    History->Predicted = Predicted;
    History->HasShown = true;
}
