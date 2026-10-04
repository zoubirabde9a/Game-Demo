/* Local controls: turns this machine's keyboard and mouse into the
   player_input the simulation understands. ZQSD move (AZERTY layout),
   the mouse cursor aims, right click sword, left click fireball, Space
   jump, Alt dash, E shockwave, F blink, R push, A launch (the action keys
   are one table, action_keys.cpp). Holding Tab shows the scoreboard
   (read in app.cpp, it is not a player action).

   Click to move: a left click on the ground, rather than on a monster or
   another player, walks the player to that spot instead of casting, and
   holding the button steers toward the cursor. It drives the same Move
   the keys do (online, the same direction buttons), so the simulation and
   the server see nothing new. A movement key or a blocked path cancels it. */

// NOTE(zoubir): close enough to the clicked spot to stop, in world units
#define CLICK_MOVE_ARRIVED 6.f
// NOTE(zoubir): seconds without getting closer before giving up (a wall)
#define CLICK_MOVE_GIVE_UP 0.5f
// NOTE(zoubir): tan(22.5 degrees): an axis is pressed when the spot lies
// within the 45 degree slice around that direction or a diagonal
#define CLICK_MOVE_DIAGONAL 0.4142f

struct click_move
{
    bool32 Active;
    // NOTE(zoubir): the current left button press began on the ground, so
    // it walks and must not cast until it is released
    bool32 OwnsLeftButton;
    v2 Target;
    float BestDistance;
    float StuckTime;
    float Age;
};

// NOTE(zoubir): client-only; a hot reload of app.dll just drops the walk
global_variable click_move ClickMove;

// NOTE(zoubir): the cursor on the ground, in world units; the camera is
// last frame's, which is what is on screen
inline v2
CursorInWorld(app_input *Input, app_state *AppState)
{
    v2 Result = V2((float)Input->MouseX + floorf(AppState->CameraOffset.X),
                   (float)Input->MouseY + floorf(AppState->CameraOffset.Y));
    return Result;
}

// NOTE(zoubir): a live monster or other player whose sprite is under
// Cursor. Sprites stand on their feet (Position), so the box reaches up.
internal bool32
IsUnitUnderCursor(app_state *AppState, v2 Cursor, world_entity *Self)
{
    world *World = &AppState->World;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        bool32 IsTarget = Entity->IsPresent && Entity != Self &&
            ((Entity->Type == EntityType_Monster && Entity->Hp > 0.f) ||
             (Entity->Type == EntityType_Player && !IsDeadPlayer(Entity)));
        if (IsTarget)
        {
            v2 Feet = V2(Entity->Position.X, Entity->Position.Y - Entity->Position.Z);
            float HalfWidth = 0.4f * Entity->Dimensions.X;
            if (Cursor.X > Feet.X - HalfWidth && Cursor.X < Feet.X + HalfWidth &&
                Cursor.Y > Feet.Y - 0.9f * Entity->Dimensions.Y &&
                Cursor.Y < Feet.Y + 0.2f * Entity->Dimensions.Y)
            {
                return true;
            }
        }
    }
    return false;
}

inline float
ClickMoveAxis(float Along, float Across)
{
    float Result = 0.f;
    if (fabsf(Along) > CLICK_MOVE_DIAGONAL * fabsf(Across))
    {
        Result = Along > 0.f ? 1.f : -1.f;
    }
    return Result;
}

