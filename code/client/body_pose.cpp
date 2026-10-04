/* Body poses: squash and stretch for players and monsters, drawn on top of
   their sprite animations. Read only from how each body's height changes
   from frame to frame, so it looks the same offline and online, where
   replicas carry no vertical speed:

   - rising or falling fast stretches the body tall and thin, and moving
     sideways very fast (a dash, a shove) stretches it long and low;
   - a sudden kick upward in the air (a second jump, a Launch) pops it
     taller still for a moment;
   - landing from a fall squashes it short and wide, springing back;
   - that same kick upward spins it once around, a somersault toward
     where it is heading;
   - running leans it a little into the way it runs;
   - losing health flashes it red and jolts it, a short squash;
   - a cast with a wind-up crouches it while it charges and pops it up as
     it lets go (SetBodyWindup, from the cast's bursts, fx_bursts.cpp);
   - a stun sways it side to side on its feet, dizzy;
   - turning to face another way squeezes it thin for an instant, so the
     sprite's snap to its new row reads as a turn.

   The sprite keeps its feet where they were; DrawEntity asks
   GetBodyPose for the scale and the angle. Updated once a frame by UpdateBodyPoses,
   after the world is drawn, so a pose is one frame old, which no one sees. */

#define BODY_POSE_SLOTS 4096
// NOTE(zoubir): vertical speed at which the stretch is full
#define BODY_STRETCH_SPEED 900.f
#define BODY_STRETCH_MAX 0.16f
// NOTE(zoubir): ground speed at which a body starts to stretch sideways
// (a full walk is 93, a dash 650), and where the stretch is full
#define BODY_RUSH_SPEED 250.f
#define BODY_RUSH_FULL_SPEED 600.f
#define BODY_RUSH_MAX 0.2f
// NOTE(zoubir): a body that moves farther than this in one frame was put
// somewhere (a respawn, a map change), not thrown: its pose starts over.
// A blink is at most 160
#define BODY_TELEPORT_DISTANCE 200.f
// NOTE(zoubir): a fall faster than this squashes on landing; the squash is
// full at BODY_SQUASH_FULL_SPEED
#define BODY_SQUASH_MIN_SPEED 120.f
#define BODY_SQUASH_FULL_SPEED 450.f
#define BODY_SQUASH_DEPTH 0.28f
// NOTE(zoubir): a jump in vertical speed this big within a frame, in the
// air, is a kick upward
#define BODY_POP_KICK 200.f
#define BODY_POP_HEIGHT 0.22f
// NOTE(zoubir): per second, how fast squash and pop wear off
#define BODY_POSE_RECOVERY 7.f
#define BODY_SPIN_SECONDS 0.32f
// NOTE(zoubir): radians of lean at full run speed (BODY_LEAN_SPEED)
#define BODY_LEAN_MAX 0.12f
#define BODY_LEAN_SPEED 180.f
// NOTE(zoubir): a hit's red flash fades over this many seconds
#define BODY_HIT_FLASH_SECONDS 0.14f
#define BODY_HIT_SQUASH 0.55f
// NOTE(zoubir): a cast crouches fully in this long, this deep; letting go
// pops by this share of how far it crouched. A charge with no release
// lets go by itself after BODY_WINDUP_MAX_SECONDS (a release lost with
// its snapshot)
#define BODY_WINDUP_SECONDS 0.15f
#define BODY_WINDUP_DEPTH 0.14f
#define BODY_WINDUP_RELEASE 0.7f
#define BODY_WINDUP_MAX_SECONDS 0.6f
// NOTE(zoubir): how thin a turn squeezes the body, and how long it lasts
#define BODY_TURN_SQUEEZE 0.25f
#define BODY_TURN_SECONDS 0.1f
// NOTE(zoubir): a stunned body's sway, in radians and turns per second
#define BODY_DIZZY_ANGLE 0.16f
#define BODY_DIZZY_SPEED 2.2f

