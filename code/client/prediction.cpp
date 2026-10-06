/* Prediction: while online, the local player's replica moves the moment a
   key is held instead of a round trip later. Every frame's held buttons
   are recorded with the input tick the client sent them under. Between
   snapshots the replica is moved by this frame's input. When a snapshot
   arrives the replica has just been put where the server had it, after
   the input the server acknowledges (InputTick); the inputs the server
   has not applied yet are replayed on top, so the replica lands where the
   server will have it once those inputs arrive.

   Movement, facing (the aim toward the cursor), jump and the movement
   abilities (dash, blink, the slam's dive) are predicted: they move only
   the player, so they happen the frame they are pressed. Snapshots carry
   the player's speed and its stagger from a shove (only the server's
   hits start one), so a replay starts from the server's, throws, shoves
   and launches included, and slides a shove out as the server does.
   They carry no jumps spent, waiting jump press or cast under way, so
   each input keeps those as they were after
   it (predicted_body), and a replay starts from the ones the server last
   acknowledged (without, replayed double jumps were refused). The combo
   trail is kept the same way, and attack and cast presses, which only
   the server runs, still mark it (player_input.ServerPressed), so a
   predicted dash knows a swing came before it. Each recorded input keeps which buttons went down on it, worked
   out the same way the server does, and the cooldowns the server sends
   say whether the movement abilities are ready; UpdatePlayer counts them
   down as it steps, replays included. Every wind-up (sim/player_casts.cpp:
   area abilities, the slam, the rewinds) is predicted too, the slowdown,
   the pose and the cast bar, but what it does to others when it ends,
   and attacks, wait for the server.

   Inputs are one server tick each (client/online_pacing.cpp), so the
   player steps NET_TICK_RATE times a second like on the server, and each
   frame draws it between its two newest steps by how far the frame stands
   between ticks, on a curve that keeps the velocity of each
   (MotionCurve, replica_smoothing.cpp). When a replay puts the newest input shown last frame
   somewhere other than where it was shown, the difference is kept as
   DrawError and the player is drawn that far off, shrinking each frame,
   so a correction slides in over ~100 ms instead of snapping. Movement
   and collision always run from the predicted position; the blend and
   the offset are added only after them. A jump longer than
   PREDICTION_SNAP_DISTANCE (a respawn) snaps.

   While a time rewind has frozen the player (sim/time_rewind/), nothing
   is predicted: the replica stays where the server, and the rewind's
   playback (client/rewind_fx/), put it. */

#define MAX_PREDICTED_INPUTS 128
// NOTE(zoubir): a correction farther than this is a teleport, not an error
#define PREDICTION_SNAP_DISTANCE (4.f * ARENA_TILE_SIZE)
// NOTE(zoubir): DrawError shrinks by e to this power a second, whatever
// the frame rate: a correction is an eighth of its size after 100 ms
#define PREDICTION_BLEND_RATE 20.f

// NOTE(zoubir): what snapshots do not carry about the local player's body
// but its prediction needs to replay from: each input keeps it as it was
// after that input, and a replay starts from the newest the server
// acknowledged. A new field the player's moves depend on goes here.
struct predicted_body
{
    u32 JumpsUsed;
    float JumpBuffer;
    float VaultPush;
    // NOTE(zoubir): a cast under way (sim/player_casts.cpp)
    u32 CastSpell;
    float CastLeft;
    v2 CastingDirection;
    // NOTE(zoubir): the combo trail and a long jump under way (combos.cpp)
    u32 ComboTrail[PLAYER_COMBO_TRAIL];
    float ComboTrailAge[PLAYER_COMBO_TRAIL];
    bool32 LongJump;
};

inline predicted_body
SavePredictedBody(world_entity *Player)
{
    predicted_body Result;
    Result.JumpsUsed = Player->JumpsUsed;
    Result.JumpBuffer = Player->JumpBuffer;
    Result.VaultPush = Player->VaultPush;
    Result.CastSpell = Player->CastSpell;
    Result.CastLeft = Player->CastLeft;
    Result.CastingDirection = Player->CastingDirection;
    for(u32 Index = 0; Index < PLAYER_COMBO_TRAIL; Index++)
    {
        Result.ComboTrail[Index] = Player->ComboTrail[Index];
        Result.ComboTrailAge[Index] = Player->ComboTrailAge[Index];
    }
    Result.LongJump = Player->LongJump;
    return Result;
}

// NOTE(zoubir): the jump state only when the snapshot has the player in
// the air; on the ground the server's word (standing) wins
inline void
RestorePredictedBody(world_entity *Player, predicted_body *Body)
{
    if (Player->Position.Z > 0.f)
    {
        Player->JumpsUsed = Body->JumpsUsed;
        Player->JumpBuffer = Body->JumpBuffer;
    }
    Player->VaultPush = Body->VaultPush;
    Player->CastSpell = Body->CastSpell;
    Player->CastLeft = Body->CastLeft;
    Player->CastingDirection = Body->CastingDirection;
    for(u32 Index = 0; Index < PLAYER_COMBO_TRAIL; Index++)
    {
        Player->ComboTrail[Index] = Body->ComboTrail[Index];
        Player->ComboTrailAge[Index] = Body->ComboTrailAge[Index];
    }
    Player->LongJump = Body->LongJump;
}

