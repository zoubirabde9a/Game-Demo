/* Hrimgar the Frost Colossus: the Rimeheart Vault's first boss
   (sim/dungeon/, docs/dungeon-vault.md), in the Calving Hall. A giant
   of blue glacier ice packed round black rock, a cold light burning in
   his chest, fists like boulders and a crown of icicles. Never roams
   (SpawnWeight 0).

   Calm:    Shatter Ring sends a ring of ice out from him that spares only
            those at his feet, so the back line has to come in close;
            Glacial Stomp hits everyone near him and slows them; Avalanche
            drops chunks of ice where the far players are heading;
            Glacier Rush charges along a locked line at someone far off.
            Frostbite Grip: a crushing grip on whoever holds him, which
            nobody can dodge.
   Enraged (below 40% health): faster, white with frost.
   The dungeon adds Rimebound Brutes at 75%, 50% and 25% that walk back
   into him if they live too long (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(FrostColossus)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_FrostColossus(monster_def *Def)
{
    Def->Name = "Hrimgar the Frost Colossus";
    Def->MaxHp = 1800.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 640.f;
    Def->StopRange = 54.f;
    Def->AttackRange = 70.f;
    Def->AttackDamage = 18.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.4f;
    Def->EnrageSpeedScale = 1.25f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFFFFE0A0;

    // NOTE(zoubir): a ring: safe only inside InnerRadius, at his feet, so
    // the whole party has to stack on the tank while it goes out. Long
    // windup, since a healer at the back has a long way to walk
    monster_ability *Ring = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Shatter Ring");
    Ring->MaxRange = 95.f;
    Ring->Cooldown = 10.f;
    Ring->Windup = 1.3f;
    Ring->Active = 0.3f;
    Ring->Recover = 0.7f;
    Ring->Damage = 20.f;
    Ring->Radius = 260.f;
    Ring->InnerRadius = 80.f;
    Ring->Knockback = 400.f;
    Ring->Status = StatusEffect_Slowed;
    Ring->StatusSeconds = 2.5f;

    monster_ability *Stomp = AddMonsterAbility(Def, MonsterAbility_Slam,
                                               "Glacial Stomp");
    Stomp->MaxRange = 95.f;
    Stomp->Cooldown = 4.5f;
    Stomp->Windup = 0.8f;
    Stomp->Active = 0.3f;
    Stomp->Recover = 0.6f;
    Stomp->Damage = 21.f;
    Stomp->Radius = 110.f;
    Stomp->Knockback = 650.f;
    Stomp->Status = StatusEffect_Slowed;
    Stomp->StatusSeconds = 2.f;

    monster_ability *Avalanche = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                                   "Avalanche");
    Avalanche->MinRange = 120.f;
    Avalanche->MaxRange = 500.f;
    Avalanche->Cooldown = 5.5f;
    Avalanche->Windup = 0.9f;
    Avalanche->Active = 0.3f;
    Avalanche->Recover = 0.5f;
    Avalanche->Damage = 13.f;
    Avalanche->Radius = 55.f;
    Avalanche->Knockback = 250.f;
    Avalanche->Count = MAX_ABILITY_POINTS;
    Avalanche->Spread = 130.f;
    Avalanche->Status = StatusEffect_Slowed;
    Avalanche->StatusSeconds = 2.f;

    monster_ability *Rush = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Glacier Rush");
    Rush->MinRange = 160.f;
    Rush->MaxRange = 500.f;
    Rush->Cooldown = 6.f;
    Rush->Windup = 0.7f;
    Rush->Active = 0.7f;
    Rush->Recover = 0.8f;
    Rush->Damage = 22.f;
    Rush->Radius = 48.f;
    Rush->Speed = 600.f;
    Rush->Knockback = 750.f;

    // NOTE(zoubir): the vault's blows nobody dodges hit harder than the
    // depths' (26), for a party some levels further on
    monster_ability *Grip = AddMonsterAbility(Def, MonsterAbility_Smite,
                                              "Frostbite Grip");
    Grip->MaxRange = 480.f;
    Grip->Cooldown = 11.f;
    Grip->Windup = 1.f;
    Grip->Active = 0.3f;
    Grip->Recover = 0.5f;
    Grip->Damage = 28.f;
    Grip->Radius = 24.f;
    Grip->Knockback = 200.f;
    Grip->Status = StatusEffect_Slowed;
    Grip->StatusSeconds = 2.f;
}

#else

internal void
DrawMonster_FrostColossus(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Ice = Ramp(ART_RGB(30, 60, 96), ART_RGB(70, 120, 170),
                          ART_RGB(130, 190, 225), ART_RGB(215, 245, 255));
    color_ramp Rock = Ramp(ART_RGB(30, 34, 44), ART_RGB(58, 64, 78),
                           ART_RGB(92, 100, 116), ART_RGB(136, 144, 158));
    color_ramp Core = Ramp(ART_RGB(20, 90, 160), ART_RGB(60, 170, 230),
                           ART_RGB(150, 230, 255), ART_RGB(240, 255, 255));

    float Bob = 0.f;
    float Step = 0.f;
    // NOTE(zoubir): the fists, 0 hanging at his sides, 1 overhead
    float Raise = 0.f;
    float Glow = 0.5f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
            Raise = 0.1f + 0.05f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): both fists rise over his head, the core
            // brightening
            Raise = Pose.t;
            Glow = 0.5f + 0.5f * Pose.t;
            Bob = -1.5f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Raise = 1.f - 1.3f * Minimum(1.f, 2.f * Pose.t);
            Bob = 2.5f * Pose.t;
            Glow = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Raise = -0.25f;
            Bob = 2.f + 0.5f * Pose.Wave;
            Glow = 0.3f;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Glow = 0.45f + 0.15f * Pose.Wave;
        } break;
    }

    float Ground = 57.f;
    float HipY = 41.f + Bob;
    float BodyY = HipY - 12.f;

    // NOTE(zoubir): legs like pillars of rock, the far one darker
    FillLimb(Canvas, V2(27.f, HipY), V2(24.f - Step, Ground - 3.f), 6.f, 5.f, Rock, -0.4f);
    FillBlob(Canvas, 24.f - Step, Ground - 1.5f, 6.5f, 3.f, Ice, -0.3f);
    FillLimb(Canvas, V2(37.f, HipY), V2(39.f + Step, Ground - 3.f), 6.f, 5.f, Rock, 0.1f);
    FillBlob(Canvas, 39.f + Step, Ground - 1.5f, 6.5f, 3.f, Ice, 0.1f);

    // NOTE(zoubir): the arms swing round the shoulders by Raise: down at
    // 0, out in front at 0.5, overhead at 1
    float Angle = Raise * Pi32;
    v2 Reach = V2(0.1f + 0.4f * Sin(Angle), Cos(Angle));
    v2 FarShoulder = V2(23.f, BodyY - 5.f);
    v2 FarHand = FarShoulder + 15.f * Reach;
    FillLimb(Canvas, FarShoulder, FarHand, 5.f, 4.5f, Ice, -0.4f);
    FillBlob(Canvas, FarHand.X, FarHand.Y, 6.f, 5.5f, Rock, -0.4f);

    // NOTE(zoubir): shards of ice jutting from the back and shoulders
    FillTriangle(Canvas, V2(19.f, BodyY - 4.f), V2(24.f, BodyY - 9.f), V2(15.f, BodyY - 19.f), Ice, 0.1f, 0.9f);
    FillTriangle(Canvas, V2(25.f, BodyY - 9.f), V2(30.f, BodyY - 11.f), V2(25.f, BodyY - 22.f), Ice, 0.2f, 1.f);
    FillTriangle(Canvas, V2(18.f, BodyY + 3.f), V2(21.f, BodyY - 2.f), V2(11.f, BodyY - 6.f), Ice, 0.f, 0.7f);

    // NOTE(zoubir): a glacier for a chest, rock plates, the cold core
    FillBlob(Canvas, 32.f, BodyY, 15.f, 13.f, Ice, 0.05f);
    FillBlob(Canvas, 26.f, BodyY + 5.f, 7.f, 6.f, Rock, -0.1f);
    FillBlob(Canvas, 30.f, BodyY - 8.f, 6.f, 3.5f, Rock, 0.1f);
    FillBlob(Canvas, 36.f, BodyY + 1.f, 4.f + 1.5f * Glow, 4.5f + 1.5f * Glow, Core, 0.4f);
    FillDot(Canvas, 36.f, BodyY + 1.f, 1.5f + 2.f * Glow, Core.C[3]);

    // NOTE(zoubir): a small head of rock sunk between the shoulders, with
    // a crown of icicles
    float HeadX = 39.f;
    float HeadY = BodyY - 14.f;
    FillBlob(Canvas, HeadX, HeadY, 5.5f, 5.f, Rock, 0.2f);
    FillDot(Canvas, HeadX + 3.f, HeadY - 0.5f, 1.3f, Core.C[2 + (Glow > 0.7f ? 1 : 0)]);
    FillDot(Canvas, HeadX - 0.5f, HeadY - 0.5f, 1.1f, Core.C[2]);
    for(u32 Spike = 0; Spike < 3; Spike++)
    {
        float X = HeadX - 4.f + 3.5f * Spike;
        FillTriangle(Canvas, V2(X - 1.5f, HeadY - 4.f), V2(X + 1.5f, HeadY - 4.f),
                     V2(X + 0.5f, HeadY - 10.f + (Spike == 1 ? -2.f : 0.f)), Ice, 0.3f, 1.f);
    }

    // NOTE(zoubir): the near arm and its fist, rimed knuckles
    v2 NearShoulder = V2(42.f, BodyY - 5.f);
    v2 NearHand = NearShoulder + 15.f * Reach + V2(2.f, 0.f);
    FillLimb(Canvas, NearShoulder, NearHand, 5.5f, 5.f, Ice, 0.2f);
    FillBlob(Canvas, NearHand.X, NearHand.Y, 7.f, 6.f, Rock, 0.2f);
    FillDot(Canvas, NearHand.X + 2.f, NearHand.Y - 2.f, 1.5f, Ice.C[3]);
    FillDot(Canvas, NearHand.X + 3.f, NearHand.Y + 1.f, 1.2f, Ice.C[3]);
    if (Glow > 0.8f)
    {
        FillDot(Canvas, NearHand.X, NearHand.Y, 2.f, Core.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(8, 12, 22));
}

#endif
