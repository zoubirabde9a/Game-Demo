/* Body poses: how players and monsters squash, stretch, tilt and flash on
   top of their sprite animations, and the dust their feet kick up. One
   pose per unit slot, kept in client memory and read only from what the
   client already sees each frame (position, vertical speed, health,
   facing, status, the last hit), so it looks the same offline and
   online, where the bodies are replicas:

   - rising or falling fast stretches the body tall and thin, and moving
     sideways very fast (a dash, a shove) stretches it long and low;
   - a sudden kick upward in the air (a second jump, a Launch) pops it
     taller still for a moment and spins it once around, a somersault
     toward where it is heading;
   - landing from a fall squashes it short and wide, springing back, and
     starting to run squashes it a little, as it pushes off;
   - running leans it into the way it runs;
   - losing health flashes it white for a couple of frames, then red, and
     jolts it: a short squash and a flinch tilting away from the blow (the
     way the hit threw it, sim/hit.cpp HitAngle; when that is not known,
     the way it moves, or else back from where it faces);
   - a hit-pause (HitStop) holds the body still, white, on its frame
     (UpdateEntityUvs skips a frozen body);
   - a body a hit lifted (HitThrown) tumbles: it tips back the higher it
     is thrown, righting itself as it comes down. Without the hit's data,
     leaving the ground soon after losing health counts as thrown;
   - heavy monsters squash and flinch less, by the share the hit system
     throws them (KnockbackScale);
   - the local player landing a solid hit freezes its own sprite for an
     instant and nudges the camera toward the blow (GetHitNudge, read by
     camera.cpp); its movement is not touched;
   - a cast with a wind-up crouches it while it charges and pops it up as
     it lets go (SetBodyWindup, from the cast's bursts, fx_bursts.cpp);
   - a stun sways it side to side on its feet, dizzy;
   - turning to face another way squeezes it thin for an instant, so the
     sprite's snap to its new row reads as a turn;
   - standing still on the ground it breathes, a slow rise and settle,
     each body at its own pace so a crowd does not breathe as one;
   - a walking unit running on the ground leaves a puff of dust every few
     steps, and a trail of it when it goes very fast (a dash, a charge, a
     shove sliding it along).

   Every tilt eases toward where it should be rather than jumping there,
   so a change of state (leaving the ground, a stun ending) never pops.
   The tuning is the table of numbers in body_pose/tuning.h. The sprite
   keeps its feet where they were; DrawEntity asks GetBodyPose for the
   scale, the angle and the flash. Updated once a frame by UpdateBodyPoses
   (body_pose/update.cpp), after the world is drawn, so a pose is one
   frame old, which no one sees. */

#include "body_pose/tuning.h"

inline float
Clamp01(float Value)
{
    float Result = Minimum(1.f, Maximum(0.f, Value));
    return Result;
}

// NOTE(zoubir): draw_entities.cpp and fx_bursts.cpp, later in the build
inline float EntityGroundZ(world *World, world_entity *Entity);
internal void AddBurst(app_state *AppState, sim_burst Kind, u32 Slot,
                       v3 Position, float Angle);

