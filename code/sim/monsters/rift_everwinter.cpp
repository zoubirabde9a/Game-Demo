/* Vaelith the Everwinter Queen: the Aurora Rift's last boss
   (sim/dungeon/, docs/dungeon-rift.md), on the Everwinter Throne. The
   queen the aurora fell for: a tall woman of blue ice in a gown that
   spreads into the floor, a crown of aurora light, two blades of ice
   circling her. Never roams (SpawnWeight 0). She asks for everything the
   rift taught at once.

   Calm:    Winter's Breath rolls three rings of frost through the hall
            to jump. Aurora Scythe sweeps a beam most of the way round her,
            stopped only by the pillars. Rimeglass Court raises Rimeglass
            Sentinels, two at most, that shatter when they die. The
            Everwinter Kiss freezes whoever holds her, past any dodge.
   Enraged (below 30% health): faster, and Frozen Tempest: shards out
            all the way round her.
   At 70% and 35% she raises two Aurora Pylons and takes nothing while
   one stands; at 85%, 55% and 20% Frostmaw Yetis come out of the snow
   and walk back into her if left alone (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Everwinter)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Everwinter(monster_def *Def)
{
    Def->Name = "Vaelith the Everwinter";
    Def->MaxHp = 2050.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 760.f;
    Def->StopRange = 54.f;
    Def->AttackRange = 70.f;
    Def->AttackDamage = 17.f;
    Def->AttackInterval = 0.95f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.3f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.65f;
    Def->EnrageTint = 0xFFFFE0F0;

    monster_ability *Tempest = AddMonsterAbility(Def, MonsterAbility_Volley,
                                                 "Frozen Tempest");
    Tempest->MaxRange = 520.f;
    Tempest->Cooldown = 5.f;
    Tempest->Windup = 0.7f;
    Tempest->Active = 1.6f;
    Tempest->Recover = 0.5f;
    Tempest->Damage = 12.f;
    Tempest->Radius = 18.f;
    Tempest->Speed = 290.f;
    Tempest->Knockback = 160.f;
    Tempest->Count = 7;
    Tempest->Spread = 300.f;
    Tempest->ShotStyle = ShotStyle_Spine;
    Tempest->Status = StatusEffect_Slowed;
    Tempest->StatusSeconds = 2.f;
    Tempest->PhaseMask = PHASE_ENRAGED;

    monster_ability *Breath = AddMonsterAbility(Def, MonsterAbility_Wave,
                                                "Winter's Breath");
    Breath->MaxRange = 650.f;
    Breath->Cooldown = 8.f;
    Breath->Windup = 1.f;
    Breath->Active = 2.8f;
    Breath->Recover = 0.6f;
    Breath->Damage = 22.f;
    Breath->Radius = 600.f;
    Breath->Speed = 320.f;
    Breath->Knockback = 280.f;
    Breath->Count = 3;
    Breath->Spread = 140.f;
    Breath->Status = StatusEffect_Slowed;
    Breath->StatusSeconds = 1.5f;

    // NOTE(zoubir): most of the way round her, so only a pillar's shadow
    // or the far side of the sweep is safe
    monster_ability *Scythe = AddMonsterAbility(Def, MonsterAbility_Beam,
                                                "Aurora Scythe");
    Scythe->MaxRange = 620.f;
    Scythe->Cooldown = 9.f;
    Scythe->Windup = 1.f;
    Scythe->Active = 2.2f;
    Scythe->Recover = 0.6f;
    Scythe->Damage = 24.f;
    Scythe->Radius = 16.f;
    Scythe->Speed = 680.f;
    Scythe->Spread = 220.f;
    Scythe->Knockback = 120.f;

    monster_ability *Court = AddMonsterAbility(Def, MonsterAbility_Summon,
                                               "Rimeglass Court");
    Court->MaxRange = 640.f;
    Court->Cooldown = 18.f;
    Court->Windup = 1.f;
    Court->Active = 0.3f;
    Court->Recover = 0.5f;
    Court->SummonKind = MonsterKind_Sentinel;
    Court->Count = 2;
    Court->MaxActive = 2;
    Court->Spread = 100.f;
    Court->Radius = 18.f;

    monster_ability *Kiss = AddMonsterAbility(Def, MonsterAbility_Smite,
                                              "Everwinter Kiss");
    Kiss->MaxRange = 560.f;
    Kiss->Cooldown = 12.f;
    Kiss->Windup = 1.f;
    Kiss->Active = 0.3f;
    Kiss->Recover = 0.6f;
    Kiss->Damage = 34.f;
    Kiss->Radius = 24.f;
    Kiss->Spread = 60.f;
    Kiss->Knockback = 300.f;
    Kiss->Status = StatusEffect_Slowed;
    Kiss->StatusSeconds = 2.f;
}

#else

internal void
DrawMonster_Everwinter(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Ice = Ramp(ART_RGB(40, 80, 130), ART_RGB(90, 156, 210),
                          ART_RGB(164, 220, 246), ART_RGB(236, 250, 255));
    color_ramp Gown = Ramp(ART_RGB(26, 44, 90), ART_RGB(50, 84, 150),
                           ART_RGB(96, 140, 206), ART_RGB(170, 206, 245));
    color_ramp Skin = Ramp(ART_RGB(120, 160, 200), ART_RGB(170, 206, 236),
                           ART_RGB(214, 236, 252), ART_RGB(248, 254, 255));
    color_ramp Green = Ramp(ART_RGB(20, 100, 80), ART_RGB(40, 190, 140),
                            ART_RGB(130, 250, 190), ART_RGB(225, 255, 240));
    color_ramp Violet = Ramp(ART_RGB(64, 32, 116), ART_RGB(124, 74, 196),
                             ART_RGB(186, 140, 246), ART_RGB(244, 228, 255));

    float Bob = 0.f;
    // NOTE(zoubir): the casting hand, 0 at her side, 1 raised high
    float Lift = 0.f;
    float Glow = 0.4f;
    float Orbit = Pose.t;
    float Sway = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Bob = -1.f * Absolute(Pose.Wave);
            Sway = 2.f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            Lift = Pose.t;
            Glow = 0.4f + 0.6f * Pose.t;
            Bob = -1.5f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Lift = 1.f - Pose.t;
            Glow = 1.f;
            Sway = 3.f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            Glow = 0.25f;
            Bob = 1.5f;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Glow = 0.4f + 0.15f * Pose.Wave2;
            Sway = Pose.Wave2;
        } break;
    }

    float Ground = 57.f;
    float WaistY = 34.f + Bob;

    // NOTE(zoubir): a blade of ice circling behind her
    float BladeAngle = 2.f * Pi32 * Orbit;
    if (Sin(BladeAngle) < 0.f)
    {
        v2 Blade = V2(32.f + 18.f * Cos(BladeAngle), WaistY - 6.f + 5.f * Sin(BladeAngle));
        FillTriangle(Canvas, Blade + V2(-2.f, 4.f), Blade + V2(2.f, 4.f), Blade + V2(0.f, -8.f), Ice, 0.2f, 0.9f);
    }

    // NOTE(zoubir): a gown of glacier spreading into the floor
    FillTriangle(Canvas, V2(32.f, WaistY - 4.f), V2(17.f - Sway, Ground), V2(47.f + Sway, Ground),
                 Gown, 0.1f, 0.6f);
    FillTriangle(Canvas, V2(32.f, WaistY), V2(22.f - Sway, Ground), V2(36.f, Ground), Ice, 0.2f, 0.8f);
    FillFlatEllipse(Canvas, 32.f, Ground, 16.f + Sway, 2.f, Ice.C[2]);

    // NOTE(zoubir): the far arm, raised as she casts
    v2 FarShoulder = V2(28.f, WaistY - 11.f);
    v2 FarHand = FarShoulder + V2(-4.f + 3.f * Lift, 9.f - 21.f * Lift);
    FillLimb(Canvas, FarShoulder, FarHand, 1.8f, 1.4f, Skin, -0.3f);
    if (Lift > 0.3f)
    {
        FillBlob(Canvas, FarHand.X, FarHand.Y - 2.f, 2.f + 2.5f * Lift, 2.f + 2.5f * Lift, Green, 0.4f);
    }

    // NOTE(zoubir): a slender body of ice, a bodice of deep blue
    FillBlob(Canvas, 32.f, WaistY - 7.f, 5.f, 8.f, Gown, 0.1f);
    FillBlob(Canvas, 32.5f, WaistY - 11.f, 4.f, 3.f, Skin, 0.3f);

    // NOTE(zoubir): her face, long white hair, a crown of aurora light
    float HeadX = 33.f;
    float HeadY = WaistY - 18.f;
    FillLimb(Canvas, V2(HeadX - 3.f, HeadY - 2.f), V2(HeadX - 7.f - Sway, HeadY + 12.f), 3.f, 1.5f, Skin, 0.5f);
    FillBlob(Canvas, HeadX, HeadY, 4.f, 4.5f, Skin, 0.3f);
    FillDot(Canvas, HeadX + 2.f, HeadY - 0.5f, 1.f, Green.C[2 + (Glow > 0.7f ? 1 : 0)]);
    for(u32 Ray = 0; Ray < 5; Ray++)
    {
        float X = HeadX - 4.f + 2.f * Ray;
        float Tall = (Ray == 2) ? 9.f : (Ray % 2 ? 6.f : 4.f);
        Tall *= 0.8f + 0.4f * Glow;
        FillTriangle(Canvas, V2(X - 1.f, HeadY - 4.f), V2(X + 1.f, HeadY - 4.f),
                     V2(X, HeadY - 4.f - Tall), (Ray % 2) ? Violet : Green, 0.5f, 1.f);
    }

    // NOTE(zoubir): the near arm, and a sceptre of ice
    v2 Shoulder = V2(36.f, WaistY - 10.f);
    v2 Hand = Shoulder + V2(4.f, 8.f);
    FillLimb(Canvas, Shoulder, Hand, 1.8f, 1.4f, Skin, 0.2f);
    v2 Top = Hand + V2(2.f + 3.f * Lift, -18.f);
    FillLimb(Canvas, Hand + V2(-1.f, 8.f), Top, 1.f, 1.f, Ice, 0.3f);
    FillBlob(Canvas, Top.X, Top.Y, 2.f + Glow, 2.5f + Glow, Violet, 0.5f);

    // NOTE(zoubir): the other blade, in front of her
    if (Sin(BladeAngle) >= 0.f)
    {
        v2 Blade = V2(32.f + 18.f * Cos(BladeAngle), WaistY - 6.f + 5.f * Sin(BladeAngle));
        FillTriangle(Canvas, Blade + V2(-2.5f, 4.f), Blade + V2(2.5f, 4.f), Blade + V2(0.f, -9.f), Ice, 0.4f, 1.f);
    }
    v2 Second = V2(32.f - 18.f * Cos(BladeAngle), WaistY - 6.f - 5.f * Sin(BladeAngle));
    FillTriangle(Canvas, Second + V2(-2.f, 4.f), Second + V2(2.f, 4.f), Second + V2(0.f, -8.f), Ice,
                 Sin(BladeAngle) < 0.f ? 0.4f : 0.2f, 0.9f);

    OutlineFrame(Canvas, ART_RGB(10, 14, 30));
}

#endif
