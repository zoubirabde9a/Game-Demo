/* Vol'karr the Ember Tyrant: the Ember Depths' last boss (sim/dungeon/,
   docs/dungeon-depths.md), on the Throne of Embers. A horned demon in
   charred plate, ragged wings of black membrane, a mane of flame and a
   cleaver of red-hot iron. Never roams (SpawnWeight 0).

   Calm:    Hellfire Cleave splits the floor round him and leaves it
            burning; Flame Step vanishes in a burst and comes down behind
            his target; Cinderfall drops three burning stones over the
            hall, where the party is heading; Tyrant's Judgement steps
            beside whoever holds him and cleaves them, past any dodge.
   Enraged (below 40% health): faster, white with heat, and Crown of
            Fire: four fireballs out in a cross round his target.
   The dungeon raises Cinder Imps at 75%, 50% and 25% and binds a Magma
   Champion at 60% and 30% that erupts if not killed in time
   (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(EmberTyrant)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_EmberTyrant(monster_def *Def)
{
    Def->Name = "Vol'karr the Ember Tyrant";
    Def->MaxHp = 1850.f;
    Def->Acceleration = 28000.f;
    Def->AggroRange = 720.f;
    Def->StopRange = 52.f;
    Def->AttackRange = 68.f;
    Def->AttackDamage = 16.5f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.4f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.65f;
    Def->EnrageTint = 0xFFC0F0FF;

    monster_ability *Crown = AddMonsterAbility(Def, MonsterAbility_Volley,
                                               "Crown of Fire");
    Crown->MaxRange = 520.f;
    Crown->Cooldown = 5.f;
    Crown->Windup = 0.7f;
    Crown->Active = 1.6f;
    Crown->Recover = 0.5f;
    Crown->Damage = 11.f;
    Crown->Radius = 20.f;
    Crown->Speed = 270.f;
    Crown->Knockback = 150.f;
    Crown->Count = MAX_ABILITY_POINTS;
    Crown->Spread = 270.f;
    Crown->ShotStyle = ShotStyle_Ember;
    Crown->Status = StatusEffect_Burning;
    Crown->StatusSeconds = 2.f;
    Crown->PhaseMask = PHASE_ENRAGED;

    monster_ability *Cleave = AddMonsterAbility(Def, MonsterAbility_Slam,
                                                "Hellfire Cleave");
    Cleave->MaxRange = 95.f;
    Cleave->Cooldown = 4.f;
    Cleave->Windup = 0.9f;
    Cleave->Active = 0.3f;
    Cleave->Recover = 0.6f;
    Cleave->Damage = 22.f;
    Cleave->Radius = 125.f;
    Cleave->Knockback = 700.f;
    Cleave->Status = StatusEffect_Burning;
    Cleave->StatusSeconds = 2.f;
    Cleave->HazardSeconds = 3.f;
    Cleave->HazardStyle = HazardStyle_Embers;

    // NOTE(zoubir): he does not stay on the tank: the step puts him on
    // the back line, and the tank has to taunt him back
    monster_ability *Step = AddMonsterAbility(Def, MonsterAbility_Blink,
                                              "Flame Step");
    Step->MinRange = 160.f;
    Step->MaxRange = 540.f;
    Step->Cooldown = 8.f;
    Step->Windup = 0.8f;
    Step->Active = 0.3f;
    Step->Recover = 0.6f;
    Step->Damage = 11.f;
    Step->Radius = 80.f;
    Step->Spread = 60.f;
    Step->Knockback = 500.f;
    Step->Status = StatusEffect_Burning;
    Step->StatusSeconds = 1.5f;

    monster_ability *Fall = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                              "Cinderfall");
    Fall->MinRange = 100.f;
    Fall->MaxRange = 560.f;
    Fall->Cooldown = 7.f;
    Fall->Windup = 0.95f;
    Fall->Active = 0.3f;
    Fall->Recover = 0.5f;
    Fall->Damage = 9.f;
    Fall->Radius = 60.f;
    Fall->Knockback = 250.f;
    Fall->Count = 3;
    Fall->Spread = 150.f;
    Fall->Status = StatusEffect_Burning;
    Fall->StatusSeconds = 2.f;

    // NOTE(zoubir): he steps out of the fire beside whoever holds him and
    // brings the cleaver down; it cannot be dodged, only taken by the
    // tank or taunted off a friend before it lands
    monster_ability *Judgement = AddMonsterAbility(Def, MonsterAbility_Smite,
                                                   "Tyrant's Judgement");
    Judgement->MaxRange = 560.f;
    Judgement->Cooldown = 12.f;
    Judgement->Windup = 1.1f;
    Judgement->Active = 0.3f;
    Judgement->Recover = 0.6f;
    Judgement->Damage = 30.f;
    Judgement->Radius = 24.f;
    Judgement->Spread = 60.f;
    Judgement->Knockback = 450.f;
    Judgement->Status = StatusEffect_Burning;
    Judgement->StatusSeconds = 1.5f;
}

#else

internal void
DrawMonster_EmberTyrant(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Hide = Ramp(ART_RGB(60, 14, 12), ART_RGB(104, 28, 20),
                           ART_RGB(150, 50, 30), ART_RGB(196, 90, 56));
    color_ramp Plate = Ramp(ART_RGB(20, 18, 20), ART_RGB(44, 40, 42),
                            ART_RGB(74, 68, 68), ART_RGB(112, 104, 100));
    color_ramp Flame = Ramp(ART_RGB(170, 40, 10), ART_RGB(240, 110, 20),
                            ART_RGB(255, 184, 50), ART_RGB(255, 244, 170));
    color_ramp Horn = Ramp(ART_RGB(60, 50, 40), ART_RGB(110, 96, 76),
                           ART_RGB(160, 146, 118), ART_RGB(220, 208, 180));
    color_ramp Membrane = Ramp(ART_RGB(24, 10, 12), ART_RGB(48, 20, 22),
                               ART_RGB(78, 34, 34), ART_RGB(110, 54, 48));
    color_ramp Blade = Ramp(ART_RGB(120, 30, 10), ART_RGB(200, 70, 20),
                            ART_RGB(250, 140, 50), ART_RGB(255, 224, 150));

    float Bob = 0.f;
    float Step = 0.f;
    float WingLift = 0.2f;
    // NOTE(zoubir): the cleaver's angle from straight up, forward positive
    float Swing = 2.3f;
    float Blaze = 0.5f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
            WingLift = 0.2f + 0.2f * Pose.Wave2;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): wings spread, cleaver raised, the mane flaring
            Swing = 2.3f - 3.3f * Pose.t;
            WingLift = 0.2f + 0.8f * Pose.t;
            Blaze = 0.5f + 0.5f * Pose.t;
            Bob = -1.5f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Swing = -1.f + 3.4f * Minimum(1.f, 2.f * Pose.t);
            WingLift = 1.f - Pose.t;
            Blaze = 1.f;
            Bob = 2.f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            Swing = 2.3f;
            WingLift = -0.4f;
            Bob = 2.f + 0.5f * Pose.Wave;
            Blaze = 0.3f;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            WingLift = 0.15f + 0.1f * Pose.Wave;
            Blaze = 0.45f + 0.2f * Pose.Wave2;
        } break;
    }

    float Ground = 56.f;
    float HipY = 40.f + Bob;
    float BodyY = HipY - 11.f;

    // NOTE(zoubir): wings behind everything, the far one first
    DrawMembraneWing(Canvas, V2(28.f, BodyY - 6.f), -1.f, WingLift, 19.f, Membrane, Horn);
    DrawMembraneWing(Canvas, V2(32.f, BodyY - 7.f), 1.f, WingLift * 0.8f, 15.f, Membrane, Horn);

    // NOTE(zoubir): digitigrade legs ending in hooves
    FillLimb(Canvas, V2(28.f, HipY), V2(25.f - Step, HipY + 8.f), 4.f, 3.f, Hide, -0.4f);
    FillLimb(Canvas, V2(25.f - Step, HipY + 8.f), V2(27.f - Step, Ground - 1.f), 3.f, 2.f, Hide, -0.4f);
    FillBlob(Canvas, 27.f - Step, Ground - 0.5f, 3.f, 1.6f, Plate, -0.3f);
    FillLimb(Canvas, V2(35.f, HipY), V2(33.f + Step, HipY + 8.f), 4.f, 3.f, Hide, 0.1f);
    FillLimb(Canvas, V2(33.f + Step, HipY + 8.f), V2(36.f + Step, Ground - 1.f), 3.f, 2.f, Hide, 0.1f);
    FillBlob(Canvas, 36.f + Step, Ground - 0.5f, 3.f, 1.6f, Plate, 0.1f);

    // NOTE(zoubir): a broad chest under charred plate, a burning heart
    FillBlob(Canvas, 32.f, BodyY, 11.f, 12.f, Hide, 0.05f);
    FillBlob(Canvas, 33.f, BodyY - 3.f, 10.f, 6.f, Plate, 0.15f);
    FillDot(Canvas, 34.f, BodyY + 2.f, 1.5f + 1.5f * Blaze, Flame.C[2 + (Blaze > 0.8f ? 1 : 0)]);
    FillBlob(Canvas, 24.f, BodyY - 7.f, 5.f, 4.f, Plate, -0.1f);

    // NOTE(zoubir): the head: a snout, two great horns sweeping back, a
    // mane of flame
    float HeadX = 38.f;
    float HeadY = BodyY - 14.f;
    for(u32 Tongue = 0; Tongue < 4; Tongue++)
    {
        float X = HeadX - 7.f + 2.5f * Tongue;
        float Rise = 7.f + 4.f * Blaze + 2.f * Sin(6.2832f * Pose.t + Tongue);
        FillLimb(Canvas, V2(X, HeadY), V2(X - 3.f, HeadY - Rise), 2.6f, 0.4f, Flame, 0.3f);
    }
    FillBlob(Canvas, HeadX, HeadY, 6.f, 5.f, Hide, 0.2f);
    FillBlob(Canvas, HeadX + 5.f, HeadY + 1.5f, 3.5f, 2.5f, Hide, 0.25f);
    FillLimb(Canvas, V2(HeadX - 2.f, HeadY - 3.f), V2(HeadX - 9.f, HeadY - 9.f), 1.8f, 1.f, Horn, 0.2f);
    FillLimb(Canvas, V2(HeadX - 9.f, HeadY - 9.f), V2(HeadX - 7.f, HeadY - 15.f), 1.f, 0.3f, Horn, 0.2f);
    FillLimb(Canvas, V2(HeadX + 2.f, HeadY - 4.f), V2(HeadX - 2.f, HeadY - 11.f), 1.6f, 0.3f, Horn, 0.3f);
    FillDot(Canvas, HeadX + 3.f, HeadY - 1.f, 1.3f, Flame.C[3]);
    FillDot(Canvas, HeadX + 0.5f, HeadY - 1.f, 1.f, Flame.C[2]);

    // NOTE(zoubir): the near arm and the cleaver
    v2 Shoulder = V2(39.f, BodyY - 6.f);
    v2 Up = V2(Sin(Swing), -Cos(Swing));
    v2 Hand = Shoulder + 10.f * Up;
    FillLimb(Canvas, Shoulder, Hand, 3.5f, 3.f, Hide, 0.2f);
    v2 Across = V2(-Up.Y, Up.X);
    v2 Tip = Hand + 20.f * Up;
    FillTriangle(Canvas, Hand + 1.5f * Across, Tip + 4.f * Across, Tip - 1.f * Across,
                 Blade, 0.2f, 0.5f);
    FillLimb(Canvas, Hand - 1.f * Across, Hand + 3.f * Across, 1.4f, 1.4f, Plate, 0.f);
    if (Blaze > 0.8f)
    {
        FillDot(Canvas, Tip.X, Tip.Y, 2.f, Flame.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(10, 4, 4));
}

#endif
