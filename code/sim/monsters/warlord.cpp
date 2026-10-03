/* Ashen Warlord: the arena's boss. A burnt giant in black iron with a
   greatsword still glowing from the forge. Only one walks the arena at a
   time (MaxAlive 1), and it is rare.

   Calm:    Cinder Cleave smashes the ground around it and leaves embers
            burning; Ember Storm throws a wide fan of five embers.
   Enraged (below half health): moves and recharges faster, glows red,
            and starts Calling the Brood: two Cinder Imps climb out of the
            ground at its side. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Warlord)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Warlord(monster_def *Def)
{
    Def->Name = "Ashen Warlord";
    Def->MaxHp = 420.f;
    Def->Acceleration = 24000.f;
    Def->AggroRange = 440.f;
    Def->StopRange = 50.f;
    Def->AttackRange = 60.f;
    Def->AttackDamage = 14.f;
    Def->AttackInterval = 1.2f;
    Def->SpawnWeight = 1;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.5f;
    Def->EnrageSpeedScale = 1.35f;
    Def->EnrageCooldownScale = 0.6f;
    Def->EnrageTint = 0xFF7070FF;

    monster_ability *Cleave = AddMonsterAbility(Def, MonsterAbility_Slam,
                                                "Cinder Cleave");
    Cleave->MaxRange = 80.f;
    Cleave->Cooldown = 5.f;
    Cleave->Windup = 0.8f;
    Cleave->Active = 0.3f;
    Cleave->Recover = 0.8f;
    Cleave->Damage = 24.f;
    Cleave->Radius = 85.f;
    Cleave->Knockback = 650.f;
    Cleave->Status = StatusEffect_Burning;
    Cleave->StatusSeconds = 2.f;
    Cleave->HazardSeconds = 3.f;
    Cleave->HazardStyle = HazardStyle_Embers;

    monster_ability *Storm = AddMonsterAbility(Def, MonsterAbility_Volley,
                                               "Ember Storm");
    Storm->MinRange = 100.f;
    Storm->MaxRange = 380.f;
    Storm->Cooldown = 4.5f;
    Storm->Windup = 0.7f;
    Storm->Active = 1.3f;
    Storm->Recover = 0.5f;
    Storm->Damage = 10.f;
    Storm->Radius = 18.f;
    Storm->Speed = 300.f;
    Storm->Knockback = 150.f;
    Storm->Count = 5;
    Storm->Spread = 70.f;
    Storm->ShotStyle = ShotStyle_Ember;
    Storm->Status = StatusEffect_Burning;
    Storm->StatusSeconds = 1.5f;

    monster_ability *Brood = AddMonsterAbility(Def, MonsterAbility_Summon,
                                               "Call the Brood");
    Brood->MaxRange = 420.f;
    Brood->Cooldown = 9.f;
    Brood->Windup = 1.f;
    Brood->Active = 0.3f;
    Brood->Recover = 0.6f;
    Brood->SummonKind = MonsterKind_Imp;
    Brood->Count = 2;
    Brood->MaxActive = 2;
    Brood->Spread = 50.f;
    Brood->Radius = 14.f;
    Brood->PhaseMask = PHASE_ENRAGED;
}

#else

internal void
DrawMonster_Warlord(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Iron = Ramp(ART_RGB(18, 16, 20), ART_RGB(38, 34, 40),
                           ART_RGB(66, 60, 66), ART_RGB(110, 102, 106));
    color_ramp Cape = Ramp(ART_RGB(40, 10, 12), ART_RGB(78, 20, 22),
                           ART_RGB(112, 34, 30), ART_RGB(146, 56, 44));
    color_ramp Ember = Ramp(ART_RGB(170, 40, 14), ART_RGB(240, 110, 20),
                            ART_RGB(255, 190, 50), ART_RGB(255, 246, 180));
    color_ramp Blade = Ramp(ART_RGB(60, 56, 60), ART_RGB(110, 104, 106),
                            ART_RGB(170, 162, 160), ART_RGB(230, 224, 216));

    float Bob = 0.f;
    float Stride = 0.f;
    float Lean = 0.f;
    // NOTE(zoubir): the sword is a line from the grip toward its tip angle;
    // 0 points straight up, positive tips forward
    v2 Grip = V2(40.f, 34.f);
    float SwordAngle = 0.9f;
    float Heat = 0.4f + 0.2f * Pose.Wave;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Stride = 4.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            SwordAngle = 1.0f + 0.1f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): the sword swings up overhead, blade heating
            Grip = Lerp2(Grip, Pose.t, V2(34.f, 24.f));
            SwordAngle = Lerp(0.9f, Pose.t, -0.5f);
            Lean = -3.f * Pose.t;
            Heat = 0.4f + 0.6f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Grip = Lerp2(V2(36.f, 24.f), Pose.t, V2(44.f, 40.f));
            SwordAngle = Lerp(-0.4f, Pose.t, 2.1f);
            Lean = 4.f;
            Heat = 1.f;
        } break;

        case AnimationType_Stop:
        {
            // NOTE(zoubir): sword planted, leaning on it
            Grip = V2(44.f, 36.f);
            SwordAngle = 3.0f;
            Lean = 3.f;
            Bob = 2.f + 0.6f * Pose.Wave;
        } break;

        default:
        {
            Bob = 0.8f * Pose.Wave;
        } break;
    }

    float Waist = 38.f + Bob;
    // NOTE(zoubir): tattered cape behind everything, lagging the body
    FillTriangle(Canvas, V2(24.f + Lean, 20.f + Bob), V2(10.f - Stride * 0.5f, 54.f),
                 V2(30.f + Lean, 52.f), Cape, 0.7f, 0.2f);
    for(u32 Tatter = 0; Tatter < 3; Tatter++)
    {
        float X = 12.f + 6.f * Tatter - Stride * 0.4f;
        FillTriangle(Canvas, V2(X, 51.f), V2(X + 2.f, 57.f - (float)(Tatter % 2) * 2.f),
                     V2(X + 5.f, 51.f), Cape, 0.4f, 0.1f);
    }

    // NOTE(zoubir): back leg, greaves of black iron
    FillLimb(Canvas, V2(27.f, Waist), V2(25.f - Stride, 52.f), 4.5f, 3.5f, Iron, -0.2f);
    FillBlob(Canvas, 24.f - Stride, 54.f, 5.f, 2.5f, Iron, -0.2f);
    // NOTE(zoubir): torso: breastplate with ember cracks glowing through
    v2 Chest = V2(32.f + Lean * 0.5f, 28.f + Bob);
    FillBlob(Canvas, Chest.X, Chest.Y + 9.f, 10.f, 6.f, Cape);
    FillBlob(Canvas, Chest.X, Chest.Y, 12.f, 11.f, Iron);
    FillLimb(Canvas, Chest + V2(-6.f, -4.f), Chest + V2(-1.f, 3.f), 0.6f, 0.5f,
             Ember, Heat - 0.5f);
    FillLimb(Canvas, Chest + V2(-1.f, 3.f), Chest + V2(4.f, 1.f), 0.6f, 0.5f,
             Ember, Heat - 0.5f);
    FillLimb(Canvas, Chest + V2(3.f, -6.f), Chest + V2(6.f, 5.f), 0.5f, 0.5f,
             Ember, Heat - 0.6f);
    // NOTE(zoubir): front leg
    FillLimb(Canvas, V2(36.f, Waist), V2(38.f + Stride, 52.f), 4.5f, 3.5f, Iron);
    FillBlob(Canvas, 39.f + Stride, 54.f, 5.5f, 2.5f, Iron);

    // NOTE(zoubir): horned helm with a burning visor slit
    v2 Helm = V2(37.f + Lean, 13.f + Bob);
    FillLimb(Canvas, Helm + V2(-4.f, -4.f), Helm + V2(-10.f, -11.f), 1.8f, 0.6f, Iron, 0.1f);
    FillLimb(Canvas, Helm + V2(2.f, -5.f), Helm + V2(5.f, -12.f), 1.8f, 0.6f, Iron, 0.15f);
    FillBlob(Canvas, Helm.X, Helm.Y, 6.5f, 6.5f, Iron, 0.1f);
    FillLimb(Canvas, Helm + V2(0.f, 0.f), Helm + V2(6.f, -0.5f), 1.f, 0.8f, Ember,
             Heat - 0.2f);
    // NOTE(zoubir): spiked pauldron
    FillBlob(Canvas, Chest.X - 6.f, Chest.Y - 8.f, 7.f, 4.5f, Iron, 0.1f);
    FillTriangle(Canvas, Chest + V2(-12.f, -9.f), Chest + V2(-14.f, -16.f),
                 Chest + V2(-7.f, -11.f), Iron, 0.9f, 0.4f);

    // NOTE(zoubir): greatsword: crossguard, then a blade with a hot edge
    v2 Along = V2(Sin(SwordAngle), -Cos(SwordAngle));
    v2 Tip = Grip + 21.f * Along;
    v2 Across = V2(-Along.Y, Along.X);
    FillLimb(Canvas, Grip - 4.f * Across, Grip + 4.f * Across, 1.2f, 1.2f, Iron, 0.1f);
    FillLimb(Canvas, Grip - 3.f * Along, Grip, 1.2f, 1.2f, Cape);
    FillLimb(Canvas, Grip + 1.f * Along, Tip, 2.6f, 1.2f, Blade, 0.05f);
    FillLimb(Canvas, Grip + 6.f * Along + 1.5f * Across, Tip + 0.8f * Across,
             0.6f, 0.4f, Ember, Heat - 0.3f);

    // NOTE(zoubir): gauntlet over the grip
    FillLimb(Canvas, Chest + V2(6.f, -4.f), Grip, 3.f, 2.6f, Iron, 0.05f);
    FillBlob(Canvas, Grip.X, Grip.Y, 3.f, 3.f, Iron, 0.15f);

    if (Pose.Anim == AnimationType_Attack && Pose.t > 0.5f)
    {
        // NOTE(zoubir): fire bursting from where the sword lands
        float Spread = 6.f + 14.f * (Pose.t - 0.5f);
        for(u32 Flame = 0; Flame < 5; Flame++)
        {
            float X = Tip.X - Spread + 0.5f * Spread * Flame;
            FillLimb(Canvas, V2(X, 58.f), V2(X + 1.f, 58.f - 4.f - 3.f * (float)(Flame % 2)),
                     1.6f, 0.5f, Ember, 0.2f);
        }
    }

    OutlineFrame(Canvas, ART_RGB(10, 6, 6));
}

#endif
