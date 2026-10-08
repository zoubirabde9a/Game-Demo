/* Camera: the screen follows the local player, never showing past the
   edge of a bounded map (infinite maps have no edge; a map smaller than
   the screen sits in its middle). The result is the top-left corner of
   the screen in world units, snapped to whole window pixels.

   The world is drawn zoomed in: WorldZoom window pixels per world unit,
   chosen so the screen shows about WORLD_VIEW_HEIGHT units from top to
   bottom whatever the window size. GetWorldView gives the window measured
   in world units, which is what the camera, the ground and the world
   overlays work in; screens over the world stay in window pixels.

   The camera is a critically damped spring pulled toward its target: it
   speeds up and slows down without a jolt, never overshoots on its own,
   and moves the same at any frame rate. The target is the player plus a
   look offset (CameraLook) that eases on its own, slower clock: a lean
   toward the mouse, so more of the screen lies the way the player aims;
   a lead ahead of a moving player, so a run shows more of what it heads
   into; and, while the player is dead, a drift toward whoever killed it.
   Easing the offset means starting, stopping and flicking the mouse glide
   the view instead of yanking it.

   Two things take the camera off the player. While the duel's last
   death plays in slow motion, it centres on the falling body and pushes
   in (final_blow.cpp). While the local player lies dead in a dungeon, it
   flies where the movement keys send it (free_camera.cpp). Either way
   the spring still carries it there, and it comes back to the player
   when that ends.

   Near the edge of a bounded map the target does not stop dead at the
   limit: over the last CAMERA_EDGE_SOFT units it slows down evenly and
   comes to rest exactly on the edge (SoftClampAxis), so running into a
   wall eases the view in. The drawn view, shake included, never shows
   past the edge.

   A respawn or a jump across the map is a cut, not a long pan. Hits
   shake it (fx_bursts.cpp), and a solid hit the local player lands
   nudges it toward the blow (body_pose.cpp); both move only what is
   drawn. Online, the player's Position is already smooth (prediction
   handles corrections), so the camera adds no smoothing of its own for
   them.

   Called after the world has moved this frame, so the camera and the
   player it follows are drawn from the same positions. */

// NOTE(zoubir): world units shown from the top of the screen to the bottom
// (the window used to show its own height, 680 at the default size); the
// zoom never goes below 1 or above 3
#define WORLD_VIEW_HEIGHT 612.f
#define WORLD_ZOOM_MIN 1.f
#define WORLD_ZOOM_MAX 3.f
// NOTE(zoubir): the spring's smoothing time in seconds: it covers most of
// a gap in about twice this. A full run (260) trails the target by about
// 23 units, which the lead more than makes up
#define CAMERA_SMOOTH_SECONDS 0.09f
// NOTE(zoubir): share of the gap the look offset (lean, lead, killer
// drift) closes per second, as a rate: 6 settles in about half a second
#define CAMERA_LOOK_RATE 6.f
// NOTE(zoubir): the lean is this share of the cursor's distance from the
// screen centre, at most CAMERA_LEAN_MAX world units
#define CAMERA_LEAN_SHARE 0.12f
#define CAMERA_LEAN_MAX 50.f
// NOTE(zoubir): the camera aims this many seconds ahead of a moving
// player, at most CAMERA_LEAD_MAX units: a full run (260) leads by 31.
// Dashes hit the cap
#define CAMERA_LEAD_SECONDS 0.12f
#define CAMERA_LEAD_MAX 40.f
// NOTE(zoubir): while the local player is dead the camera drifts this
// share of the way toward the player who killed it, at most
// CAMERA_KILLER_MAX units, so the killer is in view for the countdown
#define CAMERA_KILLER_SHARE 0.5f
#define CAMERA_KILLER_MAX 220.f
// NOTE(zoubir): the target starts slowing this many units before a map
// edge and rests on the edge this many units past it (at most a quarter
// of the room the camera has on that axis)
#define CAMERA_EDGE_SOFT 64.f
// NOTE(zoubir): a target farther than this many screen sizes away is cut to
#define CAMERA_CUT_SCREENS 0.75f
// NOTE(zoubir): longest frame the spring steps in one go, so a hitch does
// not throw the view
#define CAMERA_MAX_STEP 0.1f