// NOTE(zoubir): starts, steers or ends the walk to a clicked spot and
// writes its direction into Move while it lasts. True while the left
// button belongs to walking, so it must not cast.
internal bool32
UpdateClickMove(app_input *Input, app_state *AppState, bool32 KeysMove, v2 *Move)
{
    click_move *Walk = &ClickMove;
    world_entity *Player = GetLocalPlayer(AppState);
    bool32 CanWalk = Player && !IsDeadPlayer(Player);
    v2 Cursor = CursorInWorld(Input, AppState);
    if (Input->LeftButton.Pressed)
    {
        Walk->OwnsLeftButton = CanWalk && !IsUnitUnderCursor(AppState, Cursor, Player);
        if (Walk->OwnsLeftButton)
        {
            Walk->Active = true;
            Walk->Age = 0.f;
        }
    }
    else if (!Input->LeftButton.EndedDown)
    {
        Walk->OwnsLeftButton = false;
    }
    if (!CanWalk || KeysMove)
    {
        Walk->Active = false;
    }

    if (Walk->Active)
    {
        if (Walk->OwnsLeftButton && Input->LeftButton.EndedDown)
        {
            Walk->Target = Cursor;
            Walk->BestDistance = 1e30f;
            Walk->StuckTime = 0.f;
        }
        Walk->Age += Input->DeltaTime;
        v2 ToTarget = Walk->Target - Player->Position.XY;
        float Distance = Length(ToTarget);
        if (Distance < Walk->BestDistance - 0.5f)
        {
            Walk->BestDistance = Distance;
            Walk->StuckTime = 0.f;
        }
        else
        {
            Walk->StuckTime += Input->DeltaTime;
        }
        if (Distance < CLICK_MOVE_ARRIVED || Walk->StuckTime > CLICK_MOVE_GIVE_UP)
        {
            Walk->Active = false;
        }
        else
        {
            Move->X = ClickMoveAxis(ToTarget.X, ToTarget.Y);
            Move->Y = ClickMoveAxis(ToTarget.Y, ToTarget.X);
        }
    }
    return Walk->OwnsLeftButton;
}

// NOTE(zoubir): the input the online session sends: a walk to a clicked
// spot shows up as held direction keys, and a left button that walks is
// not held, so the server neither casts nor needs to know about walking
internal app_input
InputForServer(app_input *Input, player_input *LocalInput, bool32 KeysToUi)
{
    app_input Result = *Input;
    if (KeysToUi)
    {
        ClickMove = {};
    }
    if (ClickMove.OwnsLeftButton)
    {
        Result.LeftButton = {};
    }
    if (ClickMove.Active)
    {
        Result.ButtonQ.EndedDown = LocalInput->Move.X < 0.f;
        Result.ButtonD.EndedDown = LocalInput->Move.X > 0.f;
        Result.ButtonZ.EndedDown = LocalInput->Move.Y < 0.f;
        Result.ButtonS.EndedDown = LocalInput->Move.Y > 0.f;
    }
    return Result;
}

// NOTE(zoubir): a small ring on the spot the player walks to, shrinking
// in as it appears and then breathing
internal void
DrawClickMoveMarker(render_context *RenderContext, v3 CameraOffset)
{
    if (ClickMove.Active)
    {
        v2 Spot = ClickMove.Target - CameraOffset.XY;
        float Radius = 10.f - 4.f * Minimum(ClickMove.Age * 4.f, 1.f) +
            1.5f * Sin(6.f * ClickMove.Age);
        DrawGroundRing(RenderContext, Spot, Radius, 0xC0E8F4FF, 3.f);
    }
}

// NOTE(zoubir): from the local player to the cursor, in world units, over
// PLAYER_AIM_REACH and capped at length 1; the camera is last frame's,
// which is what is on screen. Zero when there is no local player or the
// cursor sits on it.
internal v2
AimFromCursor(app_input *Input, app_state *AppState)
{
    v2 Result = {};
    world_entity *Player = GetLocalPlayer(AppState);
    if (Player)
    {
        v2 Cursor = CursorInWorld(Input, AppState);
        v2 ToCursor = Cursor - Player->Position.XY;
        float LengthSquared = LengthSq(ToCursor);
        if (LengthSquared > 4.f)
        {
            float Length = SquareRoot(LengthSquared);
            Result = ToCursor * (Minimum(Length, PLAYER_AIM_REACH) /
                                 (Length * PLAYER_AIM_REACH));
        }
    }
    return Result;
}

internal player_input
ReadKeyboardPlayerInput(app_input *Input, app_state *AppState)
{
    player_input Result = {};
    Result.Aim = AimFromCursor(Input, AppState);
    if (Input->ButtonZ.EndedDown) { Result.Move.Y = -1.f; }
    if (Input->ButtonS.EndedDown) { Result.Move.Y = 1.f; }
    if (Input->ButtonD.EndedDown) { Result.Move.X = 1.f; }
    if (Input->ButtonQ.EndedDown) { Result.Move.X = -1.f; }
    bool32 KeysMove = Result.Move.X != 0.f || Result.Move.Y != 0.f;

    Result.Pressed = ActionButtonsFromKeys(Input, true);
    if (UpdateClickMove(Input, AppState, KeysMove, &Result.Move))
    {
        Result.Pressed &= ~(u32)PlayerButton_Cast;
    }
    return Result;
}