struct predicted_input
{
    u32 Tick;
    u32 Buttons;
    // NOTE(zoubir): buttons that went down on this input (were not held on
    // the one before), as the server works them out
    u32 Pressed;
    v2 Aim;
    float DeltaTime;
    // NOTE(zoubir): the body once this input was applied
    predicted_body After;
};

// NOTE(zoubir): a ring of the inputs the server has not acknowledged yet,
// oldest at First
struct prediction_history
{
    predicted_input Inputs[MAX_PREDICTED_INPUTS];
    u32 First;
    u32 Count;
    // NOTE(zoubir): where the newest step put the player (Predicted, after
    // input ShownTick) and the one before (Before); last frame drew the
    // player between them, DrawError away
    bool32 HasShown;
    v3 Predicted;
    v3 Before;
    v3 BeforeVelocity;
    u32 ShownTick;
    v2 DrawError;
    // NOTE(zoubir): snapshots that moved the player off where it was
    // shown, and by how much in all, for tests and diagnosis
    u32 Corrections;
    float CorrectedDistance;
    u32 LastButtons;
    // NOTE(zoubir): After of the newest input the server applied
    predicted_body Acked;
};

// NOTE(zoubir): the player_button bits the client acts on for its own
// player before the server answers: the jump, every movement and area
// ability and the rewinds' wind-ups (their effects on others still wait
// for the server)
inline u32
PredictedButtons()
{
    u32 Result = PlayerButton_Jump | PlayerMovementButtons() | PlayerAreaButtons() |
        PlayerRewindButtons();
    return Result;
}

// NOTE(zoubir): the server turns held net buttons into a move direction
// the same way, in GameApplyInput (server/sim_game.cpp)
inline v2
MoveFromNetButtons(u32 Buttons)
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
RecordPredictedInput(prediction_history *History, u32 Tick, u32 Buttons,
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
    // NOTE(zoubir): stays empty for an input never stepped (the player was
    // dead), so a replay after a respawn does not start from the body of
    // an older input that last used this slot of the ring
    Input->After = {};
}

