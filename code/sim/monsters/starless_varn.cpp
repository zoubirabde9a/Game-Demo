/* Varn the Mirror Lord: the Starless Deep's second boss (sim/dungeon/,
   docs/dungeon-starless.md), in the Obsidian Court. A knight twice a
   man's height in black glass, a great mirror for a shield, the last
   light of the star caught in it. Never roams (SpawnWeight 0).

   Mirror Aegis: he plants the mirror for 3 s (MonsterAbility_Reflect);
            every blow on him is turned back whole on whoever struck, up
            to 40. The party stops, and the striker's casts must not land
            in that window.
   Brand of Judgement: a void brand on whoever stands farthest from him.
   Obsidian Cleave: a slam round him. Shield Charge: a charge at someone
            far off. Verdict: a blow nobody dodges on whoever holds him.
   Enraged (below 30% health): faster, and the mirror comes sooner.
   The dungeon brings Obsidian Knights at 70% and 35%, whose own mirrors
   come up out of step with his, a Sunguard at 60% that erupts unless it
   dies in 20 s, and Void Seers at 50%
   (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Varn)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Varn(monster_def *Def)
{
    Def->Name = "Varn the Mirror Lord";
    Def->MaxHp = 2200.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 720.f;
    Def->StopRange = 54.f;
    Def->AttackRange = 70.f;
    Def->AttackDamage = 17.f;
    Def->AttackInterval = 0.95f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 72;

    Def->EnrageHpShare = 0.3f;
    Def->EnrageSpeedScale = 1.25f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFFF0D0E0;

    monster_ability *Aegis = AddMonsterAbility(Def, MonsterAbility_Reflect,
                                               "Mirror Aegis");
    Aegis->MaxRange = 650.f;
    Aegis->Cooldown = 10.f;
    Aegis->Windup = 1.f;
    Aegis->Active = 3.f;
    Aegis->Recover = 0.6f;
    Aegis->Damage = 40.f;
    Aegis->Radius = 46.f;
    Aegis->Spread = 1.f;

    monster_ability *Judgement = AddMonsterAbility(Def, MonsterAbility_Brand,
                                                   "Brand of Judgement");
    Judgement->MaxRange = 640.f;
    Judgement->Cooldown = 10.f;
    Judgement->Windup = 3.f;
    Judgement->Active = 0.3f;
    Judgement->Recover = 0.5f;
    Judgement->Damage = 34.f;
    Judgement->Radius = 160.f;
    Judgement->Knockback = 300.f;

    monster_ability *Cleave = AddMonsterAbility(Def, MonsterAbility_Slam,
                                                "Obsidian Cleave");
    Cleave->MaxRange = 95.f;
    Cleave->Cooldown = 4.f;
    Cleave->Windup = 0.75f;
    Cleave->Active = 0.3f;
    Cleave->Recover = 0.6f;
    Cleave->Damage = 32.f;
    Cleave->Radius = 115.f;
    Cleave->Knockback = 560.f;

    monster_ability *Charge = AddMonsterAbility(Def, MonsterAbility_Charge,
                                                "Shield Charge");
    Charge->MinRange = 220.f;
    Charge->MaxRange = 600.f;
    Charge->Cooldown = 8.f;
    Charge->Windup = 0.8f;
    Charge->Active = 0.7f;
    Charge->Recover = 0.9f;
    Charge->Damage = 24.f;
    Charge->Radius = 44.f;
    Charge->Speed = 640.f;
    Charge->Knockback = 600.f;

    monster_ability *Verdict = AddMonsterAbility(Def, MonsterAbility_Smite,
                                                 "Verdict");
    Verdict->MaxRange = 560.f;
    Verdict->Cooldown = 11.f;
    Verdict->Windup = 1.f;
    Verdict->Active = 0.3f;
    Verdict->Recover = 0.6f;
    Verdict->Damage = 36.f;
    Verdict->Radius = 24.f;
    Verdict->Spread = 60.f;
    Verdict->Knockback = 320.f;
}

#else

internal void
DrawMonster_Varn(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Glass = Ramp(ART_RGB(10, 8, 18), ART_RGB(28, 24, 44),
                            ART_RGB(60, 54, 90), ART_RGB(140, 130, 180));
    color_ramp Mirror = Ramp(ART_RGB(90, 96, 124), ART_RGB(160, 170, 200),
                             ART_RGB(216, 222, 244), ART_RGB(255, 255, 255));
    color_ramp Gold = Ramp(ART_RGB(100, 60, 10), ART_RGB(170, 120, 30),
                           ART_RGB(230, 190, 70), ART_RGB(255, 240, 170));
    color_ramp Cape = Ramp(ART_RGB(30, 6, 30), ART_RGB(60, 16, 60),
                           ART_RGB(96, 30, 96), ART_RGB(140, 60, 140));

    float Step = 0.f;
    float Bob = 0.f;
    float Guard = 0.f;
    float Swing = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 4.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Guard = Pose.t;
            Swing = -Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Guard = 1.f;
            Swing = -1.f + 2.f * Minimum(1.f, 3.f * Pose.t);
            Bob = 0.5f * Pose.Wave;
        } break;

        case AnimationType_Stop:
        {
            Guard = 1.f - Pose.t;
            Swing = 1.f - Pose.t;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
        } break;
    }

    float Ground = 63.f;
    float HipY = 44.f + Bob;
    // NOTE(zoubir): a cape of dark violet behind, stirring
    FillTriangle(Canvas, V2(28.f, HipY - 18.f), V2(38.f, HipY - 18.f),
                 V2(18.f + Pose.Wave, Ground - 2.f), Cape, -0.2f, -0.5f);
    FillTriangle(Canvas, V2(30.f, HipY - 18.f), V2(38.f, HipY - 18.f),
                 V2(30.f + Pose.Wave2, Ground), Cape, -0.1f, -0.4f);
    FillLimb(Canvas, V2(31.f, HipY), V2(29.f - Step, Ground - 3.f), 4.5f, 4.f, Glass, -0.3f);
    FillLimb(Canvas, V2(40.f, HipY), V2(42.f + Step, Ground - 3.f), 4.5f, 4.f, Glass, 0.f);
    FillBlob(Canvas, 29.f - Step, Ground - 1.f, 5.f, 2.5f, Glass, -0.2f);
    FillBlob(Canvas, 43.f + Step, Ground - 1.f, 5.f, 2.5f, Glass, 0.f);

    // NOTE(zoubir): the sword arm behind, a long blade of black glass
    v2 Shoulder = V2(30.f, HipY - 17.f);
    v2 Hand = Shoulder + V2(-6.f + 8.f * Swing, 8.f - 6.f * Swing);
    FillLimb(Canvas, Shoulder, Hand, 3.5f, 3.f, Glass, -0.2f);
    v2 Tip = Hand + V2(-8.f + 26.f * Swing, -20.f + 6.f * Swing);
    FillLimb(Canvas, Hand, Tip, 1.8f, 0.8f, Mirror, 0.3f);

    FillBlob(Canvas, 36.f, HipY - 12.f, 11.f, 14.f, Glass, 0.f);
    FillLimb(Canvas, V2(27.f, HipY - 20.f), V2(45.f, HipY - 20.f), 1.3f, 1.3f, Gold, 0.3f);
    FillLimb(Canvas, V2(36.f, HipY - 24.f), V2(36.f, HipY), 1.f, 1.f, Gold, 0.2f);
    FillBlob(Canvas, 28.f, HipY - 21.f, 5.f, 4.f, Glass, 0.2f);
    FillBlob(Canvas, 44.f, HipY - 21.f, 5.f, 4.f, Glass, 0.3f);

    // NOTE(zoubir): a crowned helm, its visor a line of violet light
    float HeadX = 37.f;
    float HeadY = HipY - 31.f;
    FillBlob(Canvas, HeadX, HeadY, 6.5f, 7.f, Glass, 0.2f);
    FillLimb(Canvas, V2(HeadX + 1.f, HeadY), V2(HeadX + 6.f, HeadY), 1.f, 1.f, Gold, 0.4f);
    FillDot(Canvas, HeadX + 4.f, HeadY, 0.9f, ART_RGB(230, 180, 255));
    for(u32 Point = 0; Point < 3; Point++)
    {
        float X = HeadX - 4.f + 4.f * Point;
        FillTriangle(Canvas, V2(X - 1.5f, HeadY - 5.f), V2(X + 1.5f, HeadY - 5.f),
                     V2(X, HeadY - 11.f + (Point == 1 ? -2.f : 0.f)), Gold, 0.4f, 0.8f);
    }

    // NOTE(zoubir): the great mirror, swung round in front when he guards
    float ShieldX = 46.f + 9.f * Guard;
    float ShieldY = HipY - 12.f - 2.f * Guard;
    FillBlob(Canvas, ShieldX, ShieldY, 6.f + 2.f * Guard, 15.f, Gold, 0.f);
    FillBlob(Canvas, ShieldX + 0.5f, ShieldY, 4.5f + 2.f * Guard, 13.f, Mirror, 0.4f + 0.3f * Guard);
    FillLimb(Canvas, V2(ShieldX - 1.f, ShieldY - 9.f), V2(ShieldX + 2.f, ShieldY - 4.f),
             0.8f, 0.6f, Mirror, 1.f);
    if (Guard > 0.5f)
    {
        FillDot(Canvas, ShieldX + 1.f, ShieldY - 6.f, 1.5f, ART_RGB(255, 255, 255));
        FillDot(Canvas, ShieldX - 1.f, ShieldY + 5.f, 1.f, ART_RGB(230, 200, 255));
    }

    OutlineFrame(Canvas, ART_RGB(4, 2, 10));
}

#endif
