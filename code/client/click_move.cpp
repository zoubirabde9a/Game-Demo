/* Click to move: a left click on the ground, rather than on a monster or
   another player, walks the local player to that spot instead of casting,
   and holding the button steers toward the cursor. Shift+click casts at
   the ground as before. A ring marks the spot; a red ring under a foe
   shows that a click there casts instead.

   The walk only fills in the Move the ZQSD keys would (online, the same
   held direction buttons, InputForServer), so the simulation and the
   server see nothing new. A movement key, death, an open screen or a
   blocked path ends it. While the tile editor (F3) is open the left
   button paints tiles and does neither. */

// NOTE(zoubir): close enough to the clicked spot to stop, in world units
#define CLICK_MOVE_ARRIVED 6.f
// NOTE(zoubir): seconds without getting closer before giving up (a wall)
#define CLICK_MOVE_GIVE_UP 0.5f
// NOTE(zoubir): tan(22.5 degrees): an axis is pressed when the spot lies
// within the 45 degree slice around that direction or a diagonal
#define CLICK_MOVE_DIAGONAL 0.4142f
#define CLICK_MOVE_SPOT_COLOR 0xC0E8F4FF
#define CLICK_MOVE_FOE_COLOR 0xD03040FF

struct click_move
{
    bool32 Active;
    // NOTE(zoubir): the current left button press began on the ground (it
    // walks) or in the tile editor (it paints), so it must not cast until
    // it is released
    bool32 OwnsLeftButton;
    v2 Target;
    float BestDistance;
    float StuckTime;
    float Age;
    // NOTE(zoubir): the foe a left click would cast at, for its ring
    world_entity *Foe;
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

inline bool32
IsLiveFoe(world_entity *Entity, world_entity *Self)
{
    bool32 Result = Entity->IsPresent && Entity != Self &&
        ((Entity->Type == EntityType_Monster && Entity->Hp > 0.f) ||
         (Entity->Type == EntityType_Player && !IsDeadPlayer(Entity)));
    return Result;
}

// NOTE(zoubir): the live monster or other player whose sprite is under
// Cursor, or 0. Sprites stand on their feet (Position), so the box reaches
// up; the nearest feet win when sprites overlap.
internal world_entity *
FoeUnderCursor(app_state *AppState, v2 Cursor, world_entity *Self)
{
    world *World = &AppState->World;
    world_entity *Result = 0;
    float BestDistance = 1e30f;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Entity = &World->Entities[EntityIndex];
        if (IsLiveFoe(Entity, Self))
        {
            v2 Feet = V2(Entity->Position.X, Entity->Position.Y - Entity->Position.Z);
            float HalfWidth = 0.4f * Entity->Dimensions.X;
            float Distance = Length(Cursor - Feet);
            if (Cursor.X > Feet.X - HalfWidth && Cursor.X < Feet.X + HalfWidth &&
                Cursor.Y > Feet.Y - 0.9f * Entity->Dimensions.Y &&
                Cursor.Y < Feet.Y + 0.2f * Entity->Dimensions.Y &&
                Distance < BestDistance)
            {
                BestDistance = Distance;
                Result = Entity;
            }
        }
    }
    return Result;
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
// button must not cast.
internal bool32
UpdateClickMove(app_input *Input, app_state *AppState, bool32 KeysMove, v2 *Move)
{
    click_move *Walk = &ClickMove;
    world_entity *Player = GetLocalPlayer(AppState);
    bool32 CanWalk = Player && !IsDeadPlayer(Player) && !AppState->TileEditing;
    bool32 ForceCast = Input->ShiftButton.EndedDown;
    v2 Cursor = CursorInWorld(Input, AppState);
    world_entity *Foe = (CanWalk && !ForceCast) ?
        FoeUnderCursor(AppState, Cursor, Player) : 0;
    Walk->Foe = Foe;

    if (Input->LeftButton.Pressed)
    {
        Walk->OwnsLeftButton = AppState->TileEditing ||
            (CanWalk && !ForceCast && !Foe);
        if (Walk->OwnsLeftButton && CanWalk)
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

// NOTE(zoubir): a ring on the spot the player walks to, shrinking in as
// it appears and then breathing, and a red ring under the foe a click
// would cast at
internal void
DrawClickMoveMarker(render_context *RenderContext, v3 CameraOffset)
{
    if (ClickMove.Active)
    {
        v2 Spot = ClickMove.Target - CameraOffset.XY;
        float Radius = 10.f - 4.f * Minimum(ClickMove.Age * 4.f, 1.f) +
            1.5f * Sin(6.f * ClickMove.Age);
        DrawGroundRing(RenderContext, Spot, Radius, CLICK_MOVE_SPOT_COLOR, 3.f);
    }
    world_entity *Foe = ClickMove.Foe;
    if (Foe && IsLiveFoe(Foe, 0))
    {
        v2 Feet = Foe->Position.XY - CameraOffset.XY;
        DrawGroundRing(RenderContext, Feet, 0.45f * Foe->Dimensions.X,
                       CLICK_MOVE_FOE_COLOR, 3.f);
    }
}