struct body_pose
{
    float LastZ;
    float SpeedZ;
    float Squash;
    float Pop;
    // NOTE(zoubir): 1 at the start of a somersault, down to 0 at its end;
    // SpinSign is which way it turns
    float Spin;
    float SpinSign;
    float LastX;
    float SpeedX;
    float LastY;
    // NOTE(zoubir): ground speed, eased like SpeedX
    float SpeedXY;
    float LastHp;
    // NOTE(zoubir): 1 the frame health drops, down to 0
    float Flash;
    // NOTE(zoubir): 1 the frame health drops, down to 0; FlinchSign is the
    // way the top of the body tips (+1 right)
    float Flinch;
    float FlinchSign;
    // NOTE(zoubir): seconds left in which leaving the ground counts as
    // being thrown; Tumbling until it lands again
    float KnockLeft;
    bool32 Tumbling;
    // NOTE(zoubir): the tilt drawn (lean, sway, tumble), eased toward
    // where it should be
    float Tilt;
    // NOTE(zoubir): 0..1, how far into a cast's crouch, and the seconds
    // left until it lets go
    float Windup;
    float ChargeLeft;
    // NOTE(zoubir): seconds this body has been tracked, for the sway
    float Clock;
    // NOTE(zoubir): seconds to its next puff of dust
    float DustTimer;
    // NOTE(zoubir): the way it faced last frame, and 1 when it has just
    // turned, down to 0
    u32 Facing;
    float Turn;
    // NOTE(zoubir): it was off the ground last frame; a kick upward only
    // counts in the air, or every jump off the ground somersaulted
    bool32 Airborne;
    bool32 Running;
    // NOTE(zoubir): held still this frame (a hit-pause, or Hold, the
    // seconds left of the local player's freeze after landing a hit)
    bool32 Frozen;
    float Hold;
    // NOTE(zoubir): the slot held a body last frame. Ids are slot indices
    // and are reused, so a new body in a freed slot is told apart by the
    // slot having been empty, not by its id
    bool32 Tracking;
    u32 EntityId;
};

struct body_pose_draw
{
    v2 Scale;
    float Angle;
    // NOTE(zoubir): turn about the feet (a lean) rather than the middle (a
    // somersault, a tumble)
    bool32 AboutFeet;
    // NOTE(zoubir): 0..1, how red the sprite is drawn, and how much white
    // light is added over it
    float Flash;
    float White;
};

struct body_poses
{
    body_pose Poses[BODY_POSE_SLOTS];
    // NOTE(zoubir): the camera's nudge after the local player lands a
    // hit: the way it goes, and the seconds left
    v2 NudgeDirection;
    float NudgeLeft;
};

inline bool32
HasBodyPose(world_entity *Entity)
{
    bool32 Result = Entity->IsPresent &&
        (Entity->Type == EntityType_Player || Entity->Type == EntityType_Monster);
    return Result;
}

// NOTE(zoubir): +1 when the body faces right, -1 left, 0 up or down
inline float
FacingSign(u32 Facing)
{
    float Result = Facing == AnimationDirection_Right ? 1.f :
        Facing == AnimationDirection_Left ? -1.f : 0.f;
    return Result;
}

// NOTE(zoubir): health dropped this frame: flash, jolt, flinch away from
// the blow, and be ready to tumble if it is thrown off its feet
internal void
StartBodyHit(body_pose *Pose, world_entity *Entity, float SpeedX, u32 Slot)
{
    Pose->Flash = 1.f;
    Pose->Squash = Maximum(Pose->Squash, BODY_HIT_SQUASH);
    Pose->Flinch = 1.f;
    // NOTE(zoubir): a hit with its data says whether it lifted the body,
    // so there is no guess to make, even once the data goes stale
    Pose->KnockLeft = Entity->HitFresh > 0.f ? 0.f : BODY_KNOCK_SECONDS;
    float Back = -FacingSign(Pose->Facing);
    float Sideways = Cos(Entity->HitAngle);
    if (Entity->HitFresh > 0.f && Absolute(Sideways) > BODY_FLINCH_SIDEWAYS)
    {
        Pose->FlinchSign = Sideways > 0.f ? 1.f : -1.f;
    }
    else if (Absolute(SpeedX) > BODY_FLINCH_KNOCK_SPEED)
    {
        Pose->FlinchSign = SpeedX > 0.f ? 1.f : -1.f;
    }
    else if (Back != 0.f)
    {
        Pose->FlinchSign = Back;
    }
    else
    {
        // NOTE(zoubir): facing up or down with nothing to go by: each slot
        // picks a side, so a crowd does not all tip the same way
        Pose->FlinchSign = (Slot & 1) ? 1.f : -1.f;
    }
}

