/* Body poses: squash and stretch for players and monsters, drawn on top of
   their sprite animations. Read only from how each body's height changes
   from frame to frame, so it looks the same offline and online, where
   replicas carry no vertical speed:

   - rising or falling fast stretches the body tall and thin;
   - a sudden kick upward in the air (a second jump, a Launch) pops it
     taller still for a moment;
   - landing from a fall squashes it short and wide, springing back.

   The sprite keeps its feet where they were; DrawEntity asks
   GetBodyScale for the scale. Updated once a frame by UpdateBodyPoses,
   after the world is drawn, so a pose is one frame old, which no one sees. */

#define BODY_POSE_SLOTS 4096
// NOTE(zoubir): vertical speed at which the stretch is full
#define BODY_STRETCH_SPEED 900.f
#define BODY_STRETCH_MAX 0.16f
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
    u32 EntityId;
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
            continue;
        }
        // NOTE(zoubir): a new entity in a reused slot starts at rest
        if (Pose->EntityId != Entity->ID)
        {
            *Pose = {};
            Pose->EntityId = Entity->ID;
            Pose->LastZ = Entity->Position.Z;
        }

        float SpeedZ = (Entity->Position.Z - Pose->LastZ) / DeltaTime;
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
        }
        Pose->SpeedZ = OnGround ? 0.f : SpeedZ;
        Pose->LastZ = Entity->Position.Z;
        Pose->Squash = Maximum(0.f, Pose->Squash - BODY_POSE_RECOVERY * DeltaTime);
        Pose->Pop = Maximum(0.f, Pose->Pop - BODY_POSE_RECOVERY * DeltaTime);
    }
}

// NOTE(zoubir): width and height multipliers for the sprite; the area
// stays about the same, so a squash reads as weight, not shrinking
internal v2
GetBodyScale(app_state *AppState, world_entity *Entity)
{
    v2 Result = V2(1.f, 1.f);
    u32 Index = (u32)(Entity - AppState->World.Entities);
    if (!AppState->BodyPoses || Index >= BODY_POSE_SLOTS || !HasBodyPose(Entity))
    {
        return Result;
    }
    body_pose *Pose = &AppState->BodyPoses->Poses[Index];
    if (Pose->EntityId != Entity->ID)
    {
        return Result;
    }
    float Stretch = BODY_STRETCH_MAX *
        Clamp01(Absolute(Pose->SpeedZ) / BODY_STRETCH_SPEED);
    // NOTE(zoubir): eased, so the squash springs back fast then settles
    float Squash = BODY_SQUASH_DEPTH * Pose->Squash * Pose->Squash;
    float Pop = BODY_POP_HEIGHT * Pose->Pop * Pose->Pop;
    Result.Y = 1.f + Stretch + Pop - Squash;
    Result.X = 1.f / Result.Y;
    return Result;
}