// NOTE(zoubir): sets AppState->WorldZoom for this frame and returns the
// window measured in world units
internal app_window
GetWorldView(app_state *AppState, app_window *Window)
{
    float Zoom = (float)Window->Height / WORLD_VIEW_HEIGHT;
    Zoom = Maximum(WORLD_ZOOM_MIN, Minimum(WORLD_ZOOM_MAX, Zoom));
    // NOTE(zoubir): the final blow's push-in goes past the usual limit
    Zoom *= FinalBlowZoom(AppState);
    AppState->WorldZoom = Zoom;
    app_window Result = *Window;
    Result.Width = (u32)ceilf((float)Window->Width / Zoom);
    Result.Height = (u32)ceilf((float)Window->Height / Zoom);
    return Result;
}

// NOTE(zoubir): where the camera's top-left corner may go. On an infinite
// map Bounded is false and Min/Max mean nothing. A map narrower than the
// view has Min == Max, the corner that centres it
struct camera_bounds
{
    bool32 Bounded;
    v2 Min;
    v2 Max;
};

inline void
CameraBoundsAxis(float MapSize, float ViewSize, float *Min, float *Max)
{
    if (ViewSize >= MapSize)
    {
        *Min = *Max = -0.5f * (ViewSize - MapSize);
    }
    else
    {
        *Min = 0.f;
        *Max = MapSize - ViewSize;
    }
}

internal camera_bounds
GetCameraBounds(world *World, app_window *View)
{
    camera_bounds Result = {};
    Result.Bounded = !World->Unbounded;
    if (Result.Bounded)
    {
        float MapWidth = (float)(World->NumTilesX * World->TileWidth);
        float MapHeight = (float)(World->NumTilesY * World->TileHeight);
        CameraBoundsAxis(MapWidth, (float)View->Width, &Result.Min.X, &Result.Max.X);
        CameraBoundsAxis(MapHeight, (float)View->Height, &Result.Min.Y, &Result.Max.Y);
    }
    return Result;
}

// NOTE(zoubir): Position kept inside Min..Max, but eased: it follows
// Position exactly until CAMERA_EDGE_SOFT units from a limit, then slows
// evenly (a parabola, so its speed has no corner) and rests on the limit
// once Position is CAMERA_EDGE_SOFT units past it
inline float
SoftClampAxis(float Position, float Min, float Max)
{
    if (Max <= Min)
    {
        return Min;
    }
    float Soft = Minimum(CAMERA_EDGE_SOFT, 0.25f * (Max - Min));
    float Result = Position;
    if (Position > Max - Soft)
    {
        float Past = Position - (Max - Soft);
        Result = Past >= 2.f * Soft ? Max :
            Max - Soft + Past - Past * Past / (4.f * Soft);
    }
    else if (Position < Min + Soft)
    {
        float Past = (Min + Soft) - Position;
        Result = Past >= 2.f * Soft ? Min :
            Min + Soft - Past + Past * Past / (4.f * Soft);
    }
    return Result;
}

inline v2
SoftClampCamera(camera_bounds *Bounds, v2 Corner)
{
    v2 Result = Corner;
    if (Bounds->Bounded)
    {
        Result.X = SoftClampAxis(Corner.X, Bounds->Min.X, Bounds->Max.X);
        Result.Y = SoftClampAxis(Corner.Y, Bounds->Min.Y, Bounds->Max.Y);
    }
    return Result;
}