// NOTE(zoubir): Target lost health to a hit the local player landed: a
// solid one holds the player's sprite still for an instant and nudges the
// camera toward the blow. Drawing only; prediction never sees it
internal void
FeelLandedHit(app_state *AppState, world_entity *Target, float Dealt)
{
    world_entity *Local = GetLocalPlayer(AppState);
    if (!Local || Target->HitFresh <= 0.f || Dealt < HITSTOP_MIN_DAMAGE ||
        Target->HitBySlot != AppState->LocalPlayerIndex + 1)
    {
        return;
    }
    u32 Index = (u32)(Local - AppState->World.Entities);
    if (Index < BODY_POSE_SLOTS)
    {
        AppState->BodyPoses->Poses[Index].Hold = BODY_ATTACK_HOLD_SECONDS;
    }
    AppState->BodyPoses->NudgeDirection = V2(Cos(Target->HitAngle), Sin(Target->HitAngle));
    AppState->BodyPoses->NudgeLeft = BODY_NUDGE_SECONDS;
}

// NOTE(zoubir): a puff at the feet of a unit running on the ground, now
// and then, more often the faster it goes
internal void
KickUpDust(app_state *AppState, world_entity *Entity, body_pose *Pose,
           bool32 OnGround, float DeltaTime)
{
    world_entity *Local = GetLocalPlayer(AppState);
    bool32 Near = !Local || LengthSq(Entity->Position.XY - Local->Position.XY) <
        Square(BODY_DUST_RANGE);
    // NOTE(zoubir): the burst pool is made by the first DrawFxBursts; until
    // then (and in tests, which only pose bodies) there is nowhere to puff
    bool32 Running = AppState->FxBursts && OnGround && Near && Entity->Hp > 0.f &&
        IsWalkingUnit(Entity) && Pose->SpeedXY > BODY_FOOTSTEP_SPEED;
    Pose->DustTimer -= DeltaTime;
    if (!Running)
    {
        Pose->DustTimer = 0.5f * BODY_FOOTSTEP_SECONDS;
    }
    else if (Pose->DustTimer <= 0.f)
    {
        bool32 Rushing = Pose->SpeedXY > BODY_TRAIL_SPEED;
        Pose->DustTimer = Rushing ? BODY_TRAIL_SECONDS : BODY_FOOTSTEP_SECONDS;
        u32 Slot = Entity->Type == EntityType_Player ? Entity->PlayerIndex : SIM_NOBODY;
        AddBurst(AppState, SimBurst_Step, Slot, Entity->Position, 0.f);
    }
}

#include "body_pose/update.cpp"

// NOTE(zoubir): the body is held still this frame: its sprite keeps the
// frame it had (UpdateEntityUvs)
internal bool32
IsBodyFrozen(app_state *AppState, world_entity *Entity)
{
    u32 Index = (u32)(Entity - AppState->World.Entities);
    bool32 Result = AppState->BodyPoses && Index < BODY_POSE_SLOTS &&
        AppState->BodyPoses->Poses[Index].Tracking &&
        AppState->BodyPoses->Poses[Index].EntityId == Entity->ID &&
        AppState->BodyPoses->Poses[Index].Frozen;
    return Result;
}

// NOTE(zoubir): where the camera is pushed this frame, in world units:
// out toward the blow and back
internal v2
GetHitNudge(app_state *AppState)
{
    v2 Result = {};
    if (AppState->BodyPoses && AppState->BodyPoses->NudgeLeft > 0.f)
    {
        float Done = 1.f - AppState->BodyPoses->NudgeLeft / BODY_NUDGE_SECONDS;
        Result = (BODY_NUDGE_DISTANCE * Sin(Pi32 * Done)) *
            AppState->BodyPoses->NudgeDirection;
    }
    return Result;
}

