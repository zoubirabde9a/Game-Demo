/* Dune Lurker: a great segmented worm with a ring of hooked teeth. Tunnel
   Strike dives into the ground, where nothing can hurt it, and a ripple
   runs toward you. The ripple follows you at first; when the ring appears
   the spot is fixed and you have a moment to leave it before the worm
   bursts out. It is helpless while it shakes the sand off. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Lurker)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Lurker(monster_def *Def)
{
    Def->Name = "Dune Lurker";
    Def->MaxHp = 100.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 460.f;
    Def->StopRange = 40.f;
    Def->AttackRange = 46.f;
    Def->AttackDamage = 9.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 48;
    Def->FrameCounts[MonsterRow_Special] = 4;
    Def->SecondsPerFrame[MonsterRow_Special] = 0.12f;

    monster_ability *Tunnel = AddMonsterAbility(Def, MonsterAbility_Burrow,
                                                "Tunnel Strike");
    Tunnel->MinRange = 90.f;
    Tunnel->MaxRange = 420.f;
    Tunnel->Cooldown = 6.f;
    Tunnel->Windup = 0.6f;
    Tunnel->Active = 1.6f;
    Tunnel->Recover = 1.f;
    Tunnel->Damage = 22.f;
    Tunnel->Radius = 50.f;
    Tunnel->Knockback = 500.f;
}

#else

internal void
DrawMonster_Lurker(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Hide = Ramp(ART_RGB(70, 50, 34), ART_RGB(120, 88, 56),
                           ART_RGB(168, 128, 82), ART_RGB(210, 176, 120));
    color_ramp Belly = Ramp(ART_RGB(120, 90, 70), ART_RGB(176, 140, 110),
                            ART_RGB(212, 184, 150), ART_RGB(236, 216, 186));
    color_ramp Maw = Ramp(ART_RGB(60, 10, 20), ART_RGB(110, 24, 36),
                          ART_RGB(160, 50, 56), ART_RGB(200, 90, 90));
    color_ramp Tooth = Ramp(ART_RGB(170, 160, 140), ART_RGB(214, 206, 186),
                            ART_RGB(236, 230, 214), ART_RGB(252, 250, 240));
    color_ramp Sand = Ramp(ART_RGB(120, 96, 60), ART_RGB(170, 140, 90),
                           ART_RGB(206, 180, 124), ART_RGB(232, 214, 166));

    if (Pose.Anim == AnimationType_JumpDown)
    {
        // NOTE(zoubir): underground: only a churning mound of sand shows
        float Churn = Pose.Wave;
        FillBlob(Canvas, 24.f, 40.f, 12.f + Churn, 4.f, Sand, -0.1f);
        for(u32 Grain = 0; Grain < 4; Grain++)
        {
            float X = 14.f + 6.f * Grain + 2.f * Pose.Wave2;
            float Y = 36.f - 2.f * (float)((Grain + Pose.Frame) % 3);
            FillDot(Canvas, X, Y, 1.f, Sand.C[3 - (Grain % 2)]);
        }
        OutlineFrame(Canvas, ART_RGB(40, 30, 20));
        return;
    }

    // NOTE(zoubir): how far up the body rears, how far its head leans
    // forward, and how wide the maw opens
    float Rear = 0.f;
    float Lean = 0.f;
    float Open = 0.2f;
    float Sink = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Rear = 2.f * Pose.Wave;
            Lean = 2.f * Pose.Wave2;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): arching over and diving head first
            Lean = 6.f * Pose.t;
            Sink = 14.f * Pose.t;
            Open = 0.2f;
        } break;

        case AnimationType_Attack:
        {
            Rear = 4.f;
            Lean = 4.f * Pose.t;
            Open = 1.f - 0.5f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            // NOTE(zoubir): just erupted: rearing high, maw wide, shedding sand
            Rear = 6.f - 2.f * Pose.t;
            Open = 1.f;
        } break;

        default:
        {
            Rear = 1.5f * Pose.Wave;
            Open = 0.3f + 0.2f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): five segments from the tail on the ground up to the
    // head, each a little bigger, along a gentle S
    v2 Segments[5];
    float Radii[5] = {5.f, 6.5f, 7.5f, 8.5f, 9.f};
    for(u32 Segment = 0; Segment < 5; Segment++)
    {
        float t = (float)Segment / 4.f;
        float X = 10.f + 22.f * t + Lean * t * t;
        float Y = 40.f - (14.f + Rear) * t + 3.f * Sin(Pi32 * t + Pose.Wave) + Sink * t;
        Segments[Segment] = V2(X, Y);
    }
    // NOTE(zoubir): sand piled where the body leaves the ground
    FillBlob(Canvas, 12.f, 42.f, 9.f, 3.f, Sand, -0.15f);
    for(u32 Segment = 0; Segment < 5; Segment++)
    {
        v2 P = Segments[Segment];
        if (P.Y > 44.f)
        {
            continue;
        }
        FillBlob(Canvas, P.X, P.Y, Radii[Segment], Radii[Segment] * 0.9f, Hide);
        FillBlob(Canvas, P.X + 1.f, P.Y + Radii[Segment] * 0.45f,
                 Radii[Segment] * 0.6f, Radii[Segment] * 0.35f, Belly, -0.1f);
        // NOTE(zoubir): a ridge plate on each segment's back
        FillTriangle(Canvas, V2(P.X - 3.f, P.Y - Radii[Segment] + 1.f),
                     V2(P.X - 1.f, P.Y - Radii[Segment] - 3.f),
                     V2(P.X + 2.f, P.Y - Radii[Segment] + 1.f), Hide, 0.9f, 0.4f);
    }

    // NOTE(zoubir): the head is the last segment seen from the front: a
    // round maw ringed with hooked teeth
    v2 Head = Segments[4];
    if (Head.Y <= 44.f)
    {
        float MawR = 3.f + 3.5f * Open;
        v2 Mouth = Head + V2(3.f, 0.f);
        FillBlob(Canvas, Mouth.X, Mouth.Y, MawR, MawR, Maw, -0.3f);
        u32 Teeth = 8;
        for(u32 ToothIndex = 0; ToothIndex < Teeth; ToothIndex++)
        {
            float Angle = 2.f * Pi32 * (float)ToothIndex / (float)Teeth + 0.2f;
            v2 Out = V2(Cos(Angle), Sin(Angle));
            v2 Base = Mouth + (MawR + 0.8f) * Out;
            v2 Tip = Mouth + (MawR - 2.f) * Out;
            FillLimb(Canvas, Base, Tip, 1.f, 0.3f, Tooth);
        }
    }

    if (Pose.Anim == AnimationType_Stop)
    {
        // NOTE(zoubir): sand raining off the body
        for(u32 Grain = 0; Grain < 5; Grain++)
        {
            float X = 14.f + 5.f * Grain;
            float Y = 18.f + 20.f * Pose.t + 4.f * (float)(Grain % 2);
            FillDot(Canvas, X, Y, 1.f, Sand.C[2]);
        }
    }

    OutlineFrame(Canvas, ART_RGB(30, 20, 14));
}

#endif
