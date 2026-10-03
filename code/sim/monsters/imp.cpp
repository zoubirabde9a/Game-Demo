/* Cinder Imp: a small fire devil that keeps out of sword reach. Cinder
   Fan holds a growing ember overhead while three lanes light up on the
   ground, then throws three embers along them. Walls and trees block the
   embers, and stepping between two lanes dodges the whole fan. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Imp)
#else

internal void
DefineMonster_Imp(monster_def *Def)
{
    Def->Name = "Cinder Imp";
    Def->MaxHp = 45.f;
    Def->Acceleration = 34000.f;
    Def->AggroRange = 420.f;
    Def->StopRange = 170.f;
    Def->AttackRange = 38.f;
    Def->AttackDamage = 5.f;
    Def->AttackInterval = 0.7f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 40;
    Def->SecondsPerFrame[MonsterRow_Idle] = 0.12f;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.07f;

    monster_ability *Fan = AddMonsterAbility(Def, MonsterAbility_Volley,
                                             "Cinder Fan");
    Fan->MinRange = 80.f;
    Fan->MaxRange = 360.f;
    Fan->Cooldown = 3.f;
    Fan->Windup = 0.6f;
    Fan->Active = 1.3f;
    Fan->Recover = 0.4f;
    Fan->Damage = 9.f;
    Fan->Radius = 18.f;
    Fan->Speed = 280.f;
    Fan->Knockback = 120.f;
    Fan->Count = 3;
    Fan->Spread = 36.f;
    Fan->ShotStyle = ShotStyle_Ember;
    Fan->Status = StatusEffect_Burning;
    Fan->StatusSeconds = 1.5f;
}

internal void
DrawMonster_Imp(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Skin = Ramp(ART_RGB(90, 20, 22), ART_RGB(160, 40, 34),
                           ART_RGB(214, 76, 44), ART_RGB(250, 130, 70));
    color_ramp Horn = Ramp(ART_RGB(40, 28, 26), ART_RGB(76, 58, 50),
                           ART_RGB(120, 100, 84), ART_RGB(170, 150, 126));
    color_ramp Wing = Ramp(ART_RGB(50, 14, 22), ART_RGB(90, 26, 34),
                           ART_RGB(130, 44, 46), ART_RGB(170, 70, 60));
    color_ramp Fire = Ramp(ART_RGB(170, 40, 14), ART_RGB(240, 110, 20),
                           ART_RGB(255, 190, 50), ART_RGB(255, 246, 180));
    u32 Eye = ART_RGB(255, 236, 90);
    u32 Smoke = ART_RGB(110, 100, 104);

    float Bob = 0.f;
    float Step = 0.f;
    // NOTE(zoubir): the throwing hand, and how big the ember in it is
    v2 Hand = V2(27.f, 22.f);
    float Ember = 1.5f;
    float WingLift = 0.3f * Pose.Wave;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
            WingLift = 0.6f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): hand climbs overhead while the ember swells
            Hand = Lerp2(V2(27.f, 22.f), Pose.t, V2(24.f, 12.f));
            Ember = 2.f + 2.f * Pose.t;
            WingLift = 0.8f;
        } break;

        case AnimationType_Attack:
        {
            // NOTE(zoubir): overhand throw, the ember leaves on frame 1
            Hand = Lerp2(V2(20.f, 12.f), Pose.t, V2(33.f, 24.f));
            Ember = Pose.t < 0.34f ? 4.f : 0.f;
            WingLift = -0.4f;
            Bob = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Hand = V2(28.f, 26.f);
            Ember = 0.f;
            Bob = 1.f + 0.5f * Pose.Wave;
            WingLift = -0.5f;
        } break;

        default:
        {
            Bob = 0.8f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): leathery wings behind everything
    DrawMembraneWing(Canvas, V2(15.f, 17.f + Bob), -1.f, 0.4f + WingLift,
                     10.f, Ramp(Wing.C[0], Wing.C[0], Wing.C[1], Wing.C[2]),
                     Horn);
    DrawMembraneWing(Canvas, V2(18.f, 16.f + Bob), -1.f, 0.7f + WingLift,
                     11.f, Wing, Horn);

    // NOTE(zoubir): tail ending in a flame
    v2 TailTip = V2(5.f, 30.f + Bob + Pose.Wave);
    FillLimb(Canvas, V2(15.f, 27.f + Bob), V2(9.f, 31.f + Bob), 1.6f, 1.2f, Skin, -0.1f);
    FillLimb(Canvas, V2(9.f, 31.f + Bob), TailTip, 1.2f, 0.8f, Skin, -0.1f);
    FillBlob(Canvas, TailTip.X - 0.5f, TailTip.Y - 2.f, 2.f, 2.6f, Fire, 0.2f);

    // NOTE(zoubir): legs, back one darker
    FillLimb(Canvas, V2(17.f, 28.f + Bob), V2(16.f - Step, 35.f), 2.f, 1.5f, Skin, -0.25f);
    FillLimb(Canvas, V2(22.f, 28.f + Bob), V2(23.f + Step, 35.f), 2.2f, 1.6f, Skin);
    FillBlob(Canvas, 16.5f - Step, 35.5f, 2.2f, 1.2f, Horn);
    FillBlob(Canvas, 24.f + Step, 35.5f, 2.4f, 1.2f, Horn);

    // NOTE(zoubir): pot belly and chest
    FillBlob(Canvas, 19.5f, 24.f + Bob, 6.5f, 6.f, Skin);
    FillBlob(Canvas, 21.f, 26.f + Bob, 3.5f, 3.f, Fire, -0.35f);

    // NOTE(zoubir): oversized head with swept horns and a grin
    float HeadX = 22.f;
    float HeadY = 14.f + Bob;
    FillLimb(Canvas, V2(HeadX - 3.f, HeadY - 4.f), V2(HeadX - 8.f, HeadY - 11.f),
             1.8f, 0.5f, Horn);
    FillLimb(Canvas, V2(HeadX + 2.f, HeadY - 5.f), V2(HeadX + 1.f, HeadY - 12.f),
             1.8f, 0.5f, Horn);
    FillBlob(Canvas, HeadX, HeadY, 6.f, 5.5f, Skin, 0.05f);
    FillTriangle(Canvas, V2(HeadX + 4.f, HeadY - 2.f), V2(HeadX + 9.f, HeadY - 4.f),
                 V2(HeadX + 5.f, HeadY + 1.f), Skin, 0.8f, 0.4f);
    FillDot(Canvas, HeadX + 3.f, HeadY - 1.f, 1.1f, Eye);
    FillDot(Canvas, HeadX + 0.f, HeadY - 1.f, 0.9f, Eye);
    FillLimb(Canvas, V2(HeadX - 1.f, HeadY + 2.5f), V2(HeadX + 4.f, HeadY + 2.f),
             0.5f, 0.5f, Ramp(Horn.C[0], Horn.C[0], Horn.C[0], Horn.C[0]));
    PutPixel(Canvas, (i32)(HeadX + 1.f), (i32)(HeadY + 3.f), Horn.C[3]);

    // NOTE(zoubir): throwing arm and the ember it holds
    v2 Shoulder = V2(23.f, 21.f + Bob);
    FillLimb(Canvas, Shoulder, Hand, 1.8f, 1.3f, Skin, 0.05f);
    if (Ember > 0.f)
    {
        FillBlob(Canvas, Hand.X + 1.f, Hand.Y - Ember * 0.6f, Ember, Ember, Fire, 0.2f);
        FillDot(Canvas, Hand.X + 1.5f, Hand.Y - Ember * 0.8f, Ember * 0.4f, Fire.C[3]);
    }

    if (Pose.Anim == AnimationType_Stop)
    {
        // NOTE(zoubir): smoke curling off the spent hand
        float Rise = Pose.t;
        FillDot(Canvas, Hand.X + 2.f, Hand.Y - 3.f - 6.f * Rise, 1.5f + Rise, Smoke);
    }

    OutlineFrame(Canvas, ART_RGB(30, 8, 8));
}

#endif
