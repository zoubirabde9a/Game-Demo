/* Grondmaw the Avalanche: the Aurora Rift's first boss (sim/dungeon/,
   docs/dungeon-rift.md), in the Avalanche Den. An old yeti as tall as a
   house, his fur matted into plates of ice, a slab of glacier grown into
   his back. Never roams (SpawnWeight 0).

   Calm:    Avalanche Roar: three rings of frost roll out across the whole
            den one behind the other, a jump rope the party has to jump
            three times. Glacier Fist hammers the ground round him,
            Boulder Hurl lobs three chunks of ice at the back line, and
            Avalanche Rush charges along a locked line at someone far off.
            Crushing Grip: he takes whoever holds him in one hand, past any
            dodge.
   Enraged (below 35% health): faster, and the roars come sooner.
   The dungeon sends Frostmaw Yetis out of the snow at 75% and 25%, which
   walk back into him if left alone, and two Rimeglass Sentinels at 50%
   (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Grondmaw)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Grondmaw(monster_def *Def)
{
    Def->Name = "Grondmaw the Avalanche";
    Def->MaxHp = 1850.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 720.f;
    Def->StopRange = 56.f;
    Def->AttackRange = 72.f;
    Def->AttackDamage = 17.f;
    Def->AttackInterval = 0.95f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 72;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.35f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFFD8E8FF;

    // NOTE(zoubir): three rings 120 apart; each takes about 1.9 s to cross
    // the den, so the jumps come about 0.4 s apart
    monster_ability *Roar = AddMonsterAbility(Def, MonsterAbility_Wave,
                                              "Avalanche Roar");
    Roar->MaxRange = 650.f;
    Roar->Cooldown = 6.f;
    Roar->Windup = 1.f;
    Roar->Active = 2.8f;
    Roar->Recover = 0.6f;
    Roar->Damage = 18.f;
    Roar->Radius = 560.f;
    Roar->Speed = 290.f;
    Roar->Knockback = 260.f;
    Roar->Count = 3;
    Roar->Spread = 120.f;
    Roar->Status = StatusEffect_Slowed;
    Roar->StatusSeconds = 1.5f;

    monster_ability *Fist = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Glacier Fist");
    Fist->MaxRange = 95.f;
    Fist->Cooldown = 4.f;
    Fist->Windup = 0.75f;
    Fist->Active = 0.3f;
    Fist->Recover = 0.6f;
    Fist->Damage = 29.f;
    Fist->Radius = 115.f;
    Fist->Knockback = 600.f;

    monster_ability *Hurl = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                              "Boulder Hurl");
    Hurl->MinRange = 120.f;
    Hurl->MaxRange = 560.f;
    Hurl->Cooldown = 4.5f;
    Hurl->Windup = 1.f;
    Hurl->Active = 0.3f;
    Hurl->Recover = 0.5f;
    Hurl->Damage = 18.f;
    Hurl->Radius = 66.f;
    Hurl->Knockback = 350.f;
    Hurl->Count = 3;
    Hurl->Spread = 150.f;
    Hurl->Status = StatusEffect_Slowed;
    Hurl->StatusSeconds = 2.f;

    monster_ability *Rush = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Avalanche Rush");
    Rush->MinRange = 220.f;
    Rush->MaxRange = 600.f;
    Rush->Cooldown = 8.f;
    Rush->Windup = 0.8f;
    Rush->Active = 0.75f;
    Rush->Recover = 0.9f;
    Rush->Damage = 22.f;
    Rush->Radius = 44.f;
    Rush->Speed = 640.f;
    Rush->Knockback = 600.f;

    monster_ability *Grip = AddMonsterAbility(Def, MonsterAbility_Smite,
                                              "Crushing Grip");
    Grip->MaxRange = 560.f;
    Grip->Cooldown = 11.f;
    Grip->Windup = 1.f;
    Grip->Active = 0.3f;
    Grip->Recover = 0.6f;
    Grip->Damage = 32.f;
    Grip->Radius = 24.f;
    Grip->Spread = 60.f;
    Grip->Knockback = 350.f;
}

#else

internal void
DrawMonster_Grondmaw(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Fur = Ramp(ART_RGB(84, 98, 128), ART_RGB(146, 162, 190),
                          ART_RGB(204, 216, 234), ART_RGB(244, 248, 255));
    color_ramp Skin = Ramp(ART_RGB(34, 44, 70), ART_RGB(58, 74, 110),
                           ART_RGB(90, 110, 150), ART_RGB(128, 150, 190));
    color_ramp Glacier = Ramp(ART_RGB(36, 90, 140), ART_RGB(80, 160, 210),
                              ART_RGB(150, 220, 245), ART_RGB(230, 250, 255));
    color_ramp Horn = Ramp(ART_RGB(90, 84, 70), ART_RGB(150, 140, 116),
                           ART_RGB(200, 192, 166), ART_RGB(240, 234, 210));

    float Bob = 0.f;
    float Step = 0.f;
    // NOTE(zoubir): both fists, 0 hanging, 1 high over his head
    float Raise = 0.f;
    float Hunch = 0.f;
    float Roar = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 4.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            Hunch = 2.f;
        } break;

        case AnimationType_Cast:
        {
            Raise = Pose.t;
            Roar = Pose.t;
            Bob = -2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Raise = 1.f - 1.3f * Minimum(1.f, 2.5f * Pose.t);
            Bob = 3.f * Minimum(1.f, 2.5f * Pose.t);
            Hunch = 3.f;
            Roar = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Raise = -0.2f;
            Hunch = 4.f;
            Bob = 2.f + 0.6f * Pose.Wave;
        } break;

        default:
        {
            Bob = 0.8f * Pose.Wave;
            Roar = 0.15f + 0.15f * Pose.Wave2;
        } break;
    }

    float Ground = 63.f;
    float HipY = 46.f + Bob;
    float BodyY = HipY - 12.f;

    // NOTE(zoubir): thick legs
    FillLimb(Canvas, V2(28.f, HipY), V2(27.f - Step, Ground - 2.f), 6.f, 5.f, Fur, -0.4f);
    FillBlob(Canvas, 27.f - Step, Ground - 1.f, 6.f, 2.5f, Skin, -0.2f);
    FillLimb(Canvas, V2(40.f, HipY), V2(41.f + Step, Ground - 2.f), 6.f, 5.f, Fur, 0.1f);
    FillBlob(Canvas, 41.f + Step, Ground - 1.f, 6.f, 2.5f, Skin, 0.f);

    // NOTE(zoubir): the slab of glacier grown into his back
    FillTriangle(Canvas, V2(17.f + Hunch, BodyY + 6.f), V2(30.f + Hunch, BodyY - 10.f),
                 V2(12.f + Hunch, BodyY - 22.f), Glacier, 0.1f, 1.f);
    FillTriangle(Canvas, V2(22.f + Hunch, BodyY - 6.f), V2(34.f + Hunch, BodyY - 12.f),
                 V2(26.f + Hunch, BodyY - 28.f), Glacier, 0.3f, 1.f);

    // NOTE(zoubir): the far arm
    v2 FarShoulder = V2(25.f + Hunch, BodyY - 6.f);
    v2 FarFist = FarShoulder + V2(-5.f, 16.f - 28.f * Raise);
    FillLimb(Canvas, FarShoulder, FarFist, 5.5f, 4.5f, Fur, -0.4f);
    FillBlob(Canvas, FarFist.X, FarFist.Y, 5.f, 5.f, Skin, -0.2f);

    // NOTE(zoubir): the barrel of a body, plated with ice
    FillBlob(Canvas, 35.f + 0.5f * Hunch, BodyY, 16.f, 14.f, Fur, 0.f);
    FillBlob(Canvas, 38.f + 0.5f * Hunch, BodyY + 3.f, 8.f, 7.f, Fur, 0.3f);
    for(u32 Icicle = 0; Icicle < 6; Icicle++)
    {
        float X = 23.f + 4.5f * Icicle + 0.5f * Hunch;
        FillTriangle(Canvas, V2(X - 1.8f, BodyY + 11.f), V2(X + 1.8f, BodyY + 11.f),
                     V2(X, BodyY + 17.f - 2.f * (Icicle % 2)), Glacier, 0.4f, 1.f);
    }

    // NOTE(zoubir): the near arm and a fist like a boulder
    v2 Shoulder = V2(45.f + Hunch, BodyY - 4.f);
    v2 Fist = Shoulder + V2(8.f, 14.f - 28.f * Raise);
    FillLimb(Canvas, Shoulder, Fist, 6.f, 5.f, Fur, 0.2f);
    FillBlob(Canvas, Fist.X, Fist.Y, 6.5f, 6.f, Skin, 0.2f);
    if (Raise > 0.6f)
    {
        FillBlob(Canvas, Fist.X, Fist.Y - 5.f, 4.f, 3.f, Glacier, 0.5f);
    }

    // NOTE(zoubir): the head: a blue face under a mane, curled horns, a
    // jaw that drops wide to roar
    float HeadX = 46.f + Hunch;
    float HeadY = BodyY - 11.f + 0.5f * Hunch;
    FillBlob(Canvas, HeadX - 2.f, HeadY - 1.f, 9.f, 8.f, Fur, 0.25f);
    FillBlob(Canvas, HeadX + 2.5f, HeadY + 1.f, 6.f, 5.5f, Skin, 0.1f);
    FillLimb(Canvas, V2(HeadX - 4.f, HeadY - 6.f), V2(HeadX - 10.f, HeadY - 13.f), 2.5f, 1.f, Horn, 0.3f);
    FillLimb(Canvas, V2(HeadX + 1.f, HeadY - 7.f), V2(HeadX + 6.f, HeadY - 14.f), 2.5f, 1.f, Horn, 0.5f);
    FillDot(Canvas, HeadX + 4.f, HeadY - 1.5f, 1.3f, ART_RGB(160, 230, 255));
    FillDot(Canvas, HeadX + 0.5f, HeadY - 1.5f, 1.1f, ART_RGB(120, 200, 240));
    FillDot(Canvas, HeadX + 4.f, HeadY + 3.f + 3.f * Roar, 1.5f + 2.f * Roar, ART_RGB(26, 16, 36));
    FillTriangle(Canvas, V2(HeadX + 2.f, HeadY + 2.5f), V2(HeadX + 3.5f, HeadY + 2.5f),
                 V2(HeadX + 2.7f, HeadY + 5.5f), Glacier, 0.8f, 1.f);
    FillTriangle(Canvas, V2(HeadX + 5.f, HeadY + 2.5f), V2(HeadX + 6.5f, HeadY + 2.5f),
                 V2(HeadX + 5.7f, HeadY + 5.5f), Glacier, 0.8f, 1.f);

    OutlineFrame(Canvas, ART_RGB(12, 16, 30));
}

#endif