// NOTE(zoubir): Corner kept inside the bounds with a hard stop; Velocity,
// if given, loses the part that pushed past. An axis where the map is
// narrower than the view is left alone: its border shows anyway, and
// shake there should still show
inline v2
HardClampCamera(camera_bounds *Bounds, v2 Corner, v2 *Velocity = 0)
{
    v2 Result = Corner;
    for(u32 Axis = 0; Bounds->Bounded && Axis < 2; Axis++)
    {
        float Min = Bounds->Min.Data[Axis];
        float Max = Bounds->Max.Data[Axis];
        if (Max <= Min)
        {
            continue;
        }
        float Clamped = Maximum(Min, Minimum(Result.Data[Axis], Max));
        if (Velocity && Clamped != Result.Data[Axis])
        {
            Velocity->Data[Axis] = 0.f;
        }
        Result.Data[Axis] = Clamped;
    }
    return Result;
}

// NOTE(zoubir): one step of a critically damped spring of smoothing time
// CAMERA_SMOOTH_SECONDS (the closed form from Game Programming Gems 4,
// "Critically Damped Ease-In/Ease-Out Smoothing"): stable at any step
inline float
SpringAxis(float Current, float Target, float *Velocity, float DeltaTime)
{
    float Omega = 2.f / CAMERA_SMOOTH_SECONDS;
    float X = Omega * DeltaTime;
    float Decay = 1.f / (1.f + X + 0.48f * X * X + 0.235f * X * X * X);
    float Change = Current - Target;
    float Temp = (*Velocity + Omega * Change) * DeltaTime;
    *Velocity = (*Velocity - Omega * Temp) * Decay;
    float Result = Target + (Change + Temp) * Decay;
    return Result;
}

inline v2
CapLength(v2 Value, float Max)
{
    v2 Result = Value;
    float Size = Length(Value);
    if (Size > Max)
    {
        Result = (Max / Size) * Value;
    }
    return Result;
}

// NOTE(zoubir): the push toward the cursor, in world units. View is the
// window in world units; the mouse is in window pixels
internal v2
CameraLean(app_input *Input, app_window *View, float Zoom)
{
    v2 Mouse = V2((float)Input->MouseX / Zoom, (float)Input->MouseY / Zoom);
    v2 FromCentre = V2(Mouse.X - 0.5f * (float)View->Width,
                       Mouse.Y - 0.5f * (float)View->Height);
    // NOTE(zoubir): a cursor outside the window (or not moved yet) pulls nothing
    if (Mouse.X < 0.f || Mouse.Y < 0.f ||
        Mouse.X > (float)View->Width || Mouse.Y > (float)View->Height)
    {
        FromCentre = V2(0.f, 0.f);
    }
    v2 Result = CapLength(CAMERA_LEAN_SHARE * FromCentre, CAMERA_LEAN_MAX);
    return Result;
}

// NOTE(zoubir): the live player who landed the local player's latest
// death (client/kill_feed.cpp), or 0 for a monster, the world or none
internal world_entity *
LocalKiller(app_state *AppState)
{
    kill_feed *Feed = AppState->KillFeed;
    for(u32 Index = 0; Feed && Index < Feed->Count; Index++)
    {
        kill_feed_entry *Entry = &Feed->Entries[Index];
        if (Entry->Victim != AppState->LocalPlayerIndex)
        {
            continue;
        }
        if (Entry->Killer >= MAX_PLAYERS || Entry->Killer == AppState->LocalPlayerIndex)
        {
            return 0;
        }
        world_entity *Killer = AppState->Players[Entry->Killer].Entity;
        bool32 Alive = Killer && Killer->IsPresent && !IsDeadPlayer(Killer);
        return Alive ? Killer : 0;
    }
    return 0;
}

// NOTE(zoubir): where the look offset is heading this frame: lean and
// lead while alive, the drift toward the killer while dead. Lean is off
// while a screen holds the mouse
internal v2
CameraLookTarget(app_state *AppState, world_entity *Player, app_window *View,
                 app_input *Input, bool32 Lean, float Zoom)
{
    v2 Result = {};
    if (IsDeadPlayer(Player))
    {
        world_entity *Killer = LocalKiller(AppState);
        if (Killer)
        {
            Result = CapLength(CAMERA_KILLER_SHARE * (Killer->Position.XY - Player->Position.XY),
                               CAMERA_KILLER_MAX);
        }
    }
    else
    {
        if (Lean)
        {
            Result += CameraLean(Input, View, Zoom);
        }
        Result += CapLength(CAMERA_LEAD_SECONDS * Player->Velocity.XY, CAMERA_LEAD_MAX);
    }
    return Result;
}

