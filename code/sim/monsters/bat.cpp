/* Duskwing: a fast bat that hovers out of sword reach. Swoop folds its
   wings, then dives in a straight line at whoever it locked onto. Side-step
   the line and it overshoots. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Bat)
#else

internal void
DefineMonster_Bat(monster_def *Def)
{
    Def->Name = "Duskwing";
    Def->MaxHp = 40.f;
    Def->Acceleration = 52000.f;
    Def->AggroRange = 420.f;
    Def->StopRange = 20.f;
    Def->AttackRange = 36.f;
    Def->AttackDamage = 4.f;
    Def->AttackInterval = 0.6f;
    Def->FlyHeight = 20.f;
    Def->SpawnWeight = 3;
    Def->FrameSize = 40;
    Def->SecondsPerFrame[MonsterRow_Idle] = 0.07f;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.06f;

    monster_ability *Swoop = AddMonsterAbility(Def, MonsterAbility_Charge,
                                               "Swoop");
    Swoop->MinRange = 70.f;
    Swoop->MaxRange = 220.f;
    Swoop->Cooldown = 3.5f;
    Swoop->Windup = 0.45f;
    Swoop->Active = 0.4f;
    Swoop->Recover = 0.5f;
    Swoop->Damage = 10.f;
    Swoop->Radius = 26.f;
    Swoop->Speed = 560.f;
    Swoop->Knockback = 250.f;
}

internal void
DrawMonster_Bat(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Fur = Ramp(ART_RGB(36, 22, 48), ART_RGB(70, 42, 88),
                          ART_RGB(108, 70, 128), ART_RGB(150, 110, 168));
    color_ramp Membrane = Ramp(ART_RGB(40, 18, 40), ART_RGB(84, 34, 70),
                               ART_RGB(128, 58, 98), ART_RGB(170, 92, 126));
    color_ramp Bone = Ramp(ART_RGB(50, 30, 56), ART_RGB(90, 60, 100),
                           ART_RGB(130, 96, 140), ART_RGB(170, 140, 176));
    u32 Eye = ART_RGB(255, 70, 70);
    u32 EyeHot = ART_RGB(255, 220, 160);
    u32 Fang = ART_RGB(240, 236, 220);

    v2 Body = V2(20.f, 22.f);
    float Flap = Pose.Wave;
    float Stretch = 0.f;
    bool32 Hot = false;

    switch(Pose.Anim)
    {
        case AnimationType_Cast:
        {
            // NOTE(zoubir): wings pulled high and back, about to drop
            Flap = 1.f;
            Body.Y -= 2.f * Pose.t;
            Hot = Pose.Frame % 2 == 1;
        } break;

        case AnimationType_Attack:
        {
            Flap = 0.6f;
            Stretch = 4.f;
            Hot = true;
        } break;

        case AnimationType_Stop:
        {
            Flap = -0.7f + 0.3f * Pose.Wave;
            Body.Y += 2.f;
        } break;

        default:
        {
        } break;
    }

    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): wind streaks behind the dive
        for(u32 Streak = 0; Streak < 3; Streak++)
        {
            float Y = Body.Y - 4.f + 4.f * Streak;
            float Length0 = 6.f + 3.f * ((Streak + Pose.Frame) % 3);
            FillLimb(Canvas, V2(2.f, Y), V2(2.f + Length0, Y), 0.5f, 0.5f,
                     Ramp(ART_RGB(150, 140, 170), ART_RGB(190, 180, 210),
                          ART_RGB(220, 214, 236), ART_RGB(240, 236, 250)));
        }
    }

    // NOTE(zoubir): far wing (left) is darker, near wing (right) brighter;
    // both sit under the body so the face stays clear
    float Lift = Flap;
    DrawMembraneWing(Canvas, V2(Body.X - 2.f, Body.Y - 2.f), -1.f, Lift,
                13.f - Stretch,
                Ramp(Membrane.C[0], Membrane.C[0], Membrane.C[1], Membrane.C[2]),
                Bone);
    DrawMembraneWing(Canvas, V2(Body.X + 3.f, Body.Y - 2.f), 1.f, Lift,
                14.f - Stretch, Membrane, Bone);

    // NOTE(zoubir): body, ears and face
    FillBlob(Canvas, Body.X + Stretch * 0.5f, Body.Y, 6.f + Stretch * 0.5f,
             7.f - Stretch * 0.3f, Fur);
    FillTriangle(Canvas, V2(Body.X + 1.f, Body.Y - 5.f), V2(Body.X + 2.f, Body.Y - 13.f),
                 V2(Body.X + 5.f, Body.Y - 5.f), Fur, 0.9f, 0.4f);
    FillTriangle(Canvas, V2(Body.X + 4.f, Body.Y - 5.f), V2(Body.X + 8.f, Body.Y - 12.f),
                 V2(Body.X + 8.f, Body.Y - 4.f), Fur, 0.8f, 0.3f);
    FillBlob(Canvas, Body.X + 4.f + Stretch, Body.Y - 2.f, 4.5f, 4.f, Fur, 0.1f);
    FillDot(Canvas, Body.X + 5.5f + Stretch, Body.Y - 3.f, 1.f, Hot ? EyeHot : Eye);
    FillDot(Canvas, Body.X + 3.f + Stretch, Body.Y - 3.f, 0.9f, Hot ? EyeHot : Eye);
    PutPixel(Canvas, (i32)(Body.X + 4.f + Stretch), (i32)(Body.Y + 1.f), Fang);
    PutPixel(Canvas, (i32)(Body.X + 6.f + Stretch), (i32)(Body.Y + 1.f), Fang);
    // NOTE(zoubir): little feet tucked under
    FillLimb(Canvas, V2(Body.X - 1.f, Body.Y + 6.f), V2(Body.X - 2.f, Body.Y + 9.f),
             0.8f, 0.6f, Bone);
    FillLimb(Canvas, V2(Body.X + 2.f, Body.Y + 6.f), V2(Body.X + 2.f, Body.Y + 9.f),
             0.8f, 0.6f, Bone);

    OutlineFrame(Canvas, ART_RGB(16, 8, 20));
}

#endif
