/* Prediction: while online, the local player's replica moves the moment a
   key is held instead of a round trip later. Every frame's held buttons
   are recorded with the input tick the client sent them under. Between
   snapshots the replica is moved by this frame's input. When a snapshot
   arrives the replica has just been put where the server had it, after
   the input the server acknowledges (InputTick); the inputs the server
   has not applied yet are replayed on top, so the replica lands where the
   server will have it once those inputs arrive.

   Only movement is predicted. Attacks, dashes and jumps change what other
   players see, so they wait for the server. */

#define MAX_PREDICTED_INPUTS 128

struct predicted_input
{
    u32 Tick;
    u16 Buttons;
    float DeltaTime;
};

// NOTE(zoubir): a ring of the inputs the server has not acknowledged yet,
// oldest at First
struct prediction_history
{
    predicted_input Inputs[MAX_PREDICTED_INPUTS];
    u32 First;
    u32 Count;
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
                     float DeltaTime)
{
    if (History->Count == MAX_PREDICTED_INPUTS)
    {
        History->First = (History->First + 1) % MAX_PREDICTED_INPUTS;
        History->Count--;
    }
    predicted_input *Input = GetPredictedInput(History, History->Count++);
    Input->Tick = Tick;
    Input->Buttons = Buttons;
    Input->DeltaTime = DeltaTime;
}

internal void
DropAcknowledgedInputs(prediction_history *History, u32 InputTick)
{
    while (History->Count > 0 &&
           GetPredictedInput(History, 0)->Tick <= InputTick)
    {
        History->First = (History->First + 1) % MAX_PREDICTED_INPUTS;
        History->Count--;
    }
}

// NOTE(zoubir): one frame of the local player's movement, by the same
// UpdatePlayer the server runs; returns false when there is no live
// local player to move
internal bool32
PredictLocalStep(app_state *AppState, memory_arena *Arena, u16 Buttons,
                 float DeltaTime)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Player = Slot->Entity;
    if (!Slot->Active || !Player || !Player->IsPresent ||
        IsDeadPlayer(Player))
    {
        return false;
    }

    Slot->Input = {};
    Slot->Input.Move = MoveFromNetButtons(Buttons);
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    UpdatePlayer(Slot, &AppState->World, Arena, DeltaTime, AppState,
                 &AnimationSpeed, &AnimationType, &AnimationDirection);
    Player->AnimationType = AnimationType;
    Player->AnimationDirection = AnimationDirection;
    return true;
}

// NOTE(zoubir): call after SyncReplicas. On a new snapshot the replica
// sits where the server had it, so replay every input it has not applied;
// otherwise move it by this frame's input, the newest in the history.
internal void
PredictLocalPlayer(app_state *AppState, memory_arena *Arena,
                   prediction_history *History, bool32 NewSnapshot,
                   u32 InputTick)
{
    if (NewSnapshot)
    {
        DropAcknowledgedInputs(History, InputTick);
        for(u32 Index = 0; Index < History->Count; Index++)
        {
            predicted_input *Input = GetPredictedInput(History, Index);
            if (!PredictLocalStep(AppState, Arena, Input->Buttons,
                                  Input->DeltaTime))
            {
                break;
            }
        }
    }
    else if (History->Count > 0)
    {
        predicted_input *Newest = GetPredictedInput(History, History->Count - 1);
        PredictLocalStep(AppState, Arena, Newest->Buttons, Newest->DeltaTime);
    }
}
