/* The Hollow King: the Sunken Crypt's last boss (sim/dungeon/,
   docs/dungeon-plan.md), on the Throne of Dust. A king's robes and
   armour with nothing inside them: a crown hovering over an empty hood
   where two cold eyes burn, and a greatsword of grey iron. Never roams
   (SpawnWeight 0).

   Calm:    Soul Cleave brings the greatsword down in a wide ring that
            hurts most of the party if the tank lets it reach them;
            Shadow Rush charges across the hall along a locked line.
   Enraged (below 40% health): faster, pale as ash, and Wail of the
            Dead: four souls fly out in an X round its target.
   The dungeon raises two Hollow Shades at 75%, 50% and 25%
   (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(HollowKing)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_HollowKing(monster_def *Def)
{
    Def->Name = "The Hollow King";
    Def->MaxHp = 1700.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 700.f;
    Def->StopRange = 50.f;
    Def->AttackRange = 66.f;
    Def->AttackDamage = 16.f;
    Def->AttackInterval = 1.2f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.4f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.65f;
    Def->EnrageTint = 0xFFE0E0E0;

    monster_ability *Wail = AddMonsterAbility(Def, MonsterAbility_Volley,
                                              "Wail of the Dead");
    Wail->MaxRange = 500.f;
    Wail->Cooldown = 6.f;
    Wail->Windup = 0.9f;
    Wail->Active = 1.6f;
    Wail->Recover = 0.5f;
    Wail->Damage = 14.f;
    Wail->Radius = 18.f;
    Wail->Speed = 260.f;
    Wail->Knockback = 120.f;
    Wail->Count = MAX_ABILITY_POINTS;
    Wail->Spread = 270.f;
    Wail->ShotStyle = ShotStyle_Spine;
    Wail->Status = StatusEffect_Slowed;
    Wail->StatusSeconds = 1.5f;
    Wail->PhaseMask = PHASE_ENRAGED;

    monster_ability *Cleave = AddMonsterAbility(Def, MonsterAbility_Slam,
                                                "Soul Cleave");
    Cleave->MaxRange = 90.f;
    Cleave->Cooldown = 5.5f;
    Cleave->Windup = 1.2f;
    Cleave->Active = 0.3f;
    Cleave->Recover = 0.9f;
    Cleave->Damage = 30.f;
    Cleave->Radius = 120.f;
    Cleave->Knockback = 700.f;

    monster_ability *Rush = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Shadow Rush");
    Rush->MinRange = 140.f;
    Rush->MaxRange = 520.f;
    Rush->Cooldown = 8.f;
    Rush->Windup = 0.9f;
    Rush->Active = 0.8f;
    Rush->Recover = 0.9f;
    Rush->Damage = 24.f;
    Rush->Radius = 44.f;
    Rush->Speed = 680.f;
    Rush->Knockback = 750.f;
}

#else

internal void
DrawMonster_HollowKing(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Robe = Ramp(ART_RGB(26, 22, 30), ART_RGB(48, 40, 56),
                           ART_RGB(74, 64, 84), ART_RGB(104, 94, 116));
    color_ramp Plate = Ramp(ART_RGB(44, 44, 50), ART_RGB(82, 82, 92),
                            ART_RGB(126, 126, 138), ART_RGB(184, 186, 198));
    color_ramp Gold = Ramp(ART_RGB(110, 70, 20), ART_RGB(176, 124, 40),
                           ART_RGB(226, 180, 70), ART_RGB(255, 236, 150));
    color_ramp Blade = Ramp(ART_RGB(60, 62, 70), ART_RGB(110, 112, 122),
                            ART_RGB(170, 172, 182), ART_RGB(230, 232, 240));
    u32 Hollow = ART_RGB(8, 6, 12);
    u32 EyeColor = ART_RGB(170, 220, 255);
    u32 EyeHot = ART_RGB(240, 250, 255);

    float Bob = 0.f;
    float Lean = 0.f;
    float Stride = 0.f;
    float Hover = 0.f;
    // NOTE(zoubir): the greatsword runs from the hands toward its tip;
    // the angle is from straight up, positive tipping it forward
    v2 Grip = V2(40.f, 36.f);
    float SwordAngle = 0.5f;
    float Glare = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Stride = 3.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            SwordAngle = 0.7f + 0.1f * Pose.Wave;
            Hover = 0.5f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): the sword rises over the crown, the eyes flare
            Grip = Lerp2(Grip, Pose.t, V2(34.f, 30.f));
            SwordAngle = Lerp(0.5f, Pose.t, -0.4f);
            Lean = -2.f * Pose.t;
            Glare = Pose.t;
            Hover = -1.5f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Grip = Lerp2(V2(34.f, 30.f), Pose.t, V2(44.f, 40.f));
            SwordAngle = Lerp(-0.4f, Pose.t, 1.9f);
            Lean = 3.f;
            Glare = 1.f - 0.5f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            Lean = 2.f;
            Bob = 1.5f + 0.5f * Pose.Wave;
            SwordAngle = 1.4f;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Hover = 1.f * Pose.Wave;
            Glare = 0.2f + 0.2f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): plated boots under a long robe
    FillBlob(Canvas, 25.f - Stride, 56.f, 3.8f, 1.8f, Plate, -0.2f);
    FillBlob(Canvas, 36.f + Stride, 56.f, 3.8f, 1.8f, Plate);
    FillTriangle(Canvas, V2(30.f + Lean, 22.f + Bob), V2(13.f, 55.f),
                 V2(47.f, 55.f), Robe, 0.8f, 0.2f);
    // NOTE(zoubir): the robe's hem trails into dust
    for(u32 Wisp = 0; Wisp < 6; Wisp++)
    {
        float X = 14.f + 6.5f * Wisp;
        FillDot(Canvas, X + 0.5f * Stride, 55.5f + (float)(Wisp % 2), 1.4f, Robe.C[1]);
    }
    // NOTE(zoubir): a gold-trimmed sash down the front
    FillLimb(Canvas, V2(30.f + Lean, 34.f + Bob), V2(30.f, 54.f), 1.f, 1.6f, Gold, -0.3f);

    // NOTE(zoubir): breastplate and broad pauldrons over nothing
    float ChestX = 30.f + Lean;
    float ChestY = 30.f + Bob;
    FillBlob(Canvas, ChestX, ChestY, 9.f, 8.f, Plate, 0.1f);
    FillBlob(Canvas, ChestX - 10.f, ChestY - 4.f, 5.5f, 4.f, Plate, -0.1f);
    FillBlob(Canvas, ChestX + 10.f, ChestY - 4.f, 5.5f, 4.f, Plate, 0.15f);
    FillLimb(Canvas, V2(ChestX - 14.f, ChestY - 3.f), V2(ChestX - 6.f, ChestY - 6.f),
             0.6f, 0.6f, Gold);
    FillLimb(Canvas, V2(ChestX + 6.f, ChestY - 6.f), V2(ChestX + 14.f, ChestY - 3.f),
             0.6f, 0.6f, Gold);

    // NOTE(zoubir): the greatsword, held in both gauntlets
    v2 Up = V2(Sin(SwordAngle), -Cos(SwordAngle));
    v2 Tip = Grip + 26.f * Up;
    FillLimb(Canvas, Grip - 6.f * Up, Grip, 1.2f, 1.2f, Robe);
    FillLimb(Canvas, Grip, Tip, 2.4f, 0.8f, Blade, 0.1f);
    v2 Across = V2(-Up.Y, Up.X);
    FillLimb(Canvas, Grip - 4.f * Across, Grip + 4.f * Across, 1.f, 1.f, Gold);
    FillLimb(Canvas, V2(ChestX + 7.f, ChestY - 2.f), Grip, 2.f, 1.6f, Plate);
    FillLimb(Canvas, V2(ChestX - 7.f, ChestY - 2.f), Grip - 2.f * Up, 2.f, 1.6f, Plate, -0.1f);
    FillBlob(Canvas, Grip.X, Grip.Y, 2.4f, 2.2f, Plate, 0.1f);

    // NOTE(zoubir): the empty hood, two eyes in the dark, the crown
    // floating over where a head should be
    float HoodX = ChestX + 1.f;
    float HoodY = ChestY - 12.f + Hover;
    FillBlob(Canvas, HoodX, HoodY, 7.f, 7.5f, Robe, 0.1f);
    FillBlob(Canvas, HoodX + 1.5f, HoodY + 1.f, 4.5f, 5.f,
             Ramp(Hollow, Hollow, Hollow, Hollow));
    u32 Eye = Glare > 0.5f ? EyeHot : EyeColor;
    FillDot(Canvas, HoodX + 0.5f, HoodY, 1.f + 0.5f * Glare, Eye);
    FillDot(Canvas, HoodX + 3.5f, HoodY, 0.9f + 0.5f * Glare, Eye);
    float CrownY = HoodY - 9.f + 0.5f * Hover;
    FillLimb(Canvas, V2(HoodX - 5.f, CrownY + 2.f), V2(HoodX + 6.f, CrownY + 2.f),
             1.4f, 1.4f, Gold, 0.1f);
    for(u32 Point = 0; Point < 4; Point++)
    {
        float X = HoodX - 4.5f + 3.4f * Point;
        FillTriangle(Canvas, V2(X - 1.4f, CrownY + 1.f), V2(X + 1.4f, CrownY + 1.f),
                     V2(X, CrownY - 3.f), Gold, 0.7f, 0.2f);
    }
    FillDot(Canvas, HoodX + 0.5f, CrownY + 2.f, 0.8f, ART_RGB(200, 40, 60));

    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): a pale arc behind the falling blade
        for(u32 Trail = 1; Trail <= 4; Trail++)
        {
            float Back = SwordAngle - 0.3f * Trail;
            v2 Point = Grip + 28.f * V2(Sin(Back), -Cos(Back));
            FillDot(Canvas, Point.X, Point.Y, 1.8f - 0.3f * Trail, EyeColor);
        }
    }

    OutlineFrame(Canvas, ART_RGB(8, 6, 10));
}

#endif
