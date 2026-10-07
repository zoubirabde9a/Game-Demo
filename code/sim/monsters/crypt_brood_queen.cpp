/* The Brood Queen: the Sunken Crypt's second boss (sim/dungeon/,
   docs/dungeon-plan.md), in the Brood Nest. A spider the size of a cart,
   her swollen abdomen ridged with pale egg sacs, a crest of horns over a
   cluster of red eyes. Never roams (SpawnWeight 0).

   Calm:    Web Nova slams the ground round her and leaves webs that slow
            anyone in them; Venom Rain throws a fan of four poisoned barbs.
   Enraged (below half health): faster, flushed red, and Hatch: two
            Hexweaver Spiders crawl out of the sacs, four at most.
   The dungeon adds two more spiders at half health
   (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(BroodQueen)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_BroodQueen(monster_def *Def)
{
    Def->Name = "The Brood Queen";
    Def->MaxHp = 720.f;
    Def->Acceleration = 24000.f;
    Def->AggroRange = 640.f;
    Def->StopRange = 50.f;
    Def->AttackRange = 62.f;
    Def->AttackDamage = 13.f;
    Def->AttackInterval = 1.1f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.08f;

    Def->EnrageHpShare = 0.5f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFF8080FF;

    monster_ability *Hatch = AddMonsterAbility(Def, MonsterAbility_Summon,
                                               "Hatch");
    Hatch->MaxRange = 520.f;
    Hatch->Cooldown = 10.f;
    Hatch->Windup = 1.1f;
    Hatch->Active = 0.3f;
    Hatch->Recover = 0.6f;
    Hatch->SummonKind = MonsterKind_Spider;
    Hatch->Count = 2;
    Hatch->MaxActive = 4;
    Hatch->Spread = 50.f;
    Hatch->Radius = 16.f;
    Hatch->PhaseMask = PHASE_ENRAGED;

    monster_ability *Nova = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Web Nova");
    Nova->MaxRange = 100.f;
    Nova->Cooldown = 6.f;
    Nova->Windup = 0.9f;
    Nova->Active = 0.3f;
    Nova->Recover = 0.8f;
    Nova->Damage = 20.f;
    Nova->Radius = 110.f;
    Nova->Knockback = 450.f;
    Nova->Status = StatusEffect_Slowed;
    Nova->StatusSeconds = 2.f;
    Nova->HazardSeconds = 5.f;
    Nova->HazardStyle = HazardStyle_Web;

    monster_ability *Rain = AddMonsterAbility(Def, MonsterAbility_Volley,
                                              "Venom Rain");
    Rain->MinRange = 90.f;
    Rain->MaxRange = 420.f;
    Rain->Cooldown = 4.f;
    Rain->Windup = 0.7f;
    Rain->Active = 1.4f;
    Rain->Recover = 0.5f;
    Rain->Damage = 9.f;
    Rain->Radius = 16.f;
    Rain->Speed = 300.f;
    Rain->Knockback = 80.f;
    Rain->Count = MAX_ABILITY_POINTS;
    Rain->Spread = 60.f;
    Rain->ShotStyle = ShotStyle_Spine;
    Rain->Status = StatusEffect_Poisoned;
    Rain->StatusSeconds = 4.f;
}

#else

// NOTE(zoubir): one leg as hip -> high knee -> foot on the ground line
internal void
DrawQueenLeg(sprite_canvas *Canvas, v2 Hip, float Reach, float KneeLift,
             float FootShift, color_ramp Ramp0, float LightBias)
{
    v2 Foot = V2(Hip.X + Reach + FootShift, 56.f);
    v2 Knee = V2(Hip.X + 0.6f * (Reach + FootShift), Hip.Y - KneeLift);
    FillLimb(Canvas, Hip, Knee, 2.2f, 1.6f, Ramp0, LightBias);
    FillLimb(Canvas, Knee, Foot, 1.6f, 0.7f, Ramp0, LightBias);
}

internal void
DrawMonster_BroodQueen(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Chitin = Ramp(ART_RGB(20, 14, 26), ART_RGB(42, 30, 54),
                             ART_RGB(72, 54, 88), ART_RGB(110, 92, 130));
    color_ramp Belly = Ramp(ART_RGB(40, 22, 38), ART_RGB(72, 40, 62),
                            ART_RGB(104, 62, 86), ART_RGB(140, 92, 112));
    color_ramp Egg = Ramp(ART_RGB(110, 120, 70), ART_RGB(160, 172, 104),
                          ART_RGB(200, 210, 140), ART_RGB(236, 240, 196));
    color_ramp Fang = Ramp(ART_RGB(150, 140, 110), ART_RGB(200, 192, 160),
                           ART_RGB(230, 226, 200), ART_RGB(250, 248, 232));
    color_ramp Mark = Ramp(ART_RGB(120, 20, 40), ART_RGB(190, 40, 60),
                           ART_RGB(240, 80, 90), ART_RGB(255, 170, 160));
    color_ramp Venom = Ramp(ART_RGB(40, 120, 30), ART_RGB(90, 190, 50),
                            ART_RGB(160, 240, 90), ART_RGB(220, 255, 180));

    float Bob = 0.f;
    float Step = 0.f;
    float Rear = 0.f;
    float Pulse = 0.f;
    float Fangs = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.5f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): rears back on her hind legs, sacs swelling
            Rear = 6.f * Pose.t;
            Pulse = Pose.t;
            Fangs = Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Rear = 6.f * (1.f - Pose.t);
            Bob = 2.f * Pose.t;
            Fangs = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Bob = 2.f + 0.5f * Pose.Wave;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Pulse = 0.3f + 0.3f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): far legs first, darker
    float BodyY = 40.f + Bob;
    v2 Hip = V2(32.f, BodyY - 2.f - 0.5f * Rear);
    DrawQueenLeg(Canvas, Hip, -18.f, 14.f, -Step, Chitin, -0.4f);
    DrawQueenLeg(Canvas, Hip, -9.f, 18.f, Step, Chitin, -0.4f);
    DrawQueenLeg(Canvas, Hip, 10.f, 18.f, -Step, Chitin, -0.4f);
    DrawQueenLeg(Canvas, Hip, 18.f, 13.f, Step, Chitin, -0.4f);

    // NOTE(zoubir): the swollen abdomen behind, ridged with egg sacs
    float AbdX = 18.f;
    float AbdY = BodyY - 4.f + 0.3f * Rear;
    FillBlob(Canvas, AbdX, AbdY, 15.f + Pulse, 12.f + 0.5f * Pulse, Belly, -0.05f);
    // NOTE(zoubir): a cluster of sacs hanging from the abdomen's tail
    for(u32 Sac = 0; Sac < 5; Sac++)
    {
        float Angle = 1.6f + 0.45f * Sac;
        float X = AbdX + 11.f * Cos(Angle);
        float Y = AbdY + 8.f * Sin(Angle);
        float R = 3.f + 0.8f * Pulse + 0.5f * (float)(Sac % 2);
        FillBlob(Canvas, X, Y, R, R * 0.9f, Egg, 0.1f);
    }
    // NOTE(zoubir): the red hourglass on her back
    FillTriangle(Canvas, V2(AbdX - 3.f, AbdY - 2.f), V2(AbdX + 3.f, AbdY - 2.f),
                 V2(AbdX, AbdY + 2.f), Mark, 0.6f, 0.2f);
    FillTriangle(Canvas, V2(AbdX - 3.f, AbdY + 6.f), V2(AbdX + 3.f, AbdY + 6.f),
                 V2(AbdX, AbdY + 2.f), Mark, 0.6f, 0.2f);

    // NOTE(zoubir): near legs over the abdomen, lighter, under the head
    DrawQueenLeg(Canvas, V2(Hip.X + 1.f, Hip.Y + 2.f), -20.f, 12.f, Step, Chitin, 0.15f);
    DrawQueenLeg(Canvas, V2(Hip.X + 3.f, Hip.Y + 2.f), 16.f, 12.f, -Step, Chitin, 0.15f);

    // NOTE(zoubir): the head and thorax, raised as she rears
    float HeadX = 44.f;
    float HeadY = BodyY - 6.f - Rear;
    FillBlob(Canvas, HeadX - 6.f, HeadY + 3.f, 9.f, 7.f, Chitin, 0.05f);
    FillBlob(Canvas, HeadX + 3.f, HeadY, 7.5f, 6.5f, Chitin, 0.15f);
    // NOTE(zoubir): a crest of horns
    for(u32 Horn = 0; Horn < 3; Horn++)
    {
        float X = HeadX + 1.f + 3.f * Horn;
        FillLimb(Canvas, V2(X, HeadY - 4.f), V2(X - 2.f + Horn, HeadY - 11.f + Horn),
                 1.2f, 0.4f, Chitin, 0.1f);
    }
    // NOTE(zoubir): a cluster of red eyes
    FillDot(Canvas, HeadX + 6.f, HeadY - 1.f, 1.6f, Mark.C[3]);
    FillDot(Canvas, HeadX + 8.5f, HeadY + 0.5f, 1.2f, Mark.C[2]);
    FillDot(Canvas, HeadX + 3.5f, HeadY - 2.5f, 1.1f, Mark.C[2]);
    FillDot(Canvas, HeadX + 7.f, HeadY - 3.5f, 1.f, Mark.C[3]);
    // NOTE(zoubir): fangs, dripping when she strikes
    float Open = 1.5f * Fangs;
    FillLimb(Canvas, V2(HeadX + 8.f, HeadY + 3.f), V2(HeadX + 10.f, HeadY + 8.f + Open),
             1.2f, 0.4f, Fang);
    FillLimb(Canvas, V2(HeadX + 5.f, HeadY + 4.f), V2(HeadX + 6.f, HeadY + 9.f - Open),
             1.1f, 0.4f, Fang, -0.2f);
    if (Fangs > 0.5f)
    {
        FillDot(Canvas, HeadX + 10.f, HeadY + 10.f + Open, 1.f, Venom.C[2]);
    }

    OutlineFrame(Canvas, ART_RGB(10, 8, 12));
}

#endif
