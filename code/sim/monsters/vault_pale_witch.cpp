/* Ysolde the Pale Witch: the Rimeheart Vault's second boss
   (sim/dungeon/, docs/dungeon-vault.md), in the Mirror Mere. A gaunt
   woman in white robes that trail into frost, floating a hand above the
   ice, silver hair streaming behind her and a staff topped with a shard
   of blue ice. Never roams (SpawnWeight 0).

   Calm:    Shard Volley throws a fan of ice shards that slow; Hailstorm
            drops hail where the party is heading; Mirror Step vanishes
            and comes out of the ice behind her target; Mirror Shades
            calls two Hollow Shades out of the mere, three at most. Heart
            Freeze: a cold that stops the heart of whoever she is after,
            which nobody can dodge.
   Enraged (below 35% health): faster, and everything comes round sooner.
   The dungeon looses chilling bats at 66% and 33% that fly back into her
   if left alone (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(PaleWitch)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_PaleWitch(monster_def *Def)
{
    Def->Name = "Ysolde the Pale Witch";
    Def->MaxHp = 1450.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 720.f;
    Def->StopRange = 50.f;
    Def->AttackRange = 64.f;
    Def->AttackDamage = 13.f;
    Def->AttackInterval = 0.9f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;

    Def->EnrageHpShare = 0.35f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.65f;
    Def->EnrageTint = 0xFFFFD0C0;

    monster_ability *Volley = AddMonsterAbility(Def, MonsterAbility_Volley,
                                                "Shard Volley");
    Volley->MaxRange = 520.f;
    Volley->Cooldown = 2.8f;
    Volley->Windup = 0.6f;
    Volley->Active = 1.4f;
    Volley->Recover = 0.4f;
    Volley->Damage = 12.f;
    Volley->Radius = 18.f;
    Volley->Speed = 330.f;
    Volley->Knockback = 120.f;
    Volley->Count = 5;
    Volley->Spread = 60.f;
    Volley->ShotStyle = ShotStyle_Spine;
    Volley->Status = StatusEffect_Slowed;
    Volley->StatusSeconds = 1.5f;

    monster_ability *Hail = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                              "Hailstorm");
    Hail->MinRange = 100.f;
    Hail->MaxRange = 560.f;
    Hail->Cooldown = 5.f;
    Hail->Windup = 0.9f;
    Hail->Active = 0.3f;
    Hail->Recover = 0.5f;
    Hail->Damage = 12.f;
    Hail->Radius = 55.f;
    Hail->Knockback = 200.f;
    Hail->Count = MAX_ABILITY_POINTS;
    Hail->Spread = 160.f;
    Hail->Status = StatusEffect_Slowed;
    Hail->StatusSeconds = 2.f;

    // NOTE(zoubir): she leaves the tank for the back line, like Vol'karr's
    // Flame Step, more often and with less behind it
    monster_ability *Step = AddMonsterAbility(Def, MonsterAbility_Blink,
                                              "Mirror Step");
    Step->MinRange = 160.f;
    Step->MaxRange = 540.f;
    Step->Cooldown = 6.f;
    Step->Windup = 0.7f;
    Step->Active = 0.3f;
    Step->Recover = 0.6f;
    Step->Damage = 15.f;
    Step->Radius = 80.f;
    Step->Spread = 60.f;
    Step->Knockback = 450.f;
    Step->Status = StatusEffect_Slowed;
    Step->StatusSeconds = 1.5f;

    monster_ability *Shades = AddMonsterAbility(Def, MonsterAbility_Summon,
                                                "Mirror Shades");
    Shades->MaxRange = 560.f;
    Shades->Cooldown = 12.f;
    Shades->Windup = 1.f;
    Shades->Active = 0.3f;
    Shades->Recover = 0.5f;
    Shades->SummonKind = MonsterKind_Shade;
    Shades->Count = 2;
    Shades->MaxActive = 3;
    Shades->Spread = 80.f;
    Shades->Radius = 14.f;

    monster_ability *Freeze = AddMonsterAbility(Def, MonsterAbility_Smite,
                                                "Heart Freeze");
    Freeze->MaxRange = 560.f;
    Freeze->Cooldown = 11.f;
    Freeze->Windup = 1.f;
    Freeze->Active = 0.3f;
    Freeze->Recover = 0.5f;
    Freeze->Damage = 30.f;
    Freeze->Radius = 22.f;
    Freeze->Knockback = 150.f;
    Freeze->Status = StatusEffect_Slowed;
    Freeze->StatusSeconds = 2.5f;
}

#else

internal void
DrawMonster_PaleWitch(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Robe = Ramp(ART_RGB(70, 90, 130), ART_RGB(130, 154, 190),
                           ART_RGB(196, 214, 236), ART_RGB(244, 250, 255));
    color_ramp Skin = Ramp(ART_RGB(90, 110, 140), ART_RGB(150, 172, 200),
                           ART_RGB(200, 218, 236), ART_RGB(236, 244, 252));
    color_ramp Hair = Ramp(ART_RGB(110, 120, 140), ART_RGB(160, 170, 188),
                           ART_RGB(206, 214, 228), ART_RGB(244, 248, 255));
    color_ramp Staff = Ramp(ART_RGB(40, 36, 50), ART_RGB(70, 64, 84),
                            ART_RGB(104, 98, 120), ART_RGB(150, 144, 166));
    color_ramp Shard = Ramp(ART_RGB(30, 90, 170), ART_RGB(70, 160, 230),
                            ART_RGB(150, 220, 255), ART_RGB(240, 255, 255));

    // NOTE(zoubir): she floats, so she bobs in every pose
    float Bob = 1.5f * Pose.Wave;
    // NOTE(zoubir): the staff's angle from straight up, forward positive
    float Swing = 0.15f;
    float Glow = 0.4f;
    float Stream = 0.f;
    float Lean = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Stream = 2.f + Pose.Wave;
            Lean = 2.f;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): the staff rises, the shard flaring
            Swing = 0.15f - 0.5f * Pose.t;
            Glow = 0.4f + 0.6f * Pose.t;
            Bob = -2.f * Pose.t;
            Stream = 1.f + 2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Swing = -0.35f + 1.6f * Minimum(1.f, 2.f * Pose.t);
            Glow = 1.f - 0.5f * Pose.t;
            Lean = 3.f * Minimum(1.f, 2.f * Pose.t);
            Stream = 3.f;
        } break;

        case AnimationType_Stop:
        {
            Swing = 0.5f;
            Bob = 2.f + 0.5f * Pose.Wave;
            Glow = 0.2f;
        } break;

        default:
        {
            Glow = 0.35f + 0.15f * Pose.Wave2;
            Stream = 0.5f * Pose.Wave2;
        } break;
    }

    float WaistY = 36.f + Bob;
    float ChestX = 31.f + Lean;
    float ChestY = WaistY - 9.f;

    // NOTE(zoubir): a frost shadow on the ice beneath her
    FillFlatEllipse(Canvas, 32.f, 57.f, 10.f, 2.f, ART_RGB(150, 200, 230));

    // NOTE(zoubir): hair streaming back behind everything
    v2 HeadC = V2(ChestX + 3.f, ChestY - 10.f);
    for(u32 Lock = 0; Lock < 3; Lock++)
    {
        float Drop = 3.f * Lock;
        v2 Tip = V2(HeadC.X - 13.f - 2.f * Stream - 2.f * Lock,
                    HeadC.Y + 6.f + Drop + Pose.Wave2 * (1.f + Lock));
        FillLimb(Canvas, HeadC + V2(-2.f, -1.f + Lock), Tip, 2.5f, 0.8f, Hair, -0.2f + 0.2f * Lock);
    }

    // NOTE(zoubir): the robe, flaring into a hem that frays into frost
    float Hem = 52.f + 0.5f * Bob;
    FillTriangle(Canvas, V2(ChestX, ChestY), V2(21.f - Stream, Hem), V2(41.f + 0.5f * Lean, Hem),
                 Robe, 0.6f, 0.1f);
    for(u32 Frill = 0; Frill < 5; Frill++)
    {
        float X = 22.f - Stream + 4.5f * Frill;
        FillDot(Canvas, X, Hem + 1.f + ((Frill + Pose.Frame) % 2), 1.6f, Robe.C[3 - (Frill % 2)]);
    }
    FillBlob(Canvas, ChestX, ChestY + 2.f, 4.5f, 7.5f, Robe, 0.2f);

    // NOTE(zoubir): the far arm, raised as she casts
    v2 FarShoulder = V2(ChestX - 4.f, ChestY - 3.f);
    v2 FarHand = FarShoulder + V2(-3.f, 9.f - 14.f * Glow);
    FillLimb(Canvas, FarShoulder, FarHand, 1.8f, 1.4f, Robe, -0.3f);
    if (Glow > 0.6f)
    {
        FillDot(Canvas, FarHand.X, FarHand.Y, 2.f, Shard.C[2]);
    }

    // NOTE(zoubir): a gaunt pale face, a circlet of ice, cold eyes
    FillBlob(Canvas, HeadC.X, HeadC.Y, 4.5f, 5.f, Skin, 0.3f);
    FillLimb(Canvas, HeadC + V2(-4.f, -3.f), HeadC + V2(3.f, -5.f), 1.2f, 1.f, Hair, 0.4f);
    FillTriangle(Canvas, V2(HeadC.X - 2.f, HeadC.Y - 4.f), V2(HeadC.X + 1.f, HeadC.Y - 5.f),
                 V2(HeadC.X - 0.5f, HeadC.Y - 10.f), Shard, 0.3f, 1.f);
    FillDot(Canvas, HeadC.X + 2.f, HeadC.Y - 0.5f, 1.f, Shard.C[2 + (Glow > 0.7f ? 1 : 0)]);

    // NOTE(zoubir): the near arm and the staff, swung round the hand
    v2 Shoulder = V2(ChestX + 3.f, ChestY - 2.f);
    v2 Hand = Shoulder + V2(5.f, 6.f);
    FillLimb(Canvas, Shoulder, Hand, 1.8f, 1.5f, Robe, 0.2f);
    v2 Up = V2(Sin(Swing), -Cos(Swing));
    v2 Butt = Hand - 13.f * Up;
    v2 Top = Hand + 15.f * Up;
    FillLimb(Canvas, Butt, Top, 1.1f, 1.3f, Staff, 0.1f);
    FillDot(Canvas, Hand.X, Hand.Y, 1.6f, Skin.C[2]);
    FillTriangle(Canvas, Top + V2(-2.5f, 0.f), Top + V2(2.5f, 0.f), Top + 8.f * Up, Shard, 0.2f, 1.f);
    FillBlob(Canvas, Top.X, Top.Y, 2.5f + Glow, 2.5f + Glow, Shard, 0.4f);
    if (Glow > 0.7f)
    {
        FillDot(Canvas, Top.X, Top.Y, 1.5f + Glow, Shard.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(14, 20, 34));
}

#endif