internal void
DropAcknowledgedInputs(prediction_history *History, u32 InputTick)
{
    while (History->Count > 0 &&
           GetPredictedInput(History, 0)->Tick <= InputTick)
    {
        History->Acked = GetPredictedInput(History, 0)->After;
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

    // NOTE(zoubir): the ground under the feet, as SimulateTick reads it for
    // every unit before it moves (sim/terrain_effects.cpp). Prediction does
    // not run SimulateTick, so off the stone floor (grass, sand, ice at the
    // map's edges) it walked at floor speed while the server did not, and
    // every snapshot pulled the player back. What standing there does to
    // health and status is left to the server
    Player->GroundSpeedScale = 1.f;
    Player->GroundFriction = 1.f;
    if (FeelsTerrain(&AppState->World, Player))
    {
        terrain_def *Ground =
            GetTerrainDef(TerrainUnder(&AppState->World, Player->Position));
        Player->GroundSpeedScale = Ground->SpeedScale;
        Player->GroundFriction = Ground->Friction;
    }

    Slot->Input = {};
    Slot->Input.Move = MoveFromNetButtons(Input->Buttons);
    Slot->Input.Aim = Input->Aim;
    u32 Pressed = (u32)Input->Pressed >> PLAYER_BUTTON_NET_SHIFT;
    Slot->Input.Pressed = Pressed & PredictedButtons();
    Slot->Input.ServerPressed = Pressed & ~PredictedButtons();
    float AnimationSpeed;
    animation_type AnimationType;
    animation_direction AnimationDirection;
    // NOTE(zoubir): only for this step, so going back to offline play the
    // slot acts in full again
    Slot->Predicted = true;
    UpdatePlayer(Slot, &AppState->World, Arena, Input->DeltaTime, AppState,
                 &AnimationSpeed, &AnimationType, &AnimationDirection);
    Slot->Predicted = false;
    Input->After = SavePredictedBody(Player);
    Player->AnimationType = AnimationType;
    Player->AnimationDirection = AnimationDirection;
    return true;
}

// NOTE(zoubir): in client/rewind_fx/rewind_fx.cpp, included later
internal bool32 IsLocalPlayerTimeLocked(app_state *AppState);

// NOTE(zoubir): puts the local player at its newest predicted position,
// undoing last frame's blend between steps and correction offset
internal void
ReturnToPredicted(app_state *AppState, memory_arena *Arena,
                  prediction_history *History, world_entity *Player)
{
    if (Player && History->HasShown)
    {
        v3 OldPosition = Player->Position;
        Player->Position = History->Predicted;
        CheckAndChangeEntityChunk(AppState, &AppState->World, Arena,
                                  OldPosition, Player);
    }
}

// NOTE(zoubir): call after SyncReplicas. NewSteps is how many inputs this
// frame recorded (the newest in the history), each one tick long. On a new
// snapshot the replica sits where the server had it, so every input the
// server has not applied is replayed; otherwise only the new ones are
// stepped. Blend (0..1) is how far the frame stands between the last tick
// and the next: the player is drawn that far from its second-newest step
// to its newest, so it moves every frame though it steps once a tick.
internal void
PredictLocalPlayer(app_state *AppState, memory_arena *Arena,
                   prediction_history *History, bool32 NewSnapshot,
                   u32 InputTick, u32 NewSteps, float Blend, float DeltaTime)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Player = Slot->Entity;
    bool32 Frozen = IsLocalPlayerTimeLocked(AppState);
    bool32 Moved = !Frozen;
    NewSteps = Minimum(NewSteps, History->Count);
    // NOTE(zoubir): where last frame's newest step put the player, and the
    // input that was, to compare with the replay of that same input
    bool32 HadShown = History->HasShown;
    v3 ShownBefore = History->Predicted;
    u32 ShownTick = History->ShownTick;
    v3 Before = History->Before;
    v3 BeforeVelocity = History->BeforeVelocity;
    v3 ReplayedAtShown = {};
    bool32 FoundShown = false;
    if (NewSnapshot)
    {
        DropAcknowledgedInputs(History, InputTick);
        if (Player && !Frozen)
        {
            RestorePredictedBody(Player, &History->Acked);
        }
        if (Player)
        {
            Before = Player->Position;
            BeforeVelocity = Player->Velocity;
            if (InputTick >= ShownTick)
            {
                ReplayedAtShown = Player->Position;
                FoundShown = true;
            }
        }
        // NOTE(zoubir): the steps shown on earlier frames make their sounds
        // and bursts again; only the new steps' are kept
        u32 EventsBefore = AppState->Events.Count;
        u32 FirstNew = History->Count - NewSteps;
        for(u32 Index = 0; Index < History->Count && Moved && Player; Index++)
        {
            predicted_input *Input = GetPredictedInput(History, Index);
            Before = Player->Position;
            BeforeVelocity = Player->Velocity;
            Moved = PredictLocalStep(AppState, Arena, Input);
            if (Index < FirstNew)
            {
                AppState->Events.Count = EventsBefore;
            }
            if (Input->Tick == ShownTick)
            {
                ReplayedAtShown = Player->Position;
                FoundShown = true;
            }
        }
    }
    else
    {
        if (!Frozen)
        {
            ReturnToPredicted(AppState, Arena, History, Player);
        }
        for(u32 Index = History->Count - NewSteps; Index < History->Count && Moved && Player && !Frozen; Index++)
        {
            Before = Player->Position;
            BeforeVelocity = Player->Velocity;
            Moved = PredictLocalStep(AppState, Arena, GetPredictedInput(History, Index));
        }
    }

    if (!Moved || !Player || !Player->IsPresent || IsDeadPlayer(Player))
    {
        History->HasShown = false;
        History->DrawError = {};
        return;
    }

    v3 Predicted = Player->Position;
    v3 PredictedVelocity = Player->Velocity;
    if (!HadShown)
    {
        Before = Predicted;
        BeforeVelocity = PredictedVelocity;
    }
    // NOTE(zoubir): the error is how far the replay of the input last shown
    // lands from where it was shown, not the newest step against the old
    // one; comparing those counted this frame's own walk as a correction
    if (NewSnapshot && HadShown && FoundShown)
    {
        v2 Correction = ShownBefore.XY - ReplayedAtShown.XY;
        History->DrawError += Correction;
        if (LengthSq(Correction) > Square(0.01f))
        {
            History->Corrections++;
            History->CorrectedDistance += Length(Correction);
        }
        if (LengthSq(History->DrawError) > Square(PREDICTION_SNAP_DISTANCE))
        {
            History->DrawError = {};
            Before = Predicted;
            BeforeVelocity = PredictedVelocity;
        }
    }
    History->DrawError *= expf(-PREDICTION_BLEND_RATE * DeltaTime);
    if (LengthSq(History->DrawError) < Square(0.01f))
    {
        History->DrawError = {};
    }
    // NOTE(zoubir): a step longer than a snap (a respawn, a blink) is not
    // blended across
    if (LengthSq(Predicted.XY - Before.XY) > Square(PREDICTION_SNAP_DISTANCE))
    {
        Before = Predicted;
        BeforeVelocity = PredictedVelocity;
    }
    v3 Shown = MotionCurve(Before, BeforeVelocity, Predicted, PredictedVelocity,
                           1.f / (float)NET_TICK_RATE, Blend);
    Shown.XY += History->DrawError;
    v3 OldPosition = Player->Position;
    Player->Position = Shown;
    CheckAndChangeEntityChunk(AppState, &AppState->World, Arena, OldPosition, Player);
    History->Predicted = Predicted;
    History->Before = Before;
    History->BeforeVelocity = BeforeVelocity;
    if (History->Count > 0)
    {
        History->ShownTick = GetPredictedInput(History, History->Count - 1)->Tick;
    }
    else
    {
        History->ShownTick = InputTick;
    }
    History->HasShown = true;
}
