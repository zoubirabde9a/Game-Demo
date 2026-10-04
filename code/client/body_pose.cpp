/* Body poses: how players and monsters squash, stretch, tilt and flash on
   top of their sprite animations, and the dust their feet kick up. One
   pose per unit slot, kept in client memory and read only from what the
   client already sees each frame (position, vertical speed, health,
   facing, status), so it looks the same offline and online, where the
   bodies are replicas:

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
     way the hit threw it, or else back from where it faces);
   - a body hit off its feet tumbles: it tips back the higher it is
     thrown, righting itself as it comes down;
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
   The tuning is the table of numbers below. The sprite keeps its feet
   where they were; DrawEntity asks GetBodyPose for the scale, the angle
   and the flash. Updated once a frame by UpdateBodyPoses, after the world
   is drawn, so a pose is one frame old, which no one sees. */

#define BODY_POSE_SLOTS 4096
// NOTE(zoubir): vertical speed at which the stretch is full
#define BODY_STRETCH_SPEED 900.f
#define BODY_STRETCH_MAX 0.16f
// NOTE(zoubir): ground speed at which a body starts to stretch sideways
// (a player's full run is 260, a dash 1300, a Push throws at 750), and
// where the stretch is full
#define BODY_RUSH_SPEED 300.f
#define BODY_RUSH_FULL_SPEED 700.f
#define BODY_RUSH_MAX 0.2f
// NOTE(zoubir): a body that moves farther than this in one frame was put
// somewhere (a respawn, a map change), not thrown: its pose starts over.
// A blink is at most PLAYER_AIM_REACH (320)
#define BODY_TELEPORT_DISTANCE 400.f
// NOTE(zoubir): a fall faster than this squashes on landing; the squash is
// full at BODY_SQUASH_FULL_SPEED
#define BODY_SQUASH_MIN_SPEED 120.f
#define BODY_SQUASH_FULL_SPEED 450.f
#define BODY_SQUASH_DEPTH 0.28f
// NOTE(zoubir): a jump in vertical speed this big within a frame, in the
// air, is a kick upward
#define BODY_POP_KICK 120.f
#define BODY_POP_HEIGHT 0.22f
// NOTE(zoubir): per second, how fast squash and pop wear off
#define BODY_POSE_RECOVERY 7.f
#define BODY_SPIN_SECONDS 0.32f
// NOTE(zoubir): radians of lean at full run speed (BODY_LEAN_SPEED), and
// how fast any tilt eases toward where it should be, per second
#define BODY_LEAN_MAX 0.12f
#define BODY_LEAN_SPEED 180.f
#define BODY_TILT_EASE 14.f
// NOTE(zoubir): a run starts when the ground speed passes RUN_START and
// stops below RUN_STOP; starting squashes this much (of a full landing)
#define BODY_RUN_START_SPEED 120.f
#define BODY_RUN_STOP_SPEED 40.f
#define BODY_RUN_START_SQUASH 0.45f
// NOTE(zoubir): a hit's flash lasts this many seconds; it is white while
// above BODY_HIT_WHITE (the first two frames at 30 a second), then red
#define BODY_HIT_FLASH_SECONDS 0.2f
#define BODY_HIT_WHITE 0.65f
#define BODY_HIT_SQUASH 0.55f
// NOTE(zoubir): a hit tilts the body this far away from the blow, back
// upright within FLINCH_SECONDS. A body knocked this fast sideways at the
// hit flinches the way it was thrown
#define BODY_FLINCH_ANGLE 0.3f
#define BODY_FLINCH_SECONDS 0.25f
#define BODY_FLINCH_KNOCK_SPEED 40.f
// NOTE(zoubir): a body that leaves the ground within KNOCK_SECONDS of a
// hit (or stunned) was thrown: it tips back up to TUMBLE_ANGLE, fully at
// TUMBLE_HEIGHT above the ground
#define BODY_KNOCK_SECONDS 0.3f
#define BODY_TUMBLE_ANGLE 1.1f
#define BODY_TUMBLE_HEIGHT 60.f
// NOTE(zoubir): a cast crouches fully in this long, this deep; letting go
// pops by this share of how far it crouched. A charge with no release
// lets go by itself after BODY_WINDUP_MAX_SECONDS (a release lost with
// its snapshot)
#define BODY_WINDUP_SECONDS 0.15f
#define BODY_WINDUP_DEPTH 0.14f
#define BODY_WINDUP_RELEASE 0.7f
#define BODY_WINDUP_MAX_SECONDS 0.6f
// NOTE(zoubir): an idle body's breath: how much taller at the top, breaths
// a second, and the ground speed under which a body counts as still
#define BODY_BREATH_DEPTH 0.03f
#define BODY_BREATH_RATE 0.45f
#define BODY_STILL_SPEED 10.f
// NOTE(zoubir): how thin a turn squeezes the body, and how long it lasts
#define BODY_TURN_SQUEEZE 0.25f
#define BODY_TURN_SECONDS 0.1f
// NOTE(zoubir): a stunned body's sway, in radians and turns per second
#define BODY_DIZZY_ANGLE 0.16f
#define BODY_DIZZY_SPEED 2.2f
// NOTE(zoubir): faster than FOOTSTEP_SPEED on the ground a unit puffs dust
// every FOOTSTEP_SECONDS, faster than TRAIL_SPEED every TRAIL_SECONDS.
// Only within DUST_RANGE of the local player, so far-off crowds do not
// fill the burst pool
#define BODY_FOOTSTEP_SPEED 80.f
#define BODY_FOOTSTEP_SECONDS 0.24f
#define BODY_TRAIL_SPEED 340.f
#define BODY_TRAIL_SECONDS 0.06f
#define BODY_DUST_RANGE 1100.f

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
StartBodyHit(body_pose *Pose, float SpeedX, u32 Slot)
{
    Pose->Flash = 1.f;
    Pose->Squash = Maximum(Pose->Squash, BODY_HIT_SQUASH);
    Pose->Flinch = 1.f;
    Pose->KnockLeft = BODY_KNOCK_SECONDS;
    float Back = -FacingSign(Pose->Facing);
    if (Absolute(SpeedX) > BODY_FLINCH_KNOCK_SPEED)
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
            Pose->Facing = (u32)Entity->AnimationState.LastAnimationDirection;
        }

        // NOTE(zoubir): the body's own vertical speed, which snapshots carry
        // for replicas too; measured from height between frames, a second
        // jump's kick came out under BODY_POP_KICK at 30 frames a second
        float SpeedZ = Entity->Velocity.Z;
        float SpeedX = (Entity->Position.X - Pose->LastX) / DeltaTime;
        if (Entity->Hp < Pose->LastHp)
        {
            StartBodyHit(Pose, SpeedX, Index);
        }
        Pose->LastHp = Entity->Hp;
        Pose->Flash = Maximum(0.f, Pose->Flash - DeltaTime / BODY_HIT_FLASH_SECONDS);
        Pose->Flinch = Maximum(0.f, Pose->Flinch - DeltaTime / BODY_FLINCH_SECONDS);
        Pose->KnockLeft = Maximum(0.f, Pose->KnockLeft - DeltaTime);

        // NOTE(zoubir): eased, a one-frame jolt (a shove, a correction)
        // does not snap the lean
        Pose->SpeedX += (SpeedX - Pose->SpeedX) * Minimum(1.f, 12.f * DeltaTime);
        Pose->LastX = Entity->Position.X;
        float SpeedY = (Entity->Position.Y - Pose->LastY) / DeltaTime;
        float SpeedXY = SquareRoot(Square(SpeedX) + Square(SpeedY));
        Pose->SpeedXY += (SpeedXY - Pose->SpeedXY) * Minimum(1.f, 12.f * DeltaTime);
        Pose->LastY = Entity->Position.Y;
        // NOTE(zoubir): replicas carry no GroundZ; raised ground comes
        // from the map (draw_entities.cpp)
        float GroundZ = EntityGroundZ(World, Entity);
        float Height = Entity->Position.Z - GroundZ;
        bool32 OnGround = Height <= 0.5f;
        if (OnGround && Pose->SpeedZ < -BODY_SQUASH_MIN_SPEED)
        {
            float Hardness = Clamp01((-Pose->SpeedZ - BODY_SQUASH_MIN_SPEED) /
                                     (BODY_SQUASH_FULL_SPEED - BODY_SQUASH_MIN_SPEED));
            Pose->Squash = Maximum(Pose->Squash, 0.35f + 0.65f * Hardness);
        }
        if (!OnGround && Pose->Airborne && SpeedZ - Pose->SpeedZ > BODY_POP_KICK)
        {
            Pose->Pop = 1.f;
            Pose->Spin = 1.f;
            // NOTE(zoubir): screen Y grows down, so a positive angle turns
            // clockwise: forward for a body heading right
            Pose->SpinSign = Pose->SpeedX < 0.f ? -1.f : 1.f;
        }
        if (!OnGround && !Pose->Tumbling &&
            (Pose->KnockLeft > 0.f || HasStatus(Entity, StatusEffect_Stunned)))
        {
            Pose->Tumbling = true;
            if (Pose->Flinch == 0.f)
            {
                Pose->FlinchSign = Pose->SpeedX < 0.f ? -1.f : 1.f;
            }
        }
        if (OnGround)
        {
            Pose->Spin = 0.f;
            Pose->Tumbling = false;
        }
        bool32 WasRunning = Pose->Running;
        Pose->Running = OnGround &&
            Pose->SpeedXY > (WasRunning ? BODY_RUN_STOP_SPEED : BODY_RUN_START_SPEED);
        if (Pose->Running && !WasRunning)
        {
            Pose->Squash = Maximum(Pose->Squash, BODY_RUN_START_SQUASH);
        }
        Pose->Spin = Maximum(0.f, Pose->Spin - DeltaTime / BODY_SPIN_SECONDS);
        Pose->SpeedZ = OnGround ? 0.f : SpeedZ;
        Pose->Airborne = !OnGround;
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

        // NOTE(zoubir): where the tilt should be: leaning into the run and
        // swaying when dizzy on the ground, tipped back by the height of
        // a throw in the air, upright otherwise
        float Tilt = 0.f;
        if (OnGround)
        {
            float Run = Pose->SpeedX / BODY_LEAN_SPEED;
            Tilt = BODY_LEAN_MAX * Minimum(1.f, Maximum(-1.f, Run));
            if (HasStatus(Entity, StatusEffect_Stunned))
            {
                Tilt += BODY_DIZZY_ANGLE *
                    Sin(2.f * Pi32 * BODY_DIZZY_SPEED * Pose->Clock);
            }
        }
        else if (Pose->Tumbling)
        {
            Tilt = Pose->FlinchSign * BODY_TUMBLE_ANGLE *
                Clamp01(Height / BODY_TUMBLE_HEIGHT);
        }
        Pose->Tilt += (Tilt - Pose->Tilt) * Minimum(1.f, BODY_TILT_EASE * DeltaTime);

        KickUpDust(AppState, Entity, Pose, OnGround, DeltaTime);
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
    float Squash = BODY_SQUASH_DEPTH * Pose->Squash * Pose->Squash +
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
        Pose->FlinchSign * BODY_FLINCH_ANGLE * Pose->Flinch * Pose->Flinch;
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