// NOTE(zoubir): keeps heading for the previous target when there is no
// local player. Window is the view in world units (GetWorldView). Lean
// is false while a screen has the keyboard and mouse
internal v3
UpdateCamera(app_state *AppState, app_window *Window, app_input *Input,
             bool32 Lean)
{
    world_entity *Player = GetLocalPlayer(AppState);
    float Zoom = AppState->WorldZoom > 0.f ? AppState->WorldZoom : 1.f;
    float DeltaTime = Maximum(0.f, Minimum(Input->DeltaTime, CAMERA_MAX_STEP));
    camera_bounds Bounds = GetCameraBounds(&AppState->World, Window);
    v2 HalfView = V2(0.5f * (float)Window->Width, 0.5f * (float)Window->Height);
    v3 *Camera = &AppState->CameraOffset;

    bool32 Cut = !AppState->CameraPlaced;
    if (Player)
    {
        v2 LookTarget = CameraLookTarget(AppState, Player, Window, Input, Lean, Zoom);
        // NOTE(zoubir): what the view centres on: the player, or the duel's
        // falling body, or a dead player's loose camera in a dungeon
        v2 Anchor = Player->Position.XY;
        v2 Focus;
        if (FinalBlowFocus(AppState, &Focus) ||
            FreeCameraFocus(AppState, Input, Player, Lean, &Focus))
        {
            Anchor = Focus;
            LookTarget = V2(0.f, 0.f);
        }
        // NOTE(zoubir): judged on where the camera would go with the look
        // already there, so a respawn far away cuts straight to it
        v2 Settled = SoftClampCamera(&Bounds, Anchor + LookTarget - HalfView);
        float CutDistance = CAMERA_CUT_SCREENS * (float)Maximum(Window->Width, Window->Height);
        if (Length(Settled - Camera->XY) > CutDistance)
        {
            Cut = true;
        }
        if (Cut)
        {
            AppState->CameraLook = LookTarget;
        }
        else
        {
            float Share = 1.f - expf(-CAMERA_LOOK_RATE * DeltaTime);
            AppState->CameraLook += Share * (LookTarget - AppState->CameraLook);
        }
        v2 Corner = SoftClampCamera(&Bounds, Anchor + AppState->CameraLook - HalfView);
        AppState->TargetCamera = V3(Corner.X, Corner.Y, Player->Position.Z);
    }

    if (Cut)
    {
        *Camera = AppState->TargetCamera;
        AppState->CameraVelocity = V2(0.f, 0.f);
        AppState->CameraPlaced = true;
    }
    else
    {
        v2 *Velocity = &AppState->CameraVelocity;
        Camera->X = SpringAxis(Camera->X, AppState->TargetCamera.X, &Velocity->X, DeltaTime);
        Camera->Y = SpringAxis(Camera->Y, AppState->TargetCamera.Y, &Velocity->Y, DeltaTime);
        Camera->Z = AppState->TargetCamera.Z;
        Camera->XY = HardClampCamera(&Bounds, Camera->XY, Velocity);
    }

    v3 CameraOffset = *Camera;
    // NOTE(zoubir): hits shake the screen (fx_bursts.cpp); added only to
    // what is drawn, never to where the camera is heading, and kept off
    // the dark past the map's edge
    CameraOffset.XY += GetCameraShake(AppState) + GetHitNudge(AppState);
    CameraOffset.XY = HardClampCamera(&Bounds, CameraOffset.XY);
    // NOTE(zoubir): to whole window pixels, rounding down so negative
    // positions snap the same way as positive ones
    CameraOffset.X = SnapToScreenPixel(CameraOffset.X, Zoom);
    CameraOffset.Y = SnapToScreenPixel(CameraOffset.Y, Zoom);
    return CameraOffset;
}
