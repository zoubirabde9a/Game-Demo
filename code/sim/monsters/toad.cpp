/* Bilecaller Toad: keeps its distance and shells you. Bile Barrage swells
   its throat sac while three landing spots light up on the ground, the
   first one placed where you are heading. Keep moving sideways. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Toad)
#else

internal void
DefineMonster_Toad(monster_def *Def)
{
    Def->Name = "Bilecaller Toad";
    Def->MaxHp = 70.f;
    Def->Acceleration = 20000.f;
    Def->AggroRange = 400.f;
    // NOTE(zoubir): stops well short of the player and fires from there
    Def->StopRange = 200.f;
    Def->AttackRange = 40.f;
    Def->AttackDamage = 6.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 48;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.09f;

    monster_ability *Barrage = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                                 "Bile Barrage");
    Barrage->MinRange = 90.f;
    Barrage->MaxRange = 380.f;
    Barrage->Cooldown = 4.5f;
    Barrage->Windup = 1.f;
    Barrage->Active = 0.35f;
    Barrage->Recover = 0.6f;
    Barrage->Damage = 14.f;
    Barrage->Radius = 34.f;
    Barrage->Count = 3;
    Barrage->Spread = 70.f;
    Barrage->Knockback = 150.f;
    // NOTE(zoubir): each shell leaves a bile puddle that poisons
    Barrage->Status = StatusEffect_Poisoned;
    Barrage->StatusSeconds = 2.5f;
    Barrage->HazardSeconds = 4.f;
    Barrage->HazardStyle = HazardStyle_Bile;
}

internal void
DrawMonster_Toad(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Skin = Ramp(ART_RGB(30, 48, 30), ART_RGB(56, 86, 44),
                           ART_RGB(92, 124, 56), ART_RGB(136, 162, 74));
    color_ramp Belly = Ramp(ART_RGB(120, 110, 52), ART_RGB(170, 160, 78),
                            ART_RGB(208, 198, 110), ART_RGB(232, 226, 150));
    color_ramp Wart = Ramp(ART_RGB(58, 30, 62), ART_RGB(92, 50, 96),
                           ART_RGB(128, 76, 128), ART_RGB(160, 110, 156));
    color_ramp Bile = Ramp(ART_RGB(60, 120, 30), ART_RGB(120, 200, 40),
                           ART_RGB(180, 240, 70), ART_RGB(230, 255, 150));
    u32 Eye = ART_RGB(250, 220, 60);
    u32 Pupil = ART_RGB(20, 16, 10);

    float Hop = 0.f;
    float Squash = 0.f;
    float Sac = 0.f;
    float SacGlow = 0.f;
    float Mouth = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            // NOTE(zoubir): one hop per loop, squashing on landing
            float Arc = Sin(Pi32 * Pose.t);
            Hop = -7.f * Arc;
            Squash = Arc < 0.25f ? 1.5f : -1.f;
        } break;

        case AnimationType_Cast:
        {
            Sac = 2.f + 6.f * Pose.t;
            SacGlow = Pose.t;
            Squash = -1.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Mouth = 1.f;
            Sac = 4.f * (1.f - Pose.t);
            SacGlow = 1.f - Pose.t;
            Squash = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Squash = 2.f + 0.5f * Pose.Wave;
        } break;

        default:
        {
            Sac = 1.5f + 1.f * Pose.Wave;
        } break;
    }

    float Y = 32.f + Hop + Squash;
    float Wide = 14.f + Squash;
    float Tall = 11.f - Squash;

    // NOTE(zoubir): far legs
    FillBlob(Canvas, 12.f, Y + 7.f, 5.f, 4.f, Skin, -0.25f);
    FillLimb(Canvas, V2(30.f, Y + 6.f), V2(32.f, 42.f + Squash), 2.5f, 2.f, Skin, -0.25f);

    // NOTE(zoubir): body, belly and haunch
    FillBlob(Canvas, 22.f, Y, Wide, Tall, Skin);
    FillBlob(Canvas, 27.f, Y + 4.f, Wide * 0.6f, Tall * 0.5f, Belly);
    FillBlob(Canvas, 13.f, Y + 5.f, 7.f, 6.f, Skin, 0.05f);
    FillBlob(Canvas, 11.f, Y + 11.f, 5.f, 2.f, Skin, -0.1f);

    // NOTE(zoubir): bile glands on the back, glowing harder while charging
    float GlowBias = -0.2f + 0.5f * SacGlow;
    FillBlob(Canvas, 15.f, Y - 9.f, 4.f, 3.5f, Bile, GlowBias);
    FillBlob(Canvas, 22.f, Y - 11.f, 3.5f, 3.f, Bile, GlowBias);
    FillBlob(Canvas, 9.f, Y - 5.f, 3.f, 2.5f, Bile, GlowBias);
    FillDot(Canvas, 19.f, Y - 3.f, 1.5f, Wart.C[2]);
    FillDot(Canvas, 12.f, Y + 1.f, 1.2f, Wart.C[1]);
    FillDot(Canvas, 26.f, Y - 5.f, 1.2f, Wart.C[2]);

    // NOTE(zoubir): head with bulging eyes
    FillBlob(Canvas, 32.f, Y - 3.f, 9.f, 7.f, Skin, 0.05f);
    FillBlob(Canvas, 30.f, Y - 10.f, 3.5f, 3.5f, Skin, 0.1f);
    FillBlob(Canvas, 36.f, Y - 9.f, 3.5f, 3.5f, Skin, 0.1f);
    FillDot(Canvas, 30.5f, Y - 10.5f, 2.f, Eye);
    FillDot(Canvas, 36.5f, Y - 9.5f, 2.f, Eye);
    FillLimb(Canvas, V2(31.f, Y - 11.5f), V2(31.f, Y - 9.5f), 0.5f, 0.5f,
             Ramp(Pupil, Pupil, Pupil, Pupil));
    FillLimb(Canvas, V2(37.f, Y - 10.5f), V2(37.f, Y - 8.5f), 0.5f, 0.5f,
             Ramp(Pupil, Pupil, Pupil, Pupil));

    // NOTE(zoubir): throat sac and mouth
    if (Sac > 0.f)
    {
        FillBlob(Canvas, 36.f, Y + 3.f + Sac * 0.3f, 3.f + Sac * 0.7f,
                 2.f + Sac * 0.6f, Bile, -0.1f + 0.4f * SacGlow);
    }
    if (Mouth > 0.f)
    {
        FillBlob(Canvas, 40.f, Y - 1.f, 3.5f, 2.5f, Wart, -0.3f);
        // NOTE(zoubir): globs of bile thrown up and forward
        for(u32 Glob = 0; Glob < 3; Glob++)
        {
            float Travel = Pose.t + 0.25f * Glob;
            float X = 41.f + 6.f * Travel;
            float GY = Y - 4.f - 18.f * Travel + 10.f * Travel * Travel;
            FillBlob(Canvas, X, GY, 2.f, 2.f, Bile, 0.3f);
        }
    }
    else
    {
        FillLimb(Canvas, V2(34.f, Y + 0.5f), V2(40.f, Y - 1.f), 0.5f, 0.5f,
                 Ramp(Skin.C[0], Skin.C[0], Skin.C[0], Skin.C[0]));
    }

    // NOTE(zoubir): near front leg
    FillLimb(Canvas, V2(28.f, Y + 6.f), V2(30.f, 43.f + Squash), 3.f, 2.f, Skin);
    FillBlob(Canvas, 31.f, 43.5f + Squash, 3.f, 1.5f, Skin, 0.1f);

    OutlineFrame(Canvas, ART_RGB(14, 22, 12));
}

#endif
