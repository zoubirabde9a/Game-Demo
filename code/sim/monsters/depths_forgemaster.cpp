/* Forgemaster Kragg: the Ember Depths' first boss (sim/dungeon/,
   docs/dungeon-depths.md), in the Anvil Hall. A squat giant of black
   iron plates riveted over a molten core that glows through the seams
   of his chest, a beard of slag, and a forge hammer as long as he is
   tall. Never roams (SpawnWeight 0).

   Calm:    Anvil Drop brings the hammer down in a wide ring that sets the
            struck burning and leaves embers on the floor; Hammer Hurl
            lobs three white-hot ingots at the party's back line, setting
            the struck burning; Bellows Rush charges along a locked
            line at a player far from him.
   Enraged (below half health): faster, glowing red, and Stoke the
            Forge: two Cinder Imps climb out of the coals, three at most.
   The dungeon adds an armoured Anvil Guard at 70% and 35% that walks
   back into him if it lives too long (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Forgemaster)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Forgemaster(monster_def *Def)
{
    Def->Name = "Forgemaster Kragg";
    Def->MaxHp = 900.f;
    Def->Acceleration = 25000.f;
    Def->AggroRange = 640.f;
    Def->StopRange = 52.f;
    Def->AttackRange = 66.f;
    Def->AttackDamage = 16.f;
    Def->AttackInterval = 1.1f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.5f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFF7080FF;

    monster_ability *Stoke = AddMonsterAbility(Def, MonsterAbility_Summon,
                                               "Stoke the Forge");
    Stoke->MaxRange = 520.f;
    Stoke->Cooldown = 11.f;
    Stoke->Windup = 1.f;
    Stoke->Active = 0.3f;
    Stoke->Recover = 0.5f;
    Stoke->SummonKind = MonsterKind_Imp;
    Stoke->Count = 2;
    Stoke->MaxActive = 3;
    Stoke->Spread = 60.f;
    Stoke->Radius = 14.f;
    Stoke->PhaseMask = PHASE_ENRAGED;

    monster_ability *Anvil = AddMonsterAbility(Def, MonsterAbility_Slam,
                                               "Anvil Drop");
    Anvil->MaxRange = 95.f;
    Anvil->Cooldown = 5.f;
    Anvil->Windup = 1.1f;
    Anvil->Active = 0.3f;
    Anvil->Recover = 0.6f;
    Anvil->Damage = 22.f;
    Anvil->Radius = 115.f;
    Anvil->Knockback = 650.f;
    Anvil->Status = StatusEffect_Burning;
    Anvil->StatusSeconds = 2.f;
    Anvil->HazardSeconds = 4.f;
    Anvil->HazardStyle = HazardStyle_Embers;

    // NOTE(zoubir): the far players are never safe: the ingots land where
    // they are heading, so the healer and the striker have to keep moving
    // while the tank holds him. No burning ground: the back line stood in
    // it and three bots died in seconds
    monster_ability *Hurl = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                              "Hammer Hurl");
    Hurl->MinRange = 120.f;
    Hurl->MaxRange = 480.f;
    Hurl->Cooldown = 5.f;
    Hurl->Windup = 1.f;
    Hurl->Active = 0.3f;
    Hurl->Recover = 0.5f;
    Hurl->Damage = 14.f;
    Hurl->Radius = 55.f;
    Hurl->Knockback = 250.f;
    Hurl->Count = 3;
    Hurl->Spread = 120.f;
    Hurl->Status = StatusEffect_Burning;
    Hurl->StatusSeconds = 1.5f;

    monster_ability *Rush = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Bellows Rush");
    Rush->MinRange = 160.f;
    Rush->MaxRange = 500.f;
    Rush->Cooldown = 7.f;
    Rush->Windup = 0.9f;
    Rush->Active = 0.7f;
    Rush->Recover = 0.7f;
    Rush->Damage = 18.f;
    Rush->Radius = 46.f;
    Rush->Speed = 640.f;
    Rush->Knockback = 700.f;
}

#else

internal void
DrawMonster_Forgemaster(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Iron = Ramp(ART_RGB(22, 20, 24), ART_RGB(48, 44, 50),
                           ART_RGB(82, 76, 84), ART_RGB(128, 120, 126));
    color_ramp Core = Ramp(ART_RGB(150, 40, 10), ART_RGB(230, 100, 20),
                           ART_RGB(255, 170, 50), ART_RGB(255, 236, 160));
    color_ramp Slag = Ramp(ART_RGB(40, 30, 26), ART_RGB(76, 58, 48),
                           ART_RGB(112, 88, 70), ART_RGB(150, 124, 100));
    color_ramp Haft = Ramp(ART_RGB(50, 30, 18), ART_RGB(86, 54, 30),
                           ART_RGB(122, 82, 48), ART_RGB(160, 116, 70));
    color_ramp Head = Ramp(ART_RGB(36, 34, 40), ART_RGB(70, 66, 74),
                           ART_RGB(110, 104, 114), ART_RGB(170, 164, 172));

    float Bob = 0.f;
    float Step = 0.f;
    // NOTE(zoubir): the hammer's angle from straight up, forward positive
    float Swing = 2.f;
    float Glow = 0.5f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
            Swing = 1.9f + 0.15f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): the hammer rises behind his head, the core
            // brightening as the bellows in his chest draw
            Swing = 2.f - 3.f * Pose.t;
            Glow = 0.5f + 0.5f * Pose.t;
            Bob = -1.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Swing = -1.f + 3.2f * Minimum(1.f, 2.f * Pose.t);
            Bob = 2.f * Pose.t;
            Glow = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Swing = 2.2f;
            Bob = 2.f + 0.5f * Pose.Wave;
            Glow = 0.3f;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Glow = 0.45f + 0.15f * Pose.Wave;
        } break;
    }

    float Ground = 56.f;
    float HipY = 41.f + Bob;
    // NOTE(zoubir): short thick legs, the far one darker
    FillLimb(Canvas, V2(28.f, HipY), V2(25.f - Step, Ground - 2.f), 5.f, 4.f, Iron, -0.4f);
    FillBlob(Canvas, 25.f - Step, Ground - 1.f, 6.f, 3.f, Iron, -0.3f);
    FillLimb(Canvas, V2(36.f, HipY), V2(38.f + Step, Ground - 2.f), 5.f, 4.f, Iron, 0.1f);
    FillBlob(Canvas, 38.f + Step, Ground - 1.f, 6.f, 3.f, Iron, 0.1f);

    // NOTE(zoubir): the far arm, hanging
    float BodyY = HipY - 11.f;
    FillLimb(Canvas, V2(24.f, BodyY - 4.f), V2(21.f, BodyY + 9.f), 4.f, 3.5f, Iron, -0.4f);

    // NOTE(zoubir): a barrel of riveted plates, the core showing through
    FillBlob(Canvas, 32.f, BodyY, 14.f, 13.f, Iron, 0.05f);
    FillBlob(Canvas, 34.f, BodyY + 1.f, 6.f + 1.5f * Glow, 7.f + 1.5f * Glow, Core, 0.4f);
    for(u32 Seam = 0; Seam < 3; Seam++)
    {
        float Y = BodyY - 6.f + 6.f * Seam;
        FillLimb(Canvas, V2(22.f, Y), V2(42.f, Y + 1.f), 0.8f, 0.8f, Core, 0.3f);
    }
    FillDot(Canvas, 34.f, BodyY + 1.f, 2.f + 2.f * Glow, Core.C[3]);
    for(u32 Rivet = 0; Rivet < 4; Rivet++)
    {
        FillDot(Canvas, 24.f + 5.f * Rivet, BodyY - 9.f, 0.9f, Iron.C[3]);
    }

    // NOTE(zoubir): the head sunk between the shoulders, a slag beard
    float HeadX = 38.f;
    float HeadY = BodyY - 13.f;
    FillBlob(Canvas, HeadX, HeadY, 6.f, 5.5f, Iron, 0.2f);
    FillBlob(Canvas, HeadX + 1.f, HeadY + 5.f, 5.f, 4.f, Slag, 0.f);
    FillDot(Canvas, HeadX + 3.f, HeadY - 1.f, 1.3f, Core.C[2 + (Glow > 0.7f ? 1 : 0)]);
    FillDot(Canvas, HeadX - 1.f, HeadY - 1.f, 1.1f, Core.C[2]);
    // NOTE(zoubir): a chimney stack on the back, smoking
    FillLimb(Canvas, V2(23.f, BodyY - 8.f), V2(21.f, BodyY - 15.f), 3.5f, 3.5f, Iron, -0.2f);
    FillDot(Canvas, 21.f + Pose.Wave, BodyY - 19.f - 2.f * Pose.t, 2.5f, Slag.C[2]);
    FillDot(Canvas, 21.f, BodyY - 15.f, 1.6f, Core.C[1]);

    // NOTE(zoubir): the near arm and the hammer, swung round the shoulder
    v2 Shoulder = V2(41.f, BodyY - 6.f);
    v2 Up = V2(Sin(Swing), -Cos(Swing));
    v2 Hand = Shoulder + 10.f * Up;
    FillLimb(Canvas, Shoulder, Hand, 4.5f, 4.f, Iron, 0.2f);
    v2 HammerHead = Hand + 13.f * Up;
    FillLimb(Canvas, Hand - 3.f * Up, HammerHead, 1.4f, 1.4f, Haft, 0.1f);
    v2 Across = V2(-Up.Y, Up.X);
    FillLimb(Canvas, HammerHead - 6.f * Across, HammerHead + 6.f * Across, 4.5f, 4.5f, Head, 0.2f);
    if (Glow > 0.8f)
    {
        FillDot(Canvas, HammerHead.X, HammerHead.Y, 2.f, Core.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(10, 8, 8));
}

#endif
