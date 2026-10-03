/* Carapace Warden: a beetle knight behind a horned shell. Hits from the
   front glance off its shell (FrontArmor), and it turns slowly
   (TurnRate), so the answer is to get round behind it. A bright arc on
   the ground shows the side its shell covers. Shell Bash is a short
   shoulder charge that throws you back into the open. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Warden)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Warden(monster_def *Def)
{
    Def->Name = "Carapace Warden";
    Def->MaxHp = 120.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 340.f;
    Def->StopRange = 40.f;
    Def->AttackRange = 48.f;
    Def->AttackDamage = 9.f;
    Def->AttackInterval = 1.1f;
    // NOTE(zoubir): 0 until TestPlayOverBadConnection (server_tests.cpp)
    // runs in an empty arena; it counts fireballs and changes result with
    // the random monster mix. Then 2
    Def->SpawnWeight = 0;
    Def->FrameSize = 48;
    Def->FrontArmor = 0.8f;
    Def->FrontArcDegrees = 120.f;
    // NOTE(zoubir): about two seconds for a full half turn
    Def->TurnRate = 1.6f;

    monster_ability *Bash = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Shell Bash");
    Bash->MinRange = 60.f;
    Bash->MaxRange = 200.f;
    Bash->Cooldown = 4.f;
    Bash->Windup = 0.6f;
    Bash->Active = 0.35f;
    Bash->Recover = 0.7f;
    Bash->Damage = 14.f;
    Bash->Radius = 40.f;
    Bash->Speed = 460.f;
    Bash->Knockback = 900.f;
}

#else

internal void
DrawMonster_Warden(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Shell = Ramp(ART_RGB(20, 40, 44), ART_RGB(30, 76, 78),
                            ART_RGB(50, 120, 112), ART_RGB(130, 200, 180));
    color_ramp Bronze = Ramp(ART_RGB(70, 44, 20), ART_RGB(130, 86, 36),
                             ART_RGB(190, 136, 60), ART_RGB(240, 200, 120));
    color_ramp Under = Ramp(ART_RGB(26, 22, 24), ART_RGB(46, 40, 42),
                            ART_RGB(70, 62, 62), ART_RGB(96, 88, 86));
    u32 Eye = ART_RGB(255, 210, 80);

    float Bob = 0.f;
    float Step = 0.f;
    float Lunge = 0.f;
    float Hunch = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = Pose.Wave;
            Bob = -0.8f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): rears back behind the shell, horn lowered
            Lunge = -3.f * Pose.t;
            Hunch = 3.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Lunge = 4.f;
            Hunch = 2.f;
            Step = 0.8f * Pose.Wave;
        } break;

        case AnimationType_Stop:
        {
            Bob = 1.5f;
            Hunch = -1.f;
        } break;

        default:
        {
            Bob = 0.5f * Pose.Wave;
        } break;
    }

    float BodyY = 34.f + Bob;
    // NOTE(zoubir): six legs, three a side; far side darker
    for(u32 Pair = 0; Pair < 3; Pair++)
    {
        float X = 15.f + 9.f * Pair + Lunge * 0.5f;
        float Swing = ((Pair % 2) ? 1.f : -1.f) * 2.5f * Step;
        FillLimb(Canvas, V2(X, BodyY + 2.f), V2(X - 3.f + Swing, 41.f), 1.6f, 1.1f,
                 Under, -0.3f);
    }

    // NOTE(zoubir): domed shell with a bronze rim and a seam down its back
    v2 Dome = V2(21.f + Lunge * 0.5f, BodyY - 4.f + Hunch * 0.3f);
    FillBlob(Canvas, Dome.X, Dome.Y + 4.f, 14.f, 5.f, Bronze, -0.1f);
    FillBlob(Canvas, Dome.X, Dome.Y, 13.f, 10.f, Shell);
    FillLimb(Canvas, V2(Dome.X - 9.f, Dome.Y - 3.f), V2(Dome.X + 9.f, Dome.Y - 4.f),
             0.5f, 0.5f, Ramp(Shell.C[0], Shell.C[0], Shell.C[0], Shell.C[0]));
    FillFlatEllipse(Canvas, Dome.X - 4.f, Dome.Y - 6.f, 4.f, 1.5f, Shell.C[3]);

    // NOTE(zoubir): head under a bronze faceplate with a great horn
    v2 Head = V2(35.f + Lunge, BodyY - 1.f + Hunch);
    FillBlob(Canvas, Head.X, Head.Y, 6.f, 5.5f, Under, 0.1f);
    FillBlob(Canvas, Head.X + 1.f, Head.Y - 1.5f, 5.f, 4.f, Bronze, 0.05f);
    FillLimb(Canvas, V2(Head.X + 3.f, Head.Y - 3.f),
             V2(Head.X + 9.f, Head.Y - 9.f + Hunch), 2.f, 0.6f, Bronze, 0.1f);
    FillDot(Canvas, Head.X + 3.f, Head.Y + 1.f, 1.f, Eye);

    // NOTE(zoubir): near legs over the rim
    for(u32 Pair = 0; Pair < 3; Pair++)
    {
        float X = 17.f + 9.f * Pair + Lunge * 0.5f;
        float Swing = ((Pair % 2) ? -1.f : 1.f) * 2.5f * Step;
        FillLimb(Canvas, V2(X, BodyY + 3.f), V2(X + 1.f + Swing, 42.f), 1.8f, 1.2f,
                 Under, 0.f);
        FillBlob(Canvas, X + 1.f + Swing, 42.f, 1.6f, 0.9f, Bronze, -0.2f);
    }

    if (Pose.Anim == AnimationType_Stop)
    {
        // NOTE(zoubir): steam venting from the shell seams
        FillDot(Canvas, Dome.X - 6.f, Dome.Y - 9.f - 3.f * Pose.t, 1.4f,
                ART_RGB(200, 210, 210));
    }

    OutlineFrame(Canvas, ART_RGB(10, 16, 18));
}

#endif
