/* UpdateBodyPoses (body_pose.cpp): once a frame, for every unit, reads
   what changed since the last frame (speed, height, health, facing,
   status) and moves its pose: stretch, squash, pop, lean, flinch, tumble,
   sway, breath, then the dust at its feet. */

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
    AppState->BodyPoses->NudgeLeft =
        Maximum(0.f, AppState->BodyPoses->NudgeLeft - DeltaTime);
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
            StartBodyHit(Pose, Entity, SpeedX, Index);
            FeelLandedHit(AppState, Entity, Pose->LastHp - Entity->Hp);
        }
        Pose->LastHp = Entity->Hp;
        // NOTE(zoubir): frozen, nothing wears off; the body is still
        // followed, so it does not seem to move fast once it lets go
        Pose->Hold = Maximum(0.f, Pose->Hold - DeltaTime);
        Pose->Frozen = Entity->HitStop > 0.f || Pose->Hold > 0.f;
        if (Pose->Frozen)
        {
            Pose->LastX = Entity->Position.X;
            Pose->LastY = Entity->Position.Y;
            Pose->LastZ = Entity->Position.Z;
            continue;
        }
        Pose->Flash = Maximum(0.f, Pose->Flash - DeltaTime / BODY_HIT_FLASH_SECONDS);
        Pose->Flinch = Maximum(0.f, Pose->Flinch - DeltaTime / BODY_FLINCH_SECONDS);
        Pose->Swing = Maximum(0.f, Pose->Swing - DeltaTime / BODY_SWING_SECONDS);
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
        // NOTE(zoubir): what the hit says when it is known, else a guess
        bool32 Thrown = Entity->HitFresh > 0.f ? Entity->HitThrown :
            Pose->KnockLeft > 0.f;
        if (!OnGround && !Pose->Tumbling &&
            (Thrown || HasStatus(Entity, StatusEffect_Stunned)))
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

        Pose->Fall = HasStatus(Entity, StatusEffect_Falling) ?
            Minimum(1.f, Pose->Fall + DeltaTime / BODY_FALL_SECONDS) : 0.f;

        KickUpDust(AppState, Entity, Pose, OnGround, DeltaTime);
    }
}
