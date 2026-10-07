/* Final blow: the death that ends a duel round (sim/round_break.cpp)
   plays for FINAL_BLOW_SECONDS as a short film while the simulation runs
   in slow motion. The camera pushes in on the body and holds it in the
   middle of the screen (FinalBlowFocus and FinalBlowZoom, read by
   camera.cpp). The body, which a dead player otherwise is not drawn as,
   flashes white, then topples over away from the killer and lands with a
   small bounce (FinalBlowPose, read by body_pose.cpp; draw_entities.cpp
   keeps drawing it). The frame's effects (bursts, hit numbers, poses, the
   camera spring) run on the same slowed clock: app.cpp scales the frame's
   DeltaTime by RoundTimeScale once the world has ticked. The bars and the
   title over the screen are ui/final_blow_view.cpp.

   Everything here reads the round break (which every snapshot carries)
   and the kill feed, so it plays the same offline and online. */

// NOTE(zoubir): the push-in adds this share to the zoom at its closest;
// it takes FINAL_BLOW_PUSH_SECONDS to get there and lets go as the slow
// motion eases out
#define FINAL_BLOW_ZOOM 0.6f
#define FINAL_BLOW_PUSH_SECONDS 0.6f
// NOTE(zoubir): the body takes this long to hit the floor, then bounces
// up by FINAL_BLOW_BOUNCE radians for FINAL_BLOW_BOUNCE_SECONDS
#define FINAL_BLOW_TOPPLE_SECONDS 0.9f
#define FINAL_BLOW_BOUNCE 0.18f
#define FINAL_BLOW_BOUNCE_SECONDS 0.35f
// NOTE(zoubir): the camera holds a point this share of the way from the
// body to the killer, at most FINAL_BLOW_KILLER_MAX units, so both show
#define FINAL_BLOW_KILLER_SHARE 0.3f
#define FINAL_BLOW_KILLER_MAX 70.f

// NOTE(zoubir): 0 to 1 with no jolt at either end
inline float
SmoothStep01(float Value)
{
    float T = Minimum(1.f, Maximum(0.f, Value));
    float Result = T * T * (3.f - 2.f * T);
    return Result;
}

// NOTE(zoubir): seconds since the final blow landed
inline float
FinalBlowElapsed(app_state *AppState)
{
    float Result = FINAL_BLOW_SECONDS - FinalBlowLeft(AppState);
    return Result;
}

// NOTE(zoubir): the kill feed's newest line while the final blow plays,
// or 0. The kill that ended the round is the newest one
internal kill_feed_entry *
FinalBlowKill(app_state *AppState)
{
    kill_feed *Feed = AppState->KillFeed;
    if (FinalBlowLeft(AppState) <= 0.f || !Feed || !Feed->Count ||
        Feed->Entries[0].Victim >= MAX_PLAYERS)
    {
        return 0;
    }
    return &Feed->Entries[0];
}

// NOTE(zoubir): the body that fell, while the final blow plays, or 0
internal world_entity *
FinalBlowVictim(app_state *AppState)
{
    kill_feed_entry *Kill = FinalBlowKill(AppState);
    world_entity *Body = Kill ? AppState->Players[Kill->Victim].Entity : 0;
    bool32 Shown = Body && Body->IsPresent && IsDeadPlayer(Body);
    return Shown ? Body : 0;
}

// NOTE(zoubir): the player who landed the final blow, standing, or 0
internal world_entity *
FinalBlowKiller(app_state *AppState)
{
    kill_feed_entry *Kill = FinalBlowKill(AppState);
    if (!Kill || Kill->Killer >= MAX_PLAYERS || Kill->Killer == Kill->Victim)
    {
        return 0;
    }
    world_entity *Killer = AppState->Players[Kill->Killer].Entity;
    bool32 Standing = Killer && Killer->IsPresent && !IsDeadPlayer(Killer);
    return Standing ? Killer : 0;
}

// NOTE(zoubir): how far the push-in has gone, 0 to 1
inline float
FinalBlowPush(app_state *AppState)
{
    float Left = FinalBlowLeft(AppState);
    if (Left <= 0.f)
    {
        return 0.f;
    }
    float In = SmoothStep01(FinalBlowElapsed(AppState) / FINAL_BLOW_PUSH_SECONDS);
    float Out = SmoothStep01(Left / FINAL_BLOW_EASE_OUT);
    float Result = Minimum(In, Out);
    return Result;
}

// NOTE(zoubir): what the world zoom is multiplied by this frame
inline float
FinalBlowZoom(app_state *AppState)
{
    float Result = FinalBlowVictim(AppState) ?
        1.f + FINAL_BLOW_ZOOM * FinalBlowPush(AppState) : 1.f;
    return Result;
}

// NOTE(zoubir): where the camera centres while the final blow plays;
// false when none plays
internal bool32
FinalBlowFocus(app_state *AppState, v2 *Focus)
{
    world_entity *Body = FinalBlowVictim(AppState);
    if (!Body)
    {
        return false;
    }
    *Focus = Body->Position.XY;
    world_entity *Killer = FinalBlowKiller(AppState);
    if (Killer)
    {
        v2 Toward = FINAL_BLOW_KILLER_SHARE * (Killer->Position.XY - Body->Position.XY);
        float Size = Length(Toward);
        if (Size > FINAL_BLOW_KILLER_MAX)
        {
            Toward = (FINAL_BLOW_KILLER_MAX / Size) * Toward;
        }
        *Focus += Toward;
    }
    return true;
}

// NOTE(zoubir): the falling body's pose, in place of the one body_pose.cpp
// keeps; false for every other unit
internal bool32
FinalBlowPose(app_state *AppState, world_entity *Entity, body_pose_draw *Pose)
{
    if (Entity->Type != EntityType_Player || Entity != FinalBlowVictim(AppState))
    {
        return false;
    }
    // NOTE(zoubir): it falls away from the killer, else backward from
    // where it aimed
    float Side = Entity->Aim.X > 0.f ? -1.f : 1.f;
    world_entity *Killer = FinalBlowKiller(AppState);
    if (Killer && Killer->Position.X != Entity->Position.X)
    {
        Side = Entity->Position.X > Killer->Position.X ? 1.f : -1.f;
    }
    float Elapsed = FinalBlowElapsed(AppState);
    // NOTE(zoubir): like a felled tree: slow to start, fast at the end
    float Fall = Minimum(1.f, Elapsed / FINAL_BLOW_TOPPLE_SECONDS);
    float Angle = 0.5f * Pi32 * Fall * Fall;
    float Bounced = (Elapsed - FINAL_BLOW_TOPPLE_SECONDS) / FINAL_BLOW_BOUNCE_SECONDS;
    if (Bounced > 0.f && Bounced < 1.f)
    {
        Angle -= FINAL_BLOW_BOUNCE * Sin(Pi32 * Bounced);
    }
    Pose->Scale = V2(1.f, 1.f);
    Pose->Angle = Side * Angle;
    Pose->AboutFeet = true;
    // NOTE(zoubir): white for the blow itself, then drawn dark red
    Pose->White = Maximum(0.f, 1.f - Elapsed / 0.25f);
    Pose->Flash = 0.85f;
    return true;
}
