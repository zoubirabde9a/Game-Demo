/* Crevasse Crawler: a long centipede of glossy blue chitin from the Aurora
   Rift (docs/dungeon-rift.md) that lives in the cracks of the glacier.
   Fissure Lines rears up and slams the ice, splitting it along four
   strips toward its target (MonsterAbility_Lanes): ice falls on them
   from above, so jumping does not help; stand in a gap between two
   strips. From farther off it dives into a crevasse (MonsterAbility_Burrow)
   and erupts under its target once the ring locks: leave the ring.
   Only the rift's encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Crawler)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Crawler(monster_def *Def)
{
    Def->Name = "Crevasse Crawler";
    Def->MaxHp = 130.f;
    Def->Acceleration = 30000.f;
    Def->AggroRange = 420.f;
    Def->StopRange = 44.f;
    Def->AttackRange = 56.f;
    Def->AttackDamage = 11.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Special] = 4;
    Def->SecondsPerFrame[MonsterRow_Special] = 0.12f;

    // NOTE(zoubir): four strips 70 apart, so the target stands in a gap
    monster_ability *Lines = AddMonsterAbility(Def, MonsterAbility_Lanes,
                                               "Fissure Lines");
    Lines->MaxRange = 220.f;
    Lines->Cooldown = 6.f;
    Lines->Windup = 0.9f;
    Lines->Active = 0.4f;
    Lines->Recover = 0.6f;
    Lines->Damage = 12.f;
    Lines->Radius = 28.f;
    Lines->Speed = 300.f;
    Lines->Spread = 70.f;
    Lines->Count = 4;
    Lines->Status = StatusEffect_Slowed;
    Lines->StatusSeconds = 1.2f;

    monster_ability *Dive = AddMonsterAbility(Def, MonsterAbility_Burrow,
                                              "Crevasse Dive");
    Dive->MinRange = 150.f;
    Dive->MaxRange = 450.f;
    Dive->Cooldown = 9.f;
    Dive->Windup = 0.6f;
    Dive->Active = 1.6f;
    Dive->Recover = 0.8f;
    Dive->Damage = 14.f;
    Dive->Radius = 70.f;
    Dive->Knockback = 360.f;
}

#else

#define CRAWLER_SEGMENTS 9

internal void
DrawMonster_Crawler(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Chitin = Ramp(ART_RGB(16, 26, 64), ART_RGB(30, 58, 124),
                             ART_RGB(48, 96, 176), ART_RGB(140, 196, 250));
    color_ramp Frost = Ramp(ART_RGB(118, 146, 186), ART_RGB(178, 204, 230),
                            ART_RGB(224, 238, 250), ART_RGB(250, 253, 255));
    color_ramp Aurora = Ramp(ART_RGB(18, 90, 72), ART_RGB(40, 170, 120),
                             ART_RGB(110, 240, 170), ART_RGB(214, 255, 232));
    color_ramp Fang = Ramp(ART_RGB(46, 30, 88), ART_RGB(90, 62, 150),
                           ART_RGB(150, 112, 212), ART_RGB(214, 186, 250));

    float Ground = 56.f;

    if (Pose.Anim == AnimationType_JumpDown)
    {
        // NOTE(zoubir): underground: a ridge of cracked ice heaving along,
        // with the tips of its back spines cutting through
        float Heave = Pose.Wave;
        FillFlatEllipse(Canvas, 32.f, Ground - 1.f, 22.f, 2.5f, ART_RGB(40, 70, 110));
        FillBlob(Canvas, 32.f + Heave, Ground - 3.f, 18.f, 4.5f + 0.5f * Pose.Wave2, Frost, -0.1f);
        // NOTE(zoubir): cracks across the ridge
        for(u32 Crack = 0; Crack < 3; Crack++)
        {
            float X = 22.f + 10.f * Crack + Heave;
            FillLimb(Canvas, V2(X, Ground - 7.f), V2(X + 2.f, Ground - 1.f), 0.6f, 0.5f, Chitin, -0.5f);
        }
        for(u32 Spine = 0; Spine < 4; Spine++)
        {
            float X = 20.f + 7.f * Spine + 2.f * Heave;
            float Up = 3.f + 2.f * Absolute(Sin(Pi32 * (Pose.t + 0.25f * Spine)));
            FillTriangle(Canvas, V2(X - 2.f, Ground - 5.f), V2(X + 2.f, Ground - 5.f),
                         V2(X + 1.f, Ground - 6.f - Up), Chitin, 0.9f, 0.2f);
        }
        for(u32 Chunk = 0; Chunk < 4; Chunk++)
        {
            float X = 12.f + 13.f * Chunk + 2.f * Pose.Wave2;
            float Y = Ground - 6.f - 4.f * (float)((Chunk + Pose.Frame) % 3);
            FillBlob(Canvas, X, Y, 1.6f, 1.4f, Frost, 0.2f);
        }
        OutlineFrame(Canvas, ART_RGB(10, 16, 34));
        return;
    }

    // NOTE(zoubir): Rear lifts the front half (1 fully reared, below 0 the
    // head driven into the ice), Open spreads the mandibles, Glow lights
    // the spiracles, Ripple runs the wave down the legs
    float Rear = 0.f;
    float Open = 0.2f;
    float Glow = 0.35f + 0.15f * Pose.Wave;
    float Ripple = 0.f;
    float Stride = 1.f;
    float Bob = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Ripple = Pose.t;
            Stride = 3.f;
        } break;

        case AnimationType_Cast:
        {
            Rear = Pose.t;
            Open = 0.2f + 0.8f * Pose.t;
            Glow = 0.4f + 0.6f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Rear = 1.f - 1.3f * Minimum(1.f, 3.f * Pose.t);
            Open = 1.f - 0.6f * Minimum(1.f, 3.f * Pose.t);
            Glow = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Rear = -0.3f + 0.3f * Pose.t + 0.08f * Pose.Wave * (1.f - Pose.t);
            Open = 0.4f * (1.f - Pose.t);
            Glow = 1.f - 0.6f * Pose.t;
        } break;

        default:
        {
            Bob = 0.5f * Pose.Wave;
            Ripple = 0.1f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): segment centers from the tail on the left to the head
    v2 Segments[CRAWLER_SEGMENTS];
    float Radii[CRAWLER_SEGMENTS];
    for(u32 Segment = 0; Segment < CRAWLER_SEGMENTS; Segment++)
    {
        float s = (float)Segment;
        float X = 8.f + 5.2f * s;
        float Y = 48.f + Bob + 0.8f * Sin(2.f * Pi32 * Ripple - 0.9f * s);
        float u = Maximum(0.f, (s - 3.f) / 5.f);
        Y -= Rear * 24.f * u * u;
        X -= Maximum(0.f, Rear) * 6.f * u * u * u;
        Radii[Segment] = 4.f + 2.f * Minimum(1.f, s / 4.f);
        Y = Minimum(Y, Ground - Radii[Segment] + 2.f);
        Segments[Segment] = V2(X, Y);
    }

    // NOTE(zoubir): legs of one side, each stepping a little after the one
    // in front so a wave runs down the body; frost-white at the tips
    for(u32 Side = 0; Side < 2; Side++)
    {
        float Shade = Side ? 0.1f : -0.45f;
        float Lean = Side ? 1.f : -1.f;
        for(u32 Segment = 0; Segment < CRAWLER_SEGMENTS - 1; Segment++)
        {
            v2 P = Segments[Segment];
            float Phase = 2.f * Pi32 * Ripple - 0.9f * (float)Segment + (Side ? Pi32 : 0.f);
            float Swing = Stride * Sin(Phase);
            float Lift = Maximum(0.f, Cos(Phase)) * (Stride > 1.f ? 1.5f : 0.f);
            v2 Hip = P + V2(Lean, 1.5f);
            v2 Knee = P + V2(Lean + 0.5f * Swing + 2.f, Radii[Segment] - 2.f - Lift);
            v2 Foot = V2(P.X + Swing + 3.f, Minimum(Ground - Lift, P.Y + 10.f));
            if (P.Y < 42.f)
            {
                // NOTE(zoubir): reared segments let their legs paw the air
                Foot = P + V2(3.f + Swing + 1.5f * Pose.Wave2, 7.f);
                Knee = P + V2(3.f, 3.f);
            }
            FillLimb(Canvas, Hip, Knee, 1.f, 0.8f, Chitin, Shade);
            FillLimb(Canvas, Knee, Foot, 0.8f, 0.6f, Chitin, Shade);
            FillLimb(Canvas, Foot + V2(-0.5f, -2.f), Foot, 0.6f, 0.5f, Frost, Shade + 0.3f);
        }
        if (Side == 0)
        {
            // NOTE(zoubir): forked tail spines, then the body over the far legs
            v2 Tail = Segments[0];
            FillLimb(Canvas, Tail, Tail + V2(-6.f, -3.f + 0.5f * Pose.Wave), 1.2f, 0.4f, Frost, -0.2f);
            FillLimb(Canvas, Tail, Tail + V2(-6.f, 2.f), 1.2f, 0.4f, Frost, -0.3f);
            for(u32 Segment = 0; Segment < CRAWLER_SEGMENTS - 1; Segment++)
            {
                v2 P = Segments[Segment];
                float R = Radii[Segment];
                FillTriangle(Canvas, V2(P.X - 2.f, P.Y - R + 1.f), V2(P.X + 1.f, P.Y - R + 1.f),
                             V2(P.X - 1.5f, P.Y - R - 2.f), Frost, 0.9f, 0.1f);
                FillBlob(Canvas, P.X, P.Y, R, R * 0.85f, Chitin, -0.12f);
                FillDot(Canvas, P.X - 0.3f * R, P.Y - 0.45f * R, 0.9f, Chitin.C[3]);
            }
            for(u32 Segment = 0; Segment < CRAWLER_SEGMENTS - 1; Segment++)
            {
                // NOTE(zoubir): an aurora spiracle on each segment's side
                v2 P = Segments[Segment];
                float R = Radii[Segment];
                u32 Step = (u32)(Glow * 2.9f);
                FillDot(Canvas, P.X + 0.5f, P.Y + 0.3f * R, 1.1f + 0.6f * Glow, Aurora.C[Step + (Step < 3 ? 1 : 0)]);
                if (Glow > 0.7f)
                {
                    FillDot(Canvas, P.X + 0.5f, P.Y + 0.25f * R, 0.6f, Aurora.C[3]);
                }
            }
        }
    }

    // NOTE(zoubir): the head, with antennae, a green eye and violet
    // mandibles that gape as it rears
    v2 Head = Segments[CRAWLER_SEGMENTS - 1];
    float Sway = Pose.Wave * 1.5f;
    FillLimb(Canvas, Head + V2(1.f, -3.f), Head + V2(5.f, -11.f + Sway), 0.8f, 0.4f, Frost, -0.1f);
    FillLimb(Canvas, Head + V2(3.f, -3.f), Head + V2(9.f, -8.f - Sway), 0.8f, 0.4f, Frost, 0.1f);
    FillBlob(Canvas, Head.X + 1.f, Head.Y, 6.f, 5.f, Chitin, 0.2f);
    FillDot(Canvas, Head.X - 0.5f, Head.Y - 2.5f, 1.f, Chitin.C[3]);
    v2 Jaw = Head + V2(5.f, 2.f);
    v2 UpperTip = Jaw + V2(5.f, -1.f - 3.f * Open);
    v2 LowerTip = Jaw + V2(5.f, 2.f + 3.f * Open);
    FillLimb(Canvas, Jaw, UpperTip, 1.5f, 0.8f, Fang, 0.2f);
    FillLimb(Canvas, UpperTip, UpperTip + V2(-1.5f, 1.5f), 0.8f, 0.4f, Fang, 0.4f);
    FillLimb(Canvas, Jaw, LowerTip, 1.5f, 0.8f, Fang, 0.f);
    FillLimb(Canvas, LowerTip, LowerTip + V2(-1.5f, -1.5f), 0.8f, 0.4f, Fang, 0.2f);
    FillDot(Canvas, Head.X + 3.f, Head.Y - 1.5f, 1.2f + 0.6f * Glow, Aurora.C[2]);
    FillDot(Canvas, Head.X + 3.3f, Head.Y - 1.8f, 0.6f, Aurora.C[3]);

    if (Pose.Anim == AnimationType_Attack || Pose.Anim == AnimationType_Stop)
    {
        // NOTE(zoubir): ice thrown up where the head struck
        float Burst = (Pose.Anim == AnimationType_Attack) ? Pose.t : 1.f + Pose.t;
        for(u32 Chip = 0; Chip < 4; Chip++)
        {
            float Angle = Pi32 * (0.15f + 0.23f * Chip);
            float Reach = 4.f + 7.f * Burst;
            float X = Minimum(61.f, Head.X + 3.f + Reach * Cos(Angle));
            float Y = Ground - 2.f - Reach * Sin(Angle) + 3.f * Burst * Burst;
            if (Y < Ground)
            {
                FillBlob(Canvas, X, Y, 1.5f, 1.3f, Frost, 0.3f);
            }
        }
    }

    OutlineFrame(Canvas, ART_RGB(10, 16, 34));
}

#endif
