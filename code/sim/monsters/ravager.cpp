/* Tuskback Ravager: a boar the size of a cart. Gore Rush paws the ground,
   then charges along a line drawn on the ground. If the line ends in a
   wall, the ravager smashes into it and stays dazed for a long time. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Ravager)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Ravager(monster_def *Def)
{
    Def->Name = "Tuskback Ravager";
    Def->MaxHp = 110.f;
    Def->Acceleration = 30000.f;
    Def->AggroRange = 380.f;
    Def->StopRange = 40.f;
    Def->AttackRange = 50.f;
    Def->AttackDamage = 8.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 56;
    Def->FrameCounts[MonsterRow_Attack] = 6;
    Def->SecondsPerFrame[MonsterRow_Attack] = 0.06f;

    monster_ability *Rush = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Gore Rush");
    Rush->MinRange = 90.f;
    Rush->MaxRange = 330.f;
    Rush->Cooldown = 5.f;
    Rush->Windup = 0.85f;
    Rush->Active = 0.7f;
    Rush->Recover = 0.8f;
    Rush->Damage = 26.f;
    Rush->Radius = 40.f;
    Rush->Speed = 620.f;
    Rush->Knockback = 700.f;
}

#else

internal void
DrawMonster_Ravager(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Fur = Ramp(ART_RGB(54, 30, 22), ART_RGB(96, 54, 34),
                          ART_RGB(140, 84, 48), ART_RGB(182, 120, 70));
    color_ramp Mane = Ramp(ART_RGB(24, 18, 20), ART_RGB(44, 34, 36),
                           ART_RGB(70, 56, 56), ART_RGB(100, 84, 80));
    color_ramp Tusk = Ramp(ART_RGB(150, 136, 110), ART_RGB(204, 192, 160),
                           ART_RGB(236, 228, 204), ART_RGB(255, 252, 238));
    color_ramp Hoof = Ramp(ART_RGB(20, 16, 18), ART_RGB(40, 34, 34),
                           ART_RGB(62, 54, 52), ART_RGB(84, 76, 72));
    u32 Eye = ART_RGB(255, 60, 40);
    u32 Steam = ART_RGB(220, 220, 228);
    u32 Star = ART_RGB(255, 230, 90);

    float Bob = 0.f;
    float HeadDrop = 0.f;
    float Reach = 0.f;
    // NOTE(zoubir): leg swing, front pair and back pair
    float FrontSwing = 0.f;
    float BackSwing = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            FrontSwing = 4.f * Pose.Wave;
            BackSwing = -4.f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): head down, one front hoof scraping
            HeadDrop = 4.f;
            FrontSwing = (Pose.Frame % 2) ? -5.f : 3.f;
            Bob = 1.f;
        } break;

        case AnimationType_Attack:
        {
            // NOTE(zoubir): full gallop, legs thrown wide
            HeadDrop = 3.f;
            Reach = 3.f;
            FrontSwing = 7.f * Pose.Wave;
            BackSwing = 7.f * Pose.Wave2;
            Bob = -2.f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Stop:
        {
            HeadDrop = 5.f;
            Bob = 2.f;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
        } break;
    }

    if (Pose.Anim == AnimationType_Attack)
    {
        for(u32 Streak = 0; Streak < 3; Streak++)
        {
            float Y = 28.f + 7.f * Streak;
            float Length0 = 5.f + 4.f * ((Streak + Pose.Frame) % 3);
            FillLimb(Canvas, V2(1.f, Y), V2(1.f + Length0, Y), 0.6f, 0.6f,
                     Ramp(ART_RGB(170, 150, 120), ART_RGB(200, 184, 150),
                          ART_RGB(226, 214, 186), ART_RGB(244, 238, 220)));
        }
    }

    // NOTE(zoubir): far legs
    FillLimb(Canvas, V2(16.f, 38.f + Bob), V2(14.f + BackSwing, 49.f), 3.5f, 2.5f,
             Fur, -0.25f);
    FillBlob(Canvas, 14.f + BackSwing, 50.f, 2.5f, 1.5f, Hoof);
    FillLimb(Canvas, V2(34.f + Reach, 38.f + Bob), V2(36.f + Reach + FrontSwing, 49.f),
             3.5f, 2.5f, Fur, -0.25f);
    FillBlob(Canvas, 36.f + Reach + FrontSwing, 50.f, 2.5f, 1.5f, Hoof);

    // NOTE(zoubir): barrel body with a hump over the shoulders
    FillBlob(Canvas, 25.f + Reach * 0.5f, 34.f + Bob, 16.f + Reach * 0.5f, 10.f, Fur);
    FillBlob(Canvas, 30.f + Reach, 27.f + Bob, 10.f, 8.f, Fur, 0.05f);
    // NOTE(zoubir): spiked mane along the spine
    for(u32 Spike = 0; Spike < 6; Spike++)
    {
        float X = 14.f + 4.f * Spike + Reach * 0.7f;
        float Base = 25.f + Bob + (Spike < 3 ? 1.f : -1.5f) - (Spike == 4 ? 1.f : 0.f);
        float Height = 5.f + (Spike >= 3 && Spike <= 4 ? 3.f : 0.f);
        FillTriangle(Canvas, V2(X - 2.5f, Base + 2.f), V2(X - 1.f, Base - Height),
                     V2(X + 2.5f, Base + 2.f), Mane, 0.9f, 0.3f);
    }
    // NOTE(zoubir): stubby tail
    FillLimb(Canvas, V2(10.f, 31.f + Bob), V2(6.f, 28.f + Bob), 1.5f, 1.f, Mane);

    // NOTE(zoubir): head and snout
    float HeadX = 42.f + Reach;
    float HeadY = 32.f + Bob + HeadDrop;
    FillBlob(Canvas, HeadX, HeadY, 8.f, 7.f, Fur, 0.05f);
    FillBlob(Canvas, HeadX + 7.f, HeadY + 3.f, 4.f, 3.5f, Fur, 0.1f);
    FillDot(Canvas, HeadX + 10.f, HeadY + 2.5f, 0.8f, Mane.C[0]);
    FillTriangle(Canvas, V2(HeadX - 4.f, HeadY - 5.f), V2(HeadX - 3.f, HeadY - 11.f),
                 V2(HeadX, HeadY - 5.f), Fur, 0.8f, 0.3f);
    FillDot(Canvas, HeadX + 3.f, HeadY - 2.f, 1.2f, Eye);
    // NOTE(zoubir): curled tusks
    FillLimb(Canvas, V2(HeadX + 6.f, HeadY + 5.f), V2(HeadX + 11.f, HeadY + 1.f),
             1.6f, 1.f, Tusk);
    FillLimb(Canvas, V2(HeadX + 11.f, HeadY + 1.f), V2(HeadX + 11.f, HeadY - 3.f),
             1.f, 0.5f, Tusk);

    // NOTE(zoubir): near legs
    FillLimb(Canvas, V2(18.f, 39.f + Bob), V2(18.f - BackSwing, 50.f), 4.f, 3.f, Fur);
    FillBlob(Canvas, 18.f - BackSwing, 51.f, 3.f, 1.8f, Hoof);
    FillLimb(Canvas, V2(36.f + Reach, 39.f + Bob), V2(38.f + Reach - FrontSwing, 50.f),
             4.f, 3.f, Fur);
    FillBlob(Canvas, 38.f + Reach - FrontSwing, 51.f, 3.f, 1.8f, Hoof);

    if (Pose.Anim == AnimationType_Cast)
    {
        // NOTE(zoubir): angry snorts
        float Puff = (float)(Pose.Frame % 2);
        FillDot(Canvas, HeadX + 13.f + 2.f * Puff, HeadY + 5.f - Puff, 1.5f + Puff, Steam);
    }
    if (Pose.Anim == AnimationType_Stop)
    {
        // NOTE(zoubir): dazed stars circling the head
        for(u32 StarIndex = 0; StarIndex < 3; StarIndex++)
        {
            float Angle = 2.f * Pi32 * (Pose.t + (float)StarIndex / 3.f);
            float X = HeadX + 7.f * Cos(Angle);
            float Y = HeadY - 12.f + 2.5f * Sin(Angle);
            FillDot(Canvas, X, Y, 1.3f, Star);
        }
    }

    OutlineFrame(Canvas, ART_RGB(20, 12, 10));
}

#endif
