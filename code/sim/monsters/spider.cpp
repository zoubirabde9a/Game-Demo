/* Hexweaver Spider: lays traps, then picks you off. Web Snare lobs a web
   that sticks to the ground for five seconds and slows anyone inside it.
   Venom Spit fires a single barb that poisons. Webs are its zone: fight
   it outside them. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Spider)
#else

internal void
DefineMonster_Spider(monster_def *Def)
{
    Def->Name = "Hexweaver Spider";
    Def->MaxHp = 65.f;
    Def->Acceleration = 30000.f;
    Def->AggroRange = 420.f;
    Def->StopRange = 130.f;
    Def->AttackRange = 42.f;
    Def->AttackDamage = 7.f;
    Def->AttackInterval = 0.9f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 48;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.07f;

    // NOTE(zoubir): listed first so it opens with a web from range, then
    // spits while the web is on cooldown
    monster_ability *Snare = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                               "Web Snare");
    Snare->MinRange = 110.f;
    Snare->MaxRange = 340.f;
    Snare->Cooldown = 6.f;
    Snare->Windup = 0.8f;
    Snare->Active = 0.3f;
    Snare->Recover = 0.5f;
    Snare->Damage = 4.f;
    Snare->Radius = 42.f;
    Snare->Count = 1;
    Snare->Status = StatusEffect_Slowed;
    Snare->StatusSeconds = 1.f;
    Snare->HazardSeconds = 5.f;
    Snare->HazardStyle = HazardStyle_Web;

    monster_ability *Spit = AddMonsterAbility(Def, MonsterAbility_Volley,
                                              "Venom Spit");
    Spit->MinRange = 50.f;
    Spit->MaxRange = 300.f;
    Spit->Cooldown = 2.2f;
    Spit->Windup = 0.4f;
    Spit->Active = 1.2f;
    Spit->Recover = 0.3f;
    Spit->Damage = 6.f;
    Spit->Radius = 16.f;
    Spit->Speed = 320.f;
    Spit->Knockback = 60.f;
    Spit->Count = 1;
    Spit->ShotStyle = ShotStyle_Spine;
    Spit->Status = StatusEffect_Poisoned;
    Spit->StatusSeconds = 3.f;
}

// NOTE(zoubir): one leg as hip -> raised knee -> foot on the ground
internal void
DrawSpiderLeg(sprite_canvas *Canvas, v2 Hip, float Reach, float KneeLift,
              float FootShift, color_ramp Ramp0, float LightBias)
{
    v2 Foot = V2(Hip.X + Reach + FootShift, 41.f);
    v2 Knee = V2(Hip.X + 0.55f * (Reach + FootShift), Hip.Y - KneeLift);
    FillLimb(Canvas, Hip, Knee, 1.6f, 1.2f, Ramp0, LightBias);
    FillLimb(Canvas, Knee, Foot, 1.2f, 0.6f, Ramp0, LightBias);
}

internal void
DrawMonster_Spider(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Chitin = Ramp(ART_RGB(22, 16, 30), ART_RGB(46, 34, 62),
                             ART_RGB(78, 62, 100), ART_RGB(120, 104, 146));
    color_ramp Mark = Ramp(ART_RGB(120, 20, 40), ART_RGB(190, 40, 60),
                           ART_RGB(240, 80, 90), ART_RGB(255, 170, 160));
    color_ramp Fang = Ramp(ART_RGB(120, 110, 96), ART_RGB(190, 180, 160),
                           ART_RGB(230, 222, 200), ART_RGB(250, 246, 230));
    u32 Eye = ART_RGB(255, 60, 70);
    u32 Silk = ART_RGB(236, 236, 244);

    float Bob = 0.f;
    // NOTE(zoubir): how far the abdomen tips up, and the body lunge
    float Tilt = 0.f;
    float Lunge = 0.f;
    float Gait = 0.f;
    float Sprawl = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Gait = Pose.Wave;
            Bob = -0.8f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Tilt = 6.f * Pose.t;
            Lunge = -2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Tilt = 6.f * (1.f - Pose.t);
            Lunge = 3.f;
        } break;

        case AnimationType_Stop:
        {
            Sprawl = 3.f;
            Bob = 2.f;
        } break;

        default:
        {
            Bob = 0.5f * Pose.Wave;
        } break;
    }

    float BodyY = 30.f + Bob;
    v2 Thorax = V2(30.f + Lunge, BodyY);
    v2 Abdomen = V2(17.f + Lunge * 0.5f, BodyY - 3.f - Tilt);

    // NOTE(zoubir): far legs, darker; alternate pairs swing opposite ways
    float Reaches[4] = {16.f, 8.f, -4.f, -13.f};
    for(u32 Leg = 0; Leg < 4; Leg++)
    {
        float Phase = (Leg % 2) ? Gait : -Gait;
        DrawSpiderLeg(Canvas, Thorax + V2(-2.f, -1.f), Reaches[Leg] - 2.f + Sprawl,
                      9.f - Sprawl * 2.f, 2.5f * Phase, Chitin, -0.3f);
    }

    // NOTE(zoubir): abdomen with a glowing hex mark, then the head
    FillBlob(Canvas, Abdomen.X, Abdomen.Y, 11.f, 9.f, Chitin);
    FillTriangle(Canvas, V2(Abdomen.X - 4.f, Abdomen.Y - 5.f),
                 V2(Abdomen.X + 3.f, Abdomen.Y - 5.f),
                 V2(Abdomen.X - 0.5f, Abdomen.Y), Mark, 0.9f, 0.5f);
    FillTriangle(Canvas, V2(Abdomen.X - 0.5f, Abdomen.Y),
                 V2(Abdomen.X - 4.f, Abdomen.Y + 5.f),
                 V2(Abdomen.X + 3.f, Abdomen.Y + 5.f), Mark, 0.9f, 0.5f);
    FillBlob(Canvas, Thorax.X, Thorax.Y, 7.f, 6.f, Chitin, 0.05f);
    // NOTE(zoubir): near legs
    for(u32 Leg = 0; Leg < 4; Leg++)
    {
        float Phase = (Leg % 2) ? -Gait : Gait;
        DrawSpiderLeg(Canvas, Thorax + V2(-1.f, 3.f), Reaches[Leg] + Sprawl,
                      7.f - Sprawl * 1.5f, 2.5f * Phase, Chitin, 0.1f);
    }

    // NOTE(zoubir): head last, so the near legs never hide the eyes
    FillBlob(Canvas, Thorax.X + 6.f, Thorax.Y + 1.f, 4.5f, 4.f, Chitin, 0.1f);
    FillDot(Canvas, Thorax.X + 7.f, Thorax.Y - 1.5f, 1.1f, Eye);
    FillDot(Canvas, Thorax.X + 9.f, Thorax.Y - 0.5f, 0.9f, Eye);
    FillDot(Canvas, Thorax.X + 5.f, Thorax.Y - 2.f, 0.8f, Eye);
    // NOTE(zoubir): fangs part during the windup
    float FangOpen = Pose.Anim == AnimationType_Cast ? 1.5f * Pose.t : 0.f;
    FillLimb(Canvas, V2(Thorax.X + 9.f, Thorax.Y + 3.f),
             V2(Thorax.X + 10.f, Thorax.Y + 7.f + FangOpen), 1.f, 0.4f, Fang);
    FillLimb(Canvas, V2(Thorax.X + 7.f, Thorax.Y + 3.f),
             V2(Thorax.X + 7.5f - FangOpen, Thorax.Y + 7.f), 1.f, 0.4f, Fang);


    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): a strand trailing from the spinnerets
        v2 Spinneret = V2(Abdomen.X - 10.f, Abdomen.Y + 2.f);
        FillLimb(Canvas, Spinneret, Spinneret + V2(-6.f, -6.f + 3.f * Pose.t),
                 0.5f, 0.5f, Ramp(Silk, Silk, Silk, Silk));
    }

    OutlineFrame(Canvas, ART_RGB(10, 6, 14));
}

#endif