// NOTE(zoubir): Entity starts crouching (Charging), or lets go and pops
internal void
SetBodyWindup(app_state *AppState, world_entity *Entity, bool32 Charging)
{
    u32 Index = (u32)(Entity - AppState->World.Entities);
    if (AppState->BodyPoses && Index < BODY_POSE_SLOTS &&
        AppState->BodyPoses->Poses[Index].Tracking &&
        AppState->BodyPoses->Poses[Index].EntityId == Entity->ID)
    {
        AppState->BodyPoses->Poses[Index].ChargeLeft =
            Charging ? BODY_WINDUP_MAX_SECONDS : 0.f;
    }
}

// NOTE(zoubir): width and height multipliers for the sprite (the area
// stays about the same, so a squash reads as weight, not shrinking), its
// turn in radians and its flash
internal body_pose_draw
GetBodyPose(app_state *AppState, world_entity *Entity)
{
    body_pose_draw Result = {V2(1.f, 1.f), 0.f, false, 0.f, 0.f};
    u32 Index = (u32)(Entity - AppState->World.Entities);
    if (!AppState->BodyPoses || Index >= BODY_POSE_SLOTS || !HasBodyPose(Entity))
    {
        return Result;
    }
    body_pose *Pose = &AppState->BodyPoses->Poses[Index];
    if (!Pose->Tracking || Pose->EntityId != Entity->ID)
    {
        return Result;
    }
    float Stretch = BODY_STRETCH_MAX *
        Clamp01(Absolute(Pose->SpeedZ) / BODY_STRETCH_SPEED);
    // NOTE(zoubir): eased, so the squash springs back fast then settles
    // NOTE(zoubir): heavy monsters squash and flinch less
    float Weight = KnockbackScale(Entity);
    float Squash = Weight * BODY_SQUASH_DEPTH * Pose->Squash * Pose->Squash +
        BODY_WINDUP_DEPTH * Pose->Windup;
    float Pop = BODY_POP_HEIGHT * Pose->Pop * Pose->Pop;
    Result.Flash = Pose->Flash;
    Result.White = Clamp01((Pose->Flash - BODY_HIT_WHITE) / (1.f - BODY_HIT_WHITE));
    Result.Scale.Y = 1.f + Stretch + Pop - Squash;
    Result.Scale.X = 1.f / Result.Scale.Y;
    float Rush = BODY_RUSH_MAX * Clamp01((Pose->SpeedXY - BODY_RUSH_SPEED) /
                                         (BODY_RUSH_FULL_SPEED - BODY_RUSH_SPEED));
    Result.Scale.X *= 1.f + Rush;
    Result.Scale.Y /= 1.f + Rush;
    Result.Scale.X *= 1.f - BODY_TURN_SQUEEZE * Pose->Turn;
    bool32 OnGround = !Pose->Airborne;
    if (Pose->SpeedXY < BODY_STILL_SPEED && OnGround)
    {
        // NOTE(zoubir): the slot index offsets each body's breath
        float Phase = 2.f * Pi32 * (BODY_BREATH_RATE * Pose->Clock + 0.37f * Index);
        Result.Scale.Y *= 1.f + BODY_BREATH_DEPTH * (0.5f + 0.5f * Sin(Phase));
    }
    // NOTE(zoubir): the flinch snaps over with the blow and eases back
    float Angle = Pose->Tilt +
        Weight * Pose->FlinchSign * BODY_FLINCH_ANGLE * Pose->Flinch * Pose->Flinch;
    if (Pose->Spin > 0.f)
    {
        // NOTE(zoubir): fast out of the kick, settling upright
        float Turned = 1.f - Pose->Spin * Pose->Spin;
        Angle += Pose->SpinSign * 2.f * Pi32 * Turned;
    }
    // NOTE(zoubir): what is left of an eased tilt is no tilt
    Result.Angle = Absolute(Angle) < 0.002f ? 0.f : Angle;
    Result.AboutFeet = OnGround;
    return Result;
}
