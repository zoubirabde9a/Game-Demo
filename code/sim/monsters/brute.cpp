/* Gravemaw Brute: a slow ogre with a bone pauldron. Its Ground Slam
   raises both fists overhead, then smashes the ground around it. Stand
   outside the ring or eat a hit that throws you back. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Brute)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Brute(monster_def *Def)
{
    Def->Name = "Gravemaw Brute";
    Def->MaxHp = 140.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 320.f;
    Def->StopRange = 40.f;
    Def->AttackRange = 52.f;
    Def->AttackDamage = 10.f;
    Def->AttackInterval = 1.1f;
    Def->SpawnWeight = 4;
    Def->FrameSize = 64;

    monster_ability *Slam = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Ground Slam");
    Slam->MaxRange = 75.f;
    Slam->Cooldown = 4.5f;
    Slam->Windup = 0.75f;
    Slam->Active = 0.25f;
    Slam->Recover = 0.9f;
    Slam->Damage = 22.f;
    Slam->Radius = 80.f;
    Slam->Knockback = 600.f;
}

#else

internal void
DrawMonster_Brute(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Skin = Ramp(ART_RGB(46, 58, 52), ART_RGB(78, 100, 80),
                           ART_RGB(112, 140, 102), ART_RGB(150, 176, 128));
    color_ramp Hide = Ramp(ART_RGB(52, 36, 32), ART_RGB(84, 56, 44),
                           ART_RGB(118, 80, 58), ART_RGB(150, 108, 76));
    color_ramp Bone = Ramp(ART_RGB(120, 108, 92), ART_RGB(178, 166, 140),
                           ART_RGB(222, 212, 186), ART_RGB(248, 244, 226));
    u32 Eye = ART_RGB(255, 196, 64);
    u32 Dust = ART_RGB(170, 150, 118);

    float Bob = 0.f;
    float Stride = 0.f;
    float Lean = 0.f;
    // NOTE(zoubir): fist positions, back fist then front fist
    v2 BackFist = V2(17.f, 46.f);
    v2 FrontFist = V2(50.f, 46.f);
    float HeadDrop = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Stride = 5.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            BackFist.X += 3.f * Pose.Wave;
            FrontFist.X -= 3.f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): both fists climb overhead while it leans back
            float Rise = Pose.t;
            Lean = -3.f * Rise;
            BackFist = Lerp2(BackFist, Rise, V2(30.f, 11.f));
            FrontFist = Lerp2(FrontFist, Rise, V2(42.f, 9.f));
        } break;

        case AnimationType_Attack:
        {
            Lean = 4.f;
            Bob = 2.f;
            HeadDrop = 2.f;
            BackFist = V2(30.f, 54.f);
            FrontFist = V2(52.f, 54.f);
        } break;

        case AnimationType_Stop:
        {
            Lean = 3.f;
            Bob = 2.f + 0.8f * Pose.Wave;
            HeadDrop = 3.f;
            BackFist = V2(22.f, 54.f);
            FrontFist = V2(48.f, 54.f);
        } break;

        default:
        {
            Bob = 0.8f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): back limbs first, a little darker
    FillLimb(Canvas, V2(28.f, 42.f + Bob), V2(25.f - Stride, 55.f), 6.f, 5.f,
             Hide, -0.2f);
    FillBlob(Canvas, 24.f - Stride, 56.f, 6.f, 3.f, Hide, -0.2f);
    FillLimb(Canvas, V2(24.f + Lean, 26.f + Bob), BackFist, 5.f, 4.f,
             Skin, -0.2f);
    FillBlob(Canvas, BackFist.X, BackFist.Y, 6.f, 5.5f, Skin, -0.2f);

    // NOTE(zoubir): torso and loincloth
    FillBlob(Canvas, 32.f + Lean * 0.5f, 34.f + Bob, 15.f, 14.f, Skin);
    FillBlob(Canvas, 34.f + Lean * 0.5f, 43.f + Bob, 11.f, 5.f, Hide);

    FillLimb(Canvas, V2(37.f, 42.f + Bob), V2(39.f + Stride, 55.f), 6.f, 5.f,
             Hide);
    FillBlob(Canvas, 40.f + Stride, 56.f, 6.5f, 3.f, Hide);

    // NOTE(zoubir): small head sunk between the shoulders
    float HeadX = 43.f + Lean;
    float HeadY = 20.f + Bob + HeadDrop;
    FillBlob(Canvas, HeadX, HeadY, 8.f, 7.f, Skin, 0.05f);
    FillBlob(Canvas, HeadX + 3.f, HeadY + 5.f, 6.f, 3.5f, Skin);
    FillTriangle(Canvas, V2(HeadX + 2.f, HeadY + 5.f), V2(HeadX + 4.f, HeadY - 1.f),
                 V2(HeadX + 5.f, HeadY + 5.f), Bone, 1.f, 0.6f);
    FillTriangle(Canvas, V2(HeadX + 6.f, HeadY + 5.f), V2(HeadX + 8.f, HeadY),
                 V2(HeadX + 9.f, HeadY + 5.f), Bone, 1.f, 0.6f);
    FillDot(Canvas, HeadX + 3.f, HeadY - 1.5f, 1.2f, Eye);
    FillDot(Canvas, HeadX + 7.f, HeadY - 1.5f, 1.f, Eye);
    // NOTE(zoubir): heavy brow
    FillLimb(Canvas, V2(HeadX - 1.f, HeadY - 4.f), V2(HeadX + 8.f, HeadY - 3.f),
             1.6f, 1.4f, Skin, -0.1f);

    // NOTE(zoubir): bone pauldron with a spike
    FillBlob(Canvas, 27.f + Lean, 22.f + Bob, 9.f, 6.f, Bone);
    FillTriangle(Canvas, V2(22.f + Lean, 20.f + Bob), V2(19.f + Lean, 10.f + Bob),
                 V2(27.f + Lean, 18.f + Bob), Bone, 1.f, 0.4f);

    // NOTE(zoubir): front arm and fist
    FillLimb(Canvas, V2(40.f + Lean, 28.f + Bob), FrontFist, 5.5f, 4.5f, Skin);
    FillBlob(Canvas, FrontFist.X, FrontFist.Y, 7.f, 6.f, Skin, 0.05f);
    FillDot(Canvas, FrontFist.X + 2.f, FrontFist.Y - 3.f, 1.f, Bone.C[2]);

    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): dust thrown out from the impact
        float Spread = 6.f + 16.f * Pose.t;
        for(u32 Puff = 0; Puff < 5; Puff++)
        {
            float Angle = Pi32 + Pi32 * (float)Puff / 4.f;
            float X = 40.f + Spread * Cos(Angle) * 1.4f;
            float Y = 57.f + 0.4f * Spread * Sin(Angle);
            FillDot(Canvas, X, Y, 2.5f - 1.5f * Pose.t, Dust);
        }
    }

    OutlineFrame(Canvas, ART_RGB(18, 16, 20));
}

#endif
