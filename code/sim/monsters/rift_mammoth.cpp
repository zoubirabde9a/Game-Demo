/* Rimetusk Mammoth: a woolly mammoth of the Aurora Rift
   (docs/dungeon-rift.md), the biggest thing in the rift that is not a
   boss, with tusks of glacier ice and icicles caught in its fur. It rears
   up and marks the player farthest from it with Avalanche Stomp
   (MonsterAbility_Share): the blow is split between everyone standing in
   the circle, so the party gathers on the marked player to cut it into
   shares, while one player caught alone takes all of it and is slowed.
   Close up it shakes its tusks into Tusk Tremor (MonsterAbility_Wave):
   two rings of frost roll out through walls, each jumped as it reaches
   you. Only the rift's encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Mammoth)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Mammoth(monster_def *Def)
{
    Def->Name = "Rimetusk Mammoth";
    Def->MaxHp = 230.f;
    Def->Acceleration = 20000.f;
    Def->AggroRange = 380.f;
    Def->StopRange = 50.f;
    Def->AttackRange = 64.f;
    Def->AttackDamage = 16.f;
    Def->AttackInterval = 1.4f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 72;

    // NOTE(zoubir): a long windup so the party has time to gather on the
    // marked player and split the blow
    monster_ability *Stomp = AddMonsterAbility(Def, MonsterAbility_Share,
                                               "Avalanche Stomp");
    Stomp->MaxRange = 380.f;
    Stomp->Cooldown = 13.f;
    Stomp->Windup = 2.2f;
    Stomp->Active = 0.4f;
    Stomp->Recover = 1.f;
    Stomp->Damage = 13.f;
    Stomp->Radius = 85.f;
    Stomp->Status = StatusEffect_Slowed;
    Stomp->StatusSeconds = 1.5f;

    // NOTE(zoubir): two rings 70 apart out to 240; 320 * 1 s covers 310
    monster_ability *Tremor = AddMonsterAbility(Def, MonsterAbility_Wave,
                                                "Tusk Tremor");
    Tremor->MaxRange = 160.f;
    Tremor->Cooldown = 7.f;
    Tremor->Windup = 1.f;
    Tremor->Active = 1.f;
    Tremor->Recover = 0.8f;
    Tremor->Damage = 12.f;
    Tremor->Radius = 240.f;
    Tremor->Speed = 320.f;
    Tremor->Spread = 70.f;
    Tremor->Count = 2;
    Tremor->Knockback = 280.f;
}

#else

// NOTE(zoubir): turns P round Pivot so a positive Angle lifts the front
inline v2
MammothTilt(v2 P, v2 Pivot, float Angle)
{
    v2 D = P - Pivot;
    float C = Cos(Angle);
    float S = Sin(Angle);
    v2 Result = Pivot + V2(D.X * C + D.Y * S, -D.X * S + D.Y * C);
    return Result;
}

// NOTE(zoubir): a long tusk of glacier ice curving down, forward and up
// again, built from four tapering pieces
internal void
DrawMammothTusk(sprite_canvas *Canvas, v2 Base, v2 Neck, float Nod, v2 Pivot, float Angle,
                color_ramp Ice, float LightBias)
{
    v2 P0 = MammothTilt(MammothTilt(Base, Neck, Nod), Pivot, Angle);
    v2 P1 = MammothTilt(MammothTilt(Base + V2(3.f, 7.f), Neck, Nod), Pivot, Angle);
    v2 P2 = MammothTilt(MammothTilt(Base + V2(9.f, 9.f), Neck, Nod), Pivot, Angle);
    v2 P3 = MammothTilt(MammothTilt(Base + V2(13.f, 5.f), Neck, Nod), Pivot, Angle);
    v2 P4 = MammothTilt(MammothTilt(Base + V2(13.5f, -1.f), Neck, Nod), Pivot, Angle);
    FillLimb(Canvas, P0, P1, 2.8f, 2.5f, Ice, LightBias);
    FillLimb(Canvas, P1, P2, 2.5f, 2.f, Ice, LightBias + 0.1f);
    FillLimb(Canvas, P2, P3, 2.f, 1.4f, Ice, LightBias + 0.2f);
    FillLimb(Canvas, P3, P4, 1.4f, 0.6f, Ice, LightBias + 0.3f);
}

// NOTE(zoubir): a lock of long hair hanging from Top, drawn as a narrow
// triangle so the edge of the coat looks ragged
internal void
DrawMammothLock(sprite_canvas *Canvas, v2 Top, float Width, float Hang, float Swing,
                v2 Pivot, float Angle, color_ramp Fur, float Light)
{
    v2 A = MammothTilt(Top + V2(-Width, 0.f), Pivot, Angle);
    v2 B = MammothTilt(Top + V2(Width, 0.f), Pivot, Angle);
    v2 C = MammothTilt(Top + V2(Swing - 0.5f * Width, Hang), Pivot, Angle);
    FillTriangle(Canvas, A, B, C, Fur, Light, Light - 0.25f);
}

internal void
DrawMonster_Mammoth(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Fur = Ramp(ART_RGB(46, 36, 34), ART_RGB(84, 66, 58),
                          ART_RGB(124, 104, 92), ART_RGB(166, 152, 140));
    color_ramp Skin = Ramp(ART_RGB(36, 34, 46), ART_RGB(62, 60, 76),
                           ART_RGB(94, 92, 110), ART_RGB(132, 132, 150));
    color_ramp Ice = Ramp(ART_RGB(50, 110, 160), ART_RGB(110, 186, 224),
                          ART_RGB(184, 234, 250), ART_RGB(240, 252, 255));
    color_ramp Aurora = Ramp(ART_RGB(60, 40, 140), ART_RGB(110, 90, 220),
                             ART_RGB(90, 230, 180), ART_RGB(220, 255, 236));

    float Bob = 0.f;
    float Step = 0.f;
    float StepLift = 0.f;
    // NOTE(zoubir): how far it rears onto its hind legs, in radians
    float Rear = 0.f;
    // NOTE(zoubir): 0 trunk hanging, 1 trunk thrown up over its head
    float TrunkUp = 0.f;
    float TrunkSway = 0.f;
    float Shake = 0.f;
    float Glow = 0.f;
    float Puff = 0.f;
    float Tail = 0.f;
    // NOTE(zoubir): the head tipping up (positive) or down at the neck
    float Nod = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 4.f * Pose.Wave;
            StepLift = 2.f;
            Bob = -1.f * Absolute(Pose.Wave2);
            TrunkSway = 2.f * Pose.Wave2;
            Tail = Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            Rear = 0.3f * Minimum(1.f, 1.4f * Pose.t);
            TrunkUp = Minimum(1.f, 1.6f * Pose.t);
            Nod = 0.2f * TrunkUp;
            Glow = Pose.t;
            Bob = -1.f * Pose.t;
            Tail = -1.f;
        } break;

        case AnimationType_Attack:
        {
            float Slam = Minimum(1.f, 3.f * Pose.t);
            Rear = 0.22f * (1.f - Slam) - 0.04f * Slam;
            Bob = 3.f * Slam;
            TrunkUp = 0.6f * (1.f - Slam);
            TrunkSway = 2.f * Slam;
            Nod = 0.12f * (1.f - Slam) - 0.1f * Slam;
            Glow = 1.f - Pose.t;
            Puff = Slam + 0.7f * Pose.t;
            Tail = -0.5f;
        } break;

        case AnimationType_Stop:
        {
            // NOTE(zoubir): the head tosses the other way every frame
            float Swing = ((Pose.Frame % 2) ? -1.f : 1.f) * (1.f - 0.4f * Pose.t);
            Shake = 2.f * Swing;
            Nod = 0.2f * Swing;
            TrunkSway = -5.f * Swing;
            Bob = 2.f * (1.f - Pose.t);
            Puff = Maximum(0.f, 0.7f - Pose.t);
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            TrunkSway = 1.5f * Pose.Wave2;
            Tail = 0.5f * Pose.Wave;
            Nod = 0.04f * Pose.Wave2;
        } break;
    }

    float Ground = 63.f;
    float BodyY = 37.f + Bob;
    // NOTE(zoubir): it rears about its hips, so the rump sinks a little
    // while the shoulders, head and front legs go up
    v2 Pivot = V2(18.f, BodyY + 7.f);

    // NOTE(zoubir): legs move in diagonal pairs; the front feet go up with
    // the body when it rears and never sink through the floor
    float LiftA = StepLift * Maximum(0.f, Pose.Wave);
    float LiftB = StepLift * Maximum(0.f, -Pose.Wave);
    v2 FarHindFoot = V2(14.f + Step, Ground - 2.f - LiftA);
    v2 NearHindFoot = V2(21.f - Step, Ground - 2.f - LiftB);
    v2 FarFrontFoot = MammothTilt(V2(40.f - Step, Ground - 2.f - LiftB), Pivot, Rear);
    v2 NearFrontFoot = MammothTilt(V2(47.f + Step, Ground - 2.f - LiftA), Pivot, Rear);
    FarFrontFoot.Y = Minimum(FarFrontFoot.Y, Ground - 2.f);
    NearFrontFoot.Y = Minimum(NearFrontFoot.Y, Ground - 2.f);

    // NOTE(zoubir): snow thrown up round the front feet by the stomp
    if (Puff > 0.f)
    {
        float Wide = 5.f + 9.f * Puff;
        FillFlatEllipse(Canvas, 44.f, Ground - 0.5f, Wide, 1.5f + Puff, ART_RGB(196, 226, 246));
    }

    // NOTE(zoubir): the far legs, in shadow
    v2 FarHindHip = MammothTilt(V2(14.f, BodyY + 6.f), Pivot, Rear);
    v2 FarFrontHip = MammothTilt(V2(40.f, BodyY + 6.f), Pivot, Rear);
    FillLimb(Canvas, FarHindHip, FarHindFoot, 6.f, 5.f, Fur, -0.5f);
    FillBlob(Canvas, FarHindFoot.X, FarHindFoot.Y + 0.5f, 5.5f, 2.f, Skin, -0.4f);
    FillLimb(Canvas, FarFrontHip, FarFrontFoot, 6.f, 5.f, Fur, -0.5f);
    FillBlob(Canvas, FarFrontFoot.X, FarFrontFoot.Y + 0.5f, 5.5f, 2.f, Skin, -0.4f);

    // NOTE(zoubir): the tail, a tuft at its end
    v2 TailRoot = MammothTilt(V2(6.f, BodyY - 2.f), Pivot, Rear);
    v2 TailTip = MammothTilt(V2(4.f - 1.f * Tail, BodyY + 7.f - 4.f * Tail), Pivot, Rear);
    FillLimb(Canvas, TailRoot, TailTip, 1.5f, 1.f, Fur, -0.3f);
    FillBlob(Canvas, TailTip.X, TailTip.Y, 2.f, 2.5f, Fur, -0.4f);

    // NOTE(zoubir): the far tusk, behind the head
    v2 HeadLocal = V2(52.f + 0.5f * Shake, BodyY - 8.f);
    v2 Neck = HeadLocal + V2(-6.f, 2.f);
    DrawMammothTusk(Canvas, HeadLocal + V2(0.f, 6.f), Neck, Nod, Pivot, Rear, Ice, -0.45f);

    // NOTE(zoubir): a huge body: low rump, barrel, and a high hump over
    // the shoulders that the back slopes down from
    v2 Rump = MammothTilt(V2(15.f, BodyY + 1.f), Pivot, Rear);
    v2 Barrel = MammothTilt(V2(29.f, BodyY), Pivot, Rear);
    v2 Hump = MammothTilt(V2(39.f, BodyY - 9.f), Pivot, Rear);
    FillBlob(Canvas, Rump.X, Rump.Y, 10.f, 12.f, Fur, -0.2f);
    FillBlob(Canvas, Barrel.X, Barrel.Y, 19.f, 14.f, Fur, 0.f);
    FillBlob(Canvas, Hump.X, Hump.Y, 13.f, 11.f, Fur, 0.15f);

    // NOTE(zoubir): strands of shag down the flank
    for(u32 Strand = 0; Strand < 7; Strand++)
    {
        float X = 14.f + 5.f * (float)Strand;
        float Y = BodyY - 6.f + 3.f * (float)(Strand % 3) - (Strand >= 4 ? 4.f : 0.f);
        v2 A = MammothTilt(V2(X, Y), Pivot, Rear);
        v2 B = MammothTilt(V2(X - 2.f, Y + 7.f), Pivot, Rear);
        FillLimb(Canvas, A, B, 0.6f, 0.5f, Fur, -0.6f);
    }

    // NOTE(zoubir): frost crusted on the back, a ridge of ice spikes
    for(u32 Spike = 0; Spike < 4; Spike++)
    {
        float X = 21.f + 6.f * (float)Spike;
        float Y = BodyY - 13.f - (Spike >= 2 ? 6.f + 1.f * (float)(Spike - 2) : 2.5f * (float)Spike);
        v2 A = MammothTilt(V2(X - 2.f, Y + 2.f), Pivot, Rear);
        v2 B = MammothTilt(V2(X + 2.f, Y + 2.f), Pivot, Rear);
        v2 C = MammothTilt(V2(X - 1.f, Y - 3.f - (float)(Spike % 2)), Pivot, Rear);
        FillTriangle(Canvas, A, B, C, Ice, 0.4f, 1.f);
    }
    v2 FrostA = MammothTilt(V2(25.f, BodyY - 10.f), Pivot, Rear);
    v2 FrostB = MammothTilt(V2(39.f, BodyY - 18.f), Pivot, Rear);
    FillBlob(Canvas, FrostA.X, FrostA.Y, 4.f, 1.5f, Ice, 0.6f);
    FillBlob(Canvas, FrostB.X, FrostB.Y, 4.5f, 1.5f, Ice, 0.6f);

    // NOTE(zoubir): the near legs, short thick pillars with pale toenails
    v2 NearHindHip = MammothTilt(V2(20.f, BodyY + 7.f), Pivot, Rear);
    v2 NearFrontHip = MammothTilt(V2(46.f, BodyY + 7.f), Pivot, Rear);
    FillLimb(Canvas, NearHindHip, NearHindFoot, 6.5f, 5.5f, Fur, -0.05f);
    FillBlob(Canvas, NearHindFoot.X, NearHindFoot.Y + 0.5f, 6.f, 2.f, Skin, 0.1f);
    FillLimb(Canvas, NearFrontHip, NearFrontFoot, 6.5f, 5.5f, Fur, 0.05f);
    FillBlob(Canvas, NearFrontFoot.X, NearFrontFoot.Y + 0.5f, 6.f, 2.f, Skin, 0.1f);
    for(u32 Nail = 0; Nail < 3; Nail++)
    {
        float NX = -1.f + 3.f * (float)Nail;
        FillDot(Canvas, NearHindFoot.X + NX, NearHindFoot.Y + 1.f, 0.9f, ART_RGB(214, 206, 190));
        FillDot(Canvas, NearFrontFoot.X + NX, NearFrontFoot.Y + 1.f, 0.9f, ART_RGB(214, 206, 190));
    }

    // NOTE(zoubir): the long coat hangs in a ragged skirt over the tops of
    // the legs, with icicles frozen into it
    for(u32 Lock = 0; Lock < 9; Lock++)
    {
        float X = 8.f + 5.f * (float)Lock;
        float Hang = 7.f + 3.f * (float)(Lock % 2) + (Lock == 4 ? 2.f : 0.f);
        float Swing = 0.4f * TrunkSway - 1.f - 0.5f * Bob;
        float Top = BodyY + 7.f + ((Lock == 0 || Lock == 8) ? -3.f : 0.f);
        DrawMammothLock(Canvas, V2(X, Top), 3.f, Hang, Swing, Pivot, Rear, Fur,
                        (Lock % 2) ? 0.3f : 0.45f);
    }
    for(u32 Icicle = 0; Icicle < 3; Icicle++)
    {
        float X = 15.f + 11.f * (float)Icicle;
        v2 A = MammothTilt(V2(X - 1.5f, BodyY + 12.f), Pivot, Rear);
        v2 B = MammothTilt(V2(X + 1.5f, BodyY + 12.f), Pivot, Rear);
        v2 C = MammothTilt(V2(X, BodyY + 18.f - (float)(Icicle % 2)), Pivot, Rear);
        FillTriangle(Canvas, A, B, C, Ice, 0.5f, 1.f);
    }

    // NOTE(zoubir): clumps of snow kicked up in front of the feet
    if (Puff > 0.f)
    {
        float Wide = 5.f + 9.f * Puff;
        for(u32 Clump = 0; Clump < 4; Clump++)
        {
            float Side = (Clump % 2) ? 1.f : -1.f;
            float Far = (float)(Clump / 2);
            float Reach = (0.5f + 0.25f * Far) * Wide;
            float Rise = 2.f + 4.f * Puff * (1.f - 0.3f * Far);
            FillBlob(Canvas, 44.f + Side * Reach, Ground - Rise, 2.f + Puff, 1.5f + Puff, Ice, 0.6f);
        }
    }

    // NOTE(zoubir): the head, a high domed crown, a small ear and an eye
    // that burns aurora green as the stomp builds; a beard of hair under it
    v2 Head = MammothTilt(MammothTilt(HeadLocal, Neck, Nod), Pivot, Rear);
    v2 Crown = MammothTilt(MammothTilt(HeadLocal + V2(-2.f, -8.f), Neck, Nod), Pivot, Rear);
    v2 Ear = MammothTilt(MammothTilt(HeadLocal + V2(-6.f, 1.f), Neck, Nod), Pivot, Rear);
    v2 Eye = MammothTilt(MammothTilt(HeadLocal + V2(3.5f, -2.f), Neck, Nod), Pivot, Rear);
    FillBlob(Canvas, Head.X, Head.Y, 8.f, 9.f, Fur, 0.2f);
    FillBlob(Canvas, Crown.X, Crown.Y, 7.f, 6.f, Fur, 0.35f);
    FillBlob(Canvas, Ear.X, Ear.Y, 3.5f, 5.f, Fur, -0.35f);
    DrawMammothLock(Canvas, HeadLocal + V2(-3.f, 6.f), 3.f, 8.f, -1.f + 0.4f * TrunkSway,
                    Pivot, Rear, Fur, 0.2f);
    FillBlob(Canvas, Crown.X - 1.f, Crown.Y - 3.f, 3.f, 1.2f, Ice, 0.6f);
    if (Glow > 0.f)
    {
        FillDot(Canvas, Eye.X, Eye.Y, 1.5f + 1.5f * Glow, ART_RGB(90, 230, 180));
    }
    FillDot(Canvas, Eye.X, Eye.Y, 1.2f, Glow > 0.3f ? ART_RGB(230, 255, 240) : ART_RGB(20, 16, 24));

    // NOTE(zoubir): the trunk hangs and sways, and is thrown up high
    // before the stomp; three pieces that taper to the tip
    v2 Base = HeadLocal + V2(6.f, 2.f);
    v2 HangA = Base + V2(3.f + 0.3f * TrunkSway, 8.f);
    v2 HangB = HangA + V2(1.f + TrunkSway, 9.f);
    v2 HangC = HangB + V2(2.5f + 0.5f * TrunkSway, 3.f);
    v2 UpA = Base + V2(6.f, -4.f);
    v2 UpB = UpA + V2(2.f, -8.f);
    v2 UpC = UpB + V2(-3.f, -3.f);
    v2 TrunkRoot = MammothTilt(MammothTilt(Base, Neck, Nod), Pivot, Rear);
    v2 TrunkA = MammothTilt(MammothTilt(HangA + TrunkUp * (UpA - HangA), Neck, Nod), Pivot, Rear);
    v2 TrunkB = MammothTilt(MammothTilt(HangB + TrunkUp * (UpB - HangB), Neck, Nod), Pivot, Rear);
    v2 TrunkC = MammothTilt(MammothTilt(HangC + TrunkUp * (UpC - HangC), Neck, Nod), Pivot, Rear);
    FillLimb(Canvas, TrunkRoot, TrunkA, 4.f, 3.f, Fur, 0.15f);
    FillLimb(Canvas, TrunkA, TrunkB, 3.f, 2.2f, Fur, 0.15f);
    FillLimb(Canvas, TrunkB, TrunkC, 2.2f, 1.6f, Fur, 0.25f);

    // NOTE(zoubir): the near tusk, lit up with aurora while it winds up
    DrawMammothTusk(Canvas, HeadLocal + V2(2.f, 6.f), Neck, Nod, Pivot, Rear,
                    Glow > 0.45f ? Aurora : Ice, 0.1f);

    OutlineFrame(Canvas, ART_RGB(14, 16, 30));
}

#endif