inline float
Clamp01(float Value)
{
    float Result = Minimum(1.f, Maximum(0.f, Value));
    return Result;
}

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
    // NOTE(zoubir): 0..1, how far into a cast's crouch, and the seconds
    // left until it lets go
    float Windup;
    float ChargeLeft;
    // NOTE(zoubir): seconds this body has been tracked, for the sway
    float Clock;
    // NOTE(zoubir): the way it faced last frame, and 1 when it has just
    // turned, down to 0
    u32 Facing;
    float Turn;
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
    // somersault)
    bool32 AboutFeet;
    // NOTE(zoubir): 0..1, how red the sprite is drawn
    float Flash;
};

struct body_poses
{
    body_pose Poses[BODY_POSE_SLOTS];
};

inline bool32
HasBodyPose(world_entity *Entity)
{
    bool32 Result = Entity->IsPresent &&
        (Entity->Type == EntityType_Player || Entity->Type == EntityType_Monster);
    return Result;
}

internal void
UpdateBodyPoses(app_state *AppState, float DeltaTime)
{
    if (DeltaTime <= 0.f)
    {
        return;
    }
    if (!AppState->BodyPoses)
    {
        AppState->BodyPoses = AllocateStruct(&AppState->MemoryArena, body_poses);
        *AppState->BodyPoses = {};
    }
    world *World = &AppState->World;
    u32 Count = Minimum(World->EntityCount, (u32)BODY_POSE_SLOTS);
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        body_pose *Pose = &AppState->BodyPoses->Poses[Index];
        if (!HasBodyPose(Entity))
        {
            Pose->Tracking = false;
            continue;
        }
        v3 Moved = Entity->Position - V3(Pose->LastX, Pose->LastY, Pose->LastZ);
        // NOTE(zoubir): a new entity in a reused slot, or one put somewhere
        // else, starts at rest
        if (!Pose->Tracking || Pose->EntityId != Entity->ID ||
            LengthSq(Moved) > Square(BODY_TELEPORT_DISTANCE))
        {
            *Pose = {};
            Pose->Tracking = true;
            Pose->EntityId = Entity->ID;
            Pose->LastZ = Entity->Position.Z;
            Pose->LastX = Entity->Position.X;
            Pose->LastY = Entity->Position.Y;
            Pose->LastHp = Entity->Hp;
        }
        if (Entity->Hp < Pose->LastHp)
        {
            Pose->Flash = 1.f;
            Pose->Squash = Maximum(Pose->Squash, BODY_HIT_SQUASH);
        }
        Pose->LastHp = Entity->Hp;
        Pose->Flash = Maximum(0.f, Pose->Flash - DeltaTime / BODY_HIT_FLASH_SECONDS);

        float SpeedZ = (Entity->Position.Z - Pose->LastZ) / DeltaTime;
        float SpeedX = (Entity->Position.X - Pose->LastX) / DeltaTime;
        // NOTE(zoubir): eased, a one-frame jolt (a shove, a correction)
        // does not snap the lean
        Pose->SpeedX += (SpeedX - Pose->SpeedX) * Minimum(1.f, 12.f * DeltaTime);
        Pose->LastX = Entity->Position.X;
        float SpeedY = (Entity->Position.Y - Pose->LastY) / DeltaTime;
        float SpeedXY = SquareRoot(Square(SpeedX) + Square(SpeedY));
        Pose->SpeedXY += (SpeedXY - Pose->SpeedXY) * Minimum(1.f, 12.f * DeltaTime);
        Pose->LastY = Entity->Position.Y;
        bool32 OnGround = Entity->Position.Z <= Entity->GroundZ + 0.5f;
        if (OnGround && Pose->SpeedZ < -BODY_SQUASH_MIN_SPEED)
        {
            float Hardness = Clamp01((-Pose->SpeedZ - BODY_SQUASH_MIN_SPEED) /
                                     (BODY_SQUASH_FULL_SPEED - BODY_SQUASH_MIN_SPEED));
            Pose->Squash = Maximum(Pose->Squash, 0.35f + 0.65f * Hardness);
        }
        if (!OnGround && SpeedZ - Pose->SpeedZ > BODY_POP_KICK)
        {
            Pose->Pop = 1.f;
            Pose->Spin = 1.f;
            // NOTE(zoubir): screen Y grows down, so a positive angle turns
            // clockwise: forward for a body heading right
            Pose->SpinSign = Pose->SpeedX < 0.f ? -1.f : 1.f;
        }
        if (OnGround)
        {
            Pose->Spin = 0.f;
        }
        Pose->Spin = Maximum(0.f, Pose->Spin - DeltaTime / BODY_SPIN_SECONDS);
        Pose->SpeedZ = OnGround ? 0.f : SpeedZ;
        Pose->LastZ = Entity->Position.Z;
        Pose->Clock += DeltaTime;
        u32 Facing = (u32)Entity->AnimationState.LastAnimationDirection;
        if (Facing != Pose->Facing && Pose->Clock > DeltaTime)
        {
            Pose->Turn = 1.f;
        }
        Pose->Facing = Facing;
        Pose->Turn = Maximum(0.f, Pose->Turn - DeltaTime / BODY_TURN_SECONDS);
        if (Pose->ChargeLeft > 0.f)
        {
            Pose->ChargeLeft -= DeltaTime;
            Pose->Windup = Minimum(1.f, Pose->Windup + DeltaTime / BODY_WINDUP_SECONDS);
        }
        else if (Pose->Windup > 0.f)
        {
            Pose->Pop = Maximum(Pose->Pop, BODY_WINDUP_RELEASE * Pose->Windup);
            Pose->Windup = 0.f;
        }
        Pose->Squash = Maximum(0.f, Pose->Squash - BODY_POSE_RECOVERY * DeltaTime);
        Pose->Pop = Maximum(0.f, Pose->Pop - BODY_POSE_RECOVERY * DeltaTime);
    }
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
// stays about the same, so a squash reads as weight, not shrinking), and
// its turn in radians
internal body_pose_draw
GetBodyPose(app_state *AppState, world_entity *Entity)
{
    body_pose_draw Result = {V2(1.f, 1.f), 0.f, false, 0.f};
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
    float Squash = BODY_SQUASH_DEPTH * Pose->Squash * Pose->Squash +
        BODY_WINDUP_DEPTH * Pose->Windup;
    float Pop = BODY_POP_HEIGHT * Pose->Pop * Pose->Pop;
    Result.Flash = Pose->Flash;
    Result.Scale.Y = 1.f + Stretch + Pop - Squash;
    Result.Scale.X = 1.f / Result.Scale.Y;
    float Rush = BODY_RUSH_MAX * Clamp01((Pose->SpeedXY - BODY_RUSH_SPEED) /
                                         (BODY_RUSH_FULL_SPEED - BODY_RUSH_SPEED));
    Result.Scale.X *= 1.f + Rush;
    Result.Scale.Y /= 1.f + Rush;
    Result.Scale.X *= 1.f - BODY_TURN_SQUEEZE * Pose->Turn;
    if (Pose->Spin > 0.f)
    {
        // NOTE(zoubir): fast out of the kick, settling upright
        float Turned = 1.f - Pose->Spin * Pose->Spin;
        Result.Angle = Pose->SpinSign * 2.f * Pi32 * Turned;
    }
    else if (Entity->Position.Z <= Entity->GroundZ + 0.5f)
    {
        float Run = Pose->SpeedX / BODY_LEAN_SPEED;
        Result.Angle = BODY_LEAN_MAX * Minimum(1.f, Maximum(-1.f, Run));
        if (HasStatus(Entity, StatusEffect_Stunned))
        {
            Result.Angle += BODY_DIZZY_ANGLE *
                Sin(2.f * Pi32 * BODY_DIZZY_SPEED * Pose->Clock);
        }
        Result.AboutFeet = true;
    }
    return Result;
}
