/* Sskarra the Cinder Wyrm: the Ember Depths' second boss (sim/dungeon/,
   docs/dungeon-depths.md), in Wyrm's Gullet. A worm as thick as a cart,
   plated in cooled black rock that cracks orange where it bends, with a
   crown of basalt spikes round a maw full of fire. Never roams
   (SpawnWeight 0).

   Calm:    Magma Dive sinks into the floor and tunnels after a player;
            the ring locks a moment before she bursts out under it.
            Magma Spit throws a fan of four burning globs. Tail Lash
            sweeps everything near her toward the lava round the rim.
   Enraged (below 45% health): faster, white-hot, and Molten Rain: three
            spots of falling magma that set the struck burning.
   The dungeon adds two Dune Lurkers at 66% and 33% that crawl back into
   her and heal her if they live too long (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(CinderWyrm)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_CinderWyrm(monster_def *Def)
{
    Def->Name = "Sskarra the Cinder Wyrm";
    Def->MaxHp = 1150.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 700.f;
    Def->StopRange = 50.f;
    Def->AttackRange = 64.f;
    Def->AttackDamage = 17.f;
    Def->AttackInterval = 0.85f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Special] = 4;
    Def->SecondsPerFrame[MonsterRow_Special] = 0.12f;

    Def->EnrageHpShare = 0.45f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFFB0E0FF;

    monster_ability *Rain = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                              "Molten Rain");
    Rain->MaxRange = 520.f;
    Rain->Cooldown = 5.f;
    Rain->Windup = 1.1f;
    Rain->Active = 0.3f;
    Rain->Recover = 0.5f;
    Rain->Damage = 13.f;
    Rain->Radius = 50.f;
    Rain->Knockback = 200.f;
    Rain->Count = 3;
    Rain->Spread = 160.f;
    Rain->Status = StatusEffect_Burning;
    Rain->StatusSeconds = 2.f;
    Rain->PhaseMask = PHASE_ENRAGED;

    // NOTE(zoubir): her answer to a party that stands round her: a
    // sweep that throws them toward the lava at the rim
    monster_ability *Lash = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Tail Lash");
    Lash->MaxRange = 90.f;
    Lash->Cooldown = 3.8f;
    Lash->Windup = 0.9f;
    Lash->Active = 0.3f;
    Lash->Recover = 0.5f;
    Lash->Damage = 22.f;
    Lash->Radius = 105.f;
    Lash->Knockback = 750.f;

    monster_ability *Dive = AddMonsterAbility(Def, MonsterAbility_Burrow,
                                              "Magma Dive");
    Dive->MinRange = 110.f;
    Dive->MaxRange = 560.f;
    Dive->Cooldown = 5.5f;
    Dive->Windup = 0.7f;
    Dive->Active = 2.f;
    Dive->Recover = 0.9f;
    Dive->Damage = 26.f;
    Dive->Radius = 72.f;
    Dive->Knockback = 600.f;
    Dive->Status = StatusEffect_Burning;
    Dive->StatusSeconds = 2.f;

    monster_ability *Spit = AddMonsterAbility(Def, MonsterAbility_Volley,
                                              "Magma Spit");
    Spit->MinRange = 90.f;
    Spit->MaxRange = 440.f;
    Spit->Cooldown = 2.8f;
    Spit->Windup = 0.7f;
    Spit->Active = 1.4f;
    Spit->Recover = 0.4f;
    Spit->Damage = 10.f;
    Spit->Radius = 18.f;
    Spit->Speed = 290.f;
    Spit->Knockback = 120.f;
    Spit->Count = MAX_ABILITY_POINTS;
    Spit->Spread = 55.f;
    Spit->ShotStyle = ShotStyle_Ember;
    Spit->Status = StatusEffect_Burning;
    Spit->StatusSeconds = 1.5f;
}

#else

internal void
DrawMonster_CinderWyrm(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Rock = Ramp(ART_RGB(18, 16, 18), ART_RGB(40, 34, 36),
                           ART_RGB(70, 60, 60), ART_RGB(108, 94, 90));
    color_ramp Magma = Ramp(ART_RGB(150, 30, 10), ART_RGB(226, 90, 20),
                            ART_RGB(255, 160, 40), ART_RGB(255, 232, 150));
    color_ramp Spike = Ramp(ART_RGB(30, 26, 30), ART_RGB(60, 52, 58),
                            ART_RGB(96, 86, 92), ART_RGB(140, 128, 130));

    if (Pose.Anim == AnimationType_JumpDown)
    {
        // NOTE(zoubir): under the floor: a mound of cracked rock with
        // magma welling through it
        float Churn = Pose.Wave;
        FillBlob(Canvas, 32.f, 52.f, 16.f + Churn, 5.f, Rock, -0.1f);
        for(u32 Crack = 0; Crack < 5; Crack++)
        {
            float X = 20.f + 6.f * Crack + 1.5f * Pose.Wave2;
            float Y = 50.f - 2.f * (float)((Crack + Pose.Frame) % 3);
            FillDot(Canvas, X, Y, 1.4f, Magma.C[2 + (Crack % 2)]);
        }
        OutlineFrame(Canvas, ART_RGB(10, 6, 6));
        return;
    }

    // NOTE(zoubir): how far the body rears, how far the head leans
    // forward, how wide the maw opens
    float Rear = 0.f;
    float Lean = 0.f;
    float Open = 0.25f;
    float Sway = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Sway = 2.5f * Pose.Wave;
            Rear = 1.f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Rear = 8.f * Pose.t;
            Lean = -3.f * Pose.t;
            Open = 0.25f + 0.75f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Rear = 8.f * (1.f - Pose.t);
            Lean = 8.f * Pose.t;
            Open = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Rear = -3.f;
            Open = 0.1f;
            Sway = 0.5f * Pose.Wave;
        } break;

        default:
        {
            Rear = 1.5f * Pose.Wave;
            Sway = Pose.Wave2;
        } break;
    }

    // NOTE(zoubir): coils on the ground at the back, then segments rising
    // to the head, each a plate with a seam of magma
    FillBlob(Canvas, 16.f, 52.f, 12.f, 5.f, Rock, -0.3f);
    FillDot(Canvas, 14.f, 50.f, 1.2f, Magma.C[1]);
    v2 Base = V2(22.f, 50.f);
    v2 Neck = V2(34.f + Lean + Sway, 30.f - Rear);
    for(u32 Segment = 0; Segment < 6; Segment++)
    {
        float t = (float)Segment / 5.f;
        v2 P = Lerp2(Base, t, Neck);
        P.X -= 4.f * Sin(3.14159f * t);
        float R = 8.f - 2.f * t;
        FillBlob(Canvas, P.X, P.Y, R, R * 0.85f, Rock, 0.05f + 0.2f * t);
        FillLimb(Canvas, V2(P.X - R * 0.6f, P.Y + 1.f), V2(P.X + R * 0.6f, P.Y),
                 0.7f, 0.7f, Magma, 0.3f);
    }

    // NOTE(zoubir): the head: a blunt wedge with a crown of spikes
    float HeadX = Neck.X + 6.f;
    float HeadY = Neck.Y - 3.f;
    FillBlob(Canvas, HeadX, HeadY, 9.f, 7.f, Rock, 0.2f);
    for(u32 Crown = 0; Crown < 4; Crown++)
    {
        float X = HeadX - 7.f + 3.5f * Crown;
        FillLimb(Canvas, V2(X, HeadY - 5.f), V2(X - 3.f, HeadY - 12.f + Crown),
                 1.6f, 0.4f, Spike, 0.1f);
    }
    // NOTE(zoubir): the maw, open on fire
    float Gape = 1.f + 5.f * Open;
    FillBlob(Canvas, HeadX + 6.f, HeadY + 2.f, 4.f + Open, Gape * 0.6f + 1.f, Magma, 0.4f);
    FillDot(Canvas, HeadX + 7.f, HeadY + 2.f, 1.f + 1.5f * Open, Magma.C[3]);
    FillTriangle(Canvas, V2(HeadX + 3.f, HeadY - 1.f), V2(HeadX + 11.f, HeadY - 1.f),
                 V2(HeadX + 8.f, HeadY + 2.f - Gape * 0.4f), Rock, 0.3f, 0.1f);
    FillDot(Canvas, HeadX + 1.f, HeadY - 2.f, 1.4f, Magma.C[3]);
    if (Open > 0.8f)
    {
        FillDot(Canvas, HeadX + 13.f, HeadY + 3.f, 1.5f, Magma.C[2]);
        FillDot(Canvas, HeadX + 15.f, HeadY + 1.f, 1.f, Magma.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(10, 6, 6));
}

#endif
