/* Click to move: in the mouse-moves scheme (control_scheme.cpp) a left
   click on the ground walks the local player there, and holding the
   button keeps walking toward the cursor. A click that confirms an aimed
   spell, or lands on a screen, walks nowhere (cast_targeting.cpp decides
   that, FreeClick).

   The walk becomes the same four movement directions the keys give
   (MoveToward), so the simulation, the server and prediction need
   nothing new: offline it is player_input.Move, online the movement
   bits of the held buttons (MoveNetButtons, sent from app.cpp). Eight
   ways only, so a long walk off those lines bends toward the spot as the
   angle changes, the way the bots walk (server/bots.cpp).

   The walk ends at the spot, when the player stops getting closer (a
   wall in the way), on death, or when the scheme changes. */

// NOTE(zoubir): close enough to stop pushing; the player skids about 9
// units after letting go (sim/player_stats.cpp), so it ends near the spot
#define CLICK_MOVE_ARRIVE 8.f
// NOTE(zoubir): a walk that gets no closer than this in CLICK_MOVE_STUCK_SECONDS
// is against something and ends
#define CLICK_MOVE_PROGRESS 2.f
#define CLICK_MOVE_STUCK_SECONDS 0.4f
// NOTE(zoubir): a direction component past this holds that key (sin of
// 22.5 degrees, so each of the 8 ways covers 45 degrees)
#define CLICK_MOVE_AXIS 0.38f

struct click_move
{
    bool32 Walking;
    // NOTE(zoubir): the button that started the walk is still down, so
    // the spot follows the cursor
    bool32 Holding;
    v2 Goal;
    // NOTE(zoubir): the nearest the walk has come, and seconds since it
    // last came CLICK_MOVE_PROGRESS closer
    float Closest;
    float SinceProgress;
};

// NOTE(zoubir): made on first use, in MemoryArena so a map switch keeps it
internal click_move *
GetClickMove(app_state *AppState)
{
    if (!AppState->ClickMove)
    {
        AppState->ClickMove = AllocateStruct(&AppState->MemoryArena, click_move);
        *AppState->ClickMove = {};
    }
    return AppState->ClickMove;
}

// NOTE(zoubir): the keys' Move (each of X and Y -1, 0 or 1) that walks
// along Direction
inline v2
MoveToward(v2 Direction)
{
    v2 Result = {};
    float Length = SquareRoot(LengthSq(Direction));
    if (Length > 0.f)
    {
        v2 Unit = Direction * (1.f / Length);
        if (Unit.X > CLICK_MOVE_AXIS) Result.X = 1.f;
        if (Unit.X < -CLICK_MOVE_AXIS) Result.X = -1.f;
        if (Unit.Y > CLICK_MOVE_AXIS) Result.Y = 1.f;
        if (Unit.Y < -CLICK_MOVE_AXIS) Result.Y = -1.f;
    }
    return Result;
}

// NOTE(zoubir): the network's movement bits for a keys' Move
inline u32
MoveNetButtons(v2 Move)
{
    u32 Result = 0;
    if (Move.X < 0.f) Result |= NetButton_Left;
    if (Move.X > 0.f) Result |= NetButton_Right;
    if (Move.Y < 0.f) Result |= NetButton_Up;
    if (Move.Y > 0.f) Result |= NetButton_Down;
    return Result;
}

// NOTE(zoubir): once a frame, after cast targeting has looked at the
// click: starts, steers and ends the walk; returns this frame's Move
internal v2
UpdateClickMove(app_input *Input, app_state *AppState)
{
    click_move *Walk = GetClickMove(AppState);
    world_entity *Player = GetLocalPlayer(AppState);
    bool32 Alive = Player && Player->IsPresent && !IsDeadPlayer(Player);
    if (!MouseMoves() || !Alive)
    {
        *Walk = {};
        return V2(0.f, 0.f);
    }
    if (GetCastTargeting(AppState)->FreeClick)
    {
        Walk->Walking = true;
        Walk->Holding = true;
        Walk->Closest = 1e9f;
        Walk->SinceProgress = 0.f;
    }
    Walk->Holding = Walk->Holding && Input->LeftButton.EndedDown;
    if (Walk->Holding)
    {
        Walk->Goal = CursorInWorld(Input, AppState);
    }

    v2 Result = {};
    if (Walk->Walking)
    {
        v2 ToGoal = Walk->Goal - Player->Position.XY;
        float Distance = SquareRoot(LengthSq(ToGoal));
        // NOTE(zoubir): while held the spot moves with the cursor, so
        // progress counts from the release
        if (Walk->Holding || Distance < Walk->Closest - CLICK_MOVE_PROGRESS)
        {
            Walk->Closest = Distance;
            Walk->SinceProgress = 0.f;
        }
        else
        {
            Walk->SinceProgress += Input->DeltaTime;
        }
        // NOTE(zoubir): held on the player itself, it waits rather than ends
        bool32 Arrived = Distance < CLICK_MOVE_ARRIVE;
        bool32 Stuck = Walk->SinceProgress > CLICK_MOVE_STUCK_SECONDS;
        if ((Arrived && !Walk->Holding) || Stuck)
        {
            Walk->Walking = false;
        }
        else if (!Arrived)
        {
            Result = MoveToward(ToGoal);
        }
    }
    return Result;
}
