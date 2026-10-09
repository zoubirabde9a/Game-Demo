/* Rimecrown Stag: a great elk of blue ice from the Aurora Rift
   (docs/dungeon-rift.md), its antlers full of the aurora. Aurora Bellow
   breathes a wide cone of frost along the aim it locks as the windup
   starts (MonsterAbility_Cone): it hits through a jump and slows, so the
   answer is to get beside or behind it while the antlers light up. From
   farther off it lowers its rack and rushes its target in a straight line.
   Only the rift's encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Stag)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Stag(monster_def *Def)
{
    Def->Name = "Rimecrown Stag";
    Def->MaxHp = 140.f;
    Def->Acceleration = 32000.f;
    Def->AggroRange = 400.f;
    Def->StopRange = 40.f;
    Def->AttackRange = 54.f;
    Def->AttackDamage = 11.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 64;

    // NOTE(zoubir): a 70 degree breath out to 230; stepping aside clears it
    monster_ability *Bellow = AddMonsterAbility(Def, MonsterAbility_Cone,
                                                "Aurora Bellow");
    Bellow->MaxRange = 200.f;
    Bellow->Cooldown = 6.5f;
    Bellow->Windup = 0.9f;
    Bellow->Active = 0.4f;
    Bellow->Recover = 0.7f;
    Bellow->Damage = 14.f;
    Bellow->Radius = 230.f;
    Bellow->Spread = 70.f;
    Bellow->Knockback = 300.f;
    Bellow->Status = StatusEffect_Slowed;
    Bellow->StatusSeconds = 1.5f;

    monster_ability *Rush = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Antler Rush");
    Rush->MinRange = 180.f;
    Rush->MaxRange = 420.f;
    Rush->Cooldown = 7.f;
    Rush->Windup = 0.7f;
    Rush->Active = 0.6f;
    Rush->Recover = 0.9f;
    Rush->Damage = 14.f;
    Rush->Radius = 38.f;
    Rush->Speed = 640.f;
    Rush->Knockback = 480.f;
}

#else

// NOTE(zoubir): one antler from its root on the skull, sweeping up and
// back, with three tines forward and the aurora caught between them
internal void
DrawRimecrownAntler(sprite_canvas *Canvas, v2 Root, color_ramp Bone, float LightBias,
                    float Glow, u32 Frame, u32 Green, u32 Violet)
{
    v2 Beam1 = Root + V2(-2.f, -5.f);
    v2 Beam2 = Root + V2(-6.f, -9.f);
    v2 Beam3 = Root + V2(-12.f, -11.f);
    v2 Tip1 = Beam1 + V2(4.f, -4.f);
    v2 Tip2 = Beam2 + V2(2.f, -6.f);
    v2 Tip3 = Beam3 + V2(-2.f, -4.f);
    v2 Brow = Root + V2(5.f, -2.f);

    if (Glow > 0.f)
    {
        // NOTE(zoubir): the light sits in the gaps between the tines and
        // flickers between green and violet
        v2 Gaps[3] = {(Tip1 + Beam1) * 0.5f + V2(1.f, -1.f),
                      (Tip1 + Tip2) * 0.5f + V2(0.f, 1.f),
                      (Tip2 + Tip3) * 0.5f + V2(0.f, 1.5f)};
        for(u32 Gap = 0; Gap < 3; Gap++)
        {
            u32 Color = ((Gap + Frame) % 2) ? Violet : Green;
            float Size = Glow * (1.8f + 0.5f * (float)((Gap + Frame) % 3));
            FillDot(Canvas, Gaps[Gap].X, Gaps[Gap].Y, Size, Color);
            FillDot(Canvas, Gaps[Gap].X, Gaps[Gap].Y, 0.45f * Size, ART_RGB(236, 255, 244));
        }
    }

    FillLimb(Canvas, Root, Beam1, 1.8f, 1.5f, Bone, LightBias);
    FillLimb(Canvas, Beam1, Beam2, 1.5f, 1.3f, Bone, LightBias);
    FillLimb(Canvas, Beam2, Beam3, 1.3f, 1.f, Bone, LightBias);
    FillLimb(Canvas, Beam1, Tip1, 1.2f, 0.6f, Bone, LightBias + 0.2f);
    FillLimb(Canvas, Beam2, Tip2, 1.2f, 0.6f, Bone, LightBias + 0.2f);
    FillLimb(Canvas, Beam3, Tip3, 1.f, 0.5f, Bone, LightBias + 0.2f);
    FillLimb(Canvas, Root + V2(0.f, -1.f), Brow, 1.1f, 0.6f, Bone, LightBias + 0.1f);

    if (Glow > 0.5f)
    {
        // NOTE(zoubir): bright sparks at the tips once the lungs are full
        FillDot(Canvas, Tip1.X, Tip1.Y, 0.8f, Green);
        FillDot(Canvas, Tip2.X, Tip2.Y, 0.8f, Violet);
        FillDot(Canvas, Tip3.X, Tip3.Y, 0.8f, Green);
    }
}

internal void
DrawMonster_Stag(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Hide = Ramp(ART_RGB(34, 52, 96), ART_RGB(62, 98, 154),
                           ART_RGB(104, 152, 204), ART_RGB(166, 206, 238));
    color_ramp Frost = Ramp(ART_RGB(120, 150, 188), ART_RGB(184, 208, 232),
                            ART_RGB(226, 238, 250), ART_RGB(250, 253, 255));
    color_ramp Bone = Ramp(ART_RGB(70, 110, 150), ART_RGB(130, 190, 220),
                           ART_RGB(190, 236, 248), ART_RGB(240, 255, 255));
    color_ramp Hoof = Ramp(ART_RGB(24, 32, 56), ART_RGB(40, 52, 84),
                           ART_RGB(62, 78, 116), ART_RGB(90, 108, 148));
    u32 Green = ART_RGB(110, 250, 170);
    u32 Violet = ART_RGB(190, 120, 255);
    u32 Mist = ART_RGB(220, 238, 252);
    u32 MistDim = ART_RGB(160, 196, 230);

    float Bob = 0.f;
    // NOTE(zoubir): leg swing for the diagonal pairs of a trot
    float SwingA = 0.f;
    float SwingB = 0.f;
    // NOTE(zoubir): head forward along the neck, and down (negative is up)
    float Thrust = 0.f;
    float HeadDrop = 0.f;
    float Glow = 0.f;
    float Chest = 0.f;
    float Mouth = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            // NOTE(zoubir): a proud trot, head held high, knees lifted
            SwingA = 4.f * Pose.Wave;
            SwingB = -4.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            HeadDrop = -1.f + 0.5f * Absolute(Pose.Wave2);
            Glow = 0.4f;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): rears the head back and fills its lungs while the
            // aurora floods the antlers
            Thrust = -2.f * Pose.t;
            HeadDrop = -0.5f * Pose.t;
            Bob = -1.f * Pose.t;
            Chest = 2.f * Pose.t;
            Glow = 0.6f + 0.8f * Pose.t;
            SwingA = -1.5f * Pose.t;
            SwingB = 1.5f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            // NOTE(zoubir): the head snaps forward, the mouth gapes and frost
            // pours out
            float Snap = Minimum(1.f, 3.f * Pose.t);
            Thrust = -2.f + 6.f * Snap;
            HeadDrop = -0.5f + 4.5f * Snap;
            Chest = 2.f - 2.f * Pose.t;
            Glow = 1.4f - 0.8f * Pose.t;
            Mouth = Snap;
            SwingA = -2.f;
            SwingB = 2.f;
            Bob = 1.f * Snap;
        } break;

        case AnimationType_Stop:
        {
            // NOTE(zoubir): head hung low, flanks heaving, panting mist
            Thrust = 2.f;
            HeadDrop = 9.f;
            Bob = 1.f + 0.8f * Pose.Wave;
            Chest = 0.5f + 0.8f * Pose.Wave;
            Mouth = 0.4f + 0.3f * Pose.Wave;
        } break;

        default:
        {
            Bob = 0.5f * Pose.Wave;
            HeadDrop = 0.5f * Pose.Wave2;
            Glow = 0.3f + 0.15f * Pose.Wave;
        } break;
    }

    float Ground = 56.f;
    float BackY = 33.f + Bob;
    float HipX = 17.f;
    float ShoulderX = 36.f;

    // NOTE(zoubir): a puff of tail, then the far legs in a darker shade
    FillBlob(Canvas, HipX - 6.f, BackY - 4.f, 2.5f, 3.f, Frost, 0.2f);
    FillLimb(Canvas, V2(HipX, BackY + 4.f), V2(HipX - 2.f + 0.5f * SwingA, BackY + 13.f), 3.5f, 2.f, Hide, -0.5f);
    FillLimb(Canvas, V2(HipX - 2.f + 0.5f * SwingA, BackY + 13.f), V2(HipX + SwingA, Ground - 1.f), 1.6f, 1.3f, Hide, -0.5f);
    FillBlob(Canvas, HipX + SwingA, Ground - 0.5f, 2.f, 1.3f, Hoof, -0.3f);
    FillLimb(Canvas, V2(ShoulderX + 1.f, BackY + 5.f), V2(ShoulderX + 2.f + 0.5f * SwingB, BackY + 14.f), 3.f, 1.8f, Hide, -0.5f);
    FillLimb(Canvas, V2(ShoulderX + 2.f + 0.5f * SwingB, BackY + 14.f), V2(ShoulderX + 1.f + SwingB, Ground - 1.f), 1.6f, 1.3f, Hide, -0.5f);
    FillBlob(Canvas, ShoulderX + 1.f + SwingB, Ground - 0.5f, 2.f, 1.3f, Hoof, -0.3f);

    // NOTE(zoubir): a long deep body, the chest swelling as it breathes in
    FillBlob(Canvas, 25.f, BackY + 0.5f, 14.f, 7.5f, Hide, 0.f);
    FillBlob(Canvas, ShoulderX, BackY - 0.5f + 0.25f * Chest, 7.5f + 0.5f * Chest, 7.5f + 0.5f * Chest, Hide, 0.1f);
    // NOTE(zoubir): rime crusted along the spine and icicles off the belly
    for(u32 Crust = 0; Crust < 5; Crust++)
    {
        float X = 14.f + 5.f * Crust;
        float Y = BackY - 6.5f - (Crust == 4 ? 1.f : 0.f);
        FillTriangle(Canvas, V2(X - 2.f, Y + 2.f), V2(X + 2.f, Y + 2.f),
                     V2(X + 0.5f, Y - 1.5f - (float)(Crust % 2)), Bone, 0.4f, 1.f);
    }
    for(u32 Icicle = 0; Icicle < 4; Icicle++)
    {
        float X = 20.f + 4.f * Icicle;
        float Y = BackY + 7.f;
        FillTriangle(Canvas, V2(X - 1.3f, Y), V2(X + 1.3f, Y),
                     V2(X, Y + 3.f + (float)(Icicle % 2)), Bone, 0.4f, 1.f);
    }

    // NOTE(zoubir): the neck rises from the shoulders, a white frost ruff
    // hanging at its throat
    v2 NeckBase = V2(ShoulderX + 3.f, BackY - 3.f);
    v2 HeadPos = V2(44.f + Thrust, 24.f + Bob + HeadDrop);
    FillLimb(Canvas, NeckBase, HeadPos + V2(-1.f, 2.f), 5.5f, 3.5f, Hide, 0.15f);
    v2 Ruff = (NeckBase + HeadPos) * 0.5f + V2(2.f, 3.f);
    FillBlob(Canvas, Ruff.X, Ruff.Y, 3.5f, 4.5f, Frost, 0.2f);
    FillTriangle(Canvas, V2(Ruff.X - 2.f, Ruff.Y + 3.f), V2(Ruff.X + 2.f, Ruff.Y + 3.f),
                 V2(Ruff.X, Ruff.Y + 7.f), Frost, 0.3f, 0.9f);

    // NOTE(zoubir): the far antler sits behind the head, a step back
    DrawRimecrownAntler(Canvas, HeadPos + V2(-3.f, -3.f), Bone, -0.45f,
                        Glow, Pose.Frame + 1, Green, Violet);

    // NOTE(zoubir): a long head, the muzzle tipped down, jaw dropping open
    float MuzzleDown = 0.5f * Mouth;
    FillBlob(Canvas, HeadPos.X, HeadPos.Y, 4.5f, 4.f, Hide, 0.25f);
    FillLimb(Canvas, HeadPos + V2(1.f, 0.5f), HeadPos + V2(7.f, 3.f + MuzzleDown), 3.5f, 2.5f, Hide, 0.3f);
    if (Mouth > 0.f)
    {
        FillLimb(Canvas, HeadPos + V2(2.f, 4.f), HeadPos + V2(6.f, 5.f + 1.5f * Mouth), 1.5f, 1.2f, Hide, -0.1f);
        FillDot(Canvas, HeadPos.X + 7.f, HeadPos.Y + 4.5f + 1.2f * Mouth, 0.6f + 0.9f * Mouth, ART_RGB(24, 30, 60));
    }
    FillDot(Canvas, HeadPos.X + 9.f, HeadPos.Y + 3.f + MuzzleDown, 0.9f, ART_RGB(28, 40, 72));
    // NOTE(zoubir): the eye burns with the aurora as it fills
    u32 EyeColor = (Glow > 0.9f) ? Green : ART_RGB(230, 250, 255);
    FillDot(Canvas, HeadPos.X + 1.5f, HeadPos.Y - 0.5f, 1.f, EyeColor);
    // NOTE(zoubir): the ear, laid back
    FillTriangle(Canvas, HeadPos + V2(-3.f, -2.f), HeadPos + V2(-1.f, -3.f),
                 HeadPos + V2(-7.f, -4.f), Hide, 0.5f, 0.1f);

    DrawRimecrownAntler(Canvas, HeadPos + V2(0.f, -3.f), Bone, 0.1f,
                        Glow, Pose.Frame, Green, Violet);

    // NOTE(zoubir): the near legs, a thin cannon bone under a thick forearm
    FillLimb(Canvas, V2(HipX + 2.f, BackY + 4.f), V2(HipX + 0.5f * SwingB, BackY + 13.f), 4.f, 2.2f, Hide, 0.f);
    FillLimb(Canvas, V2(HipX + 0.5f * SwingB, BackY + 13.f), V2(HipX + 2.f + SwingB, Ground - 1.f), 1.8f, 1.4f, Hide, 0.f);
    FillBlob(Canvas, HipX + 2.f + SwingB, Ground - 0.5f, 2.2f, 1.4f, Hoof, 0.f);
    FillLimb(Canvas, V2(ShoulderX + 3.f, BackY + 5.f), V2(ShoulderX + 4.f + 0.5f * SwingA, BackY + 14.f), 3.5f, 2.f, Hide, 0.1f);
    FillLimb(Canvas, V2(ShoulderX + 4.f + 0.5f * SwingA, BackY + 14.f), V2(ShoulderX + 4.f + SwingA, Ground - 1.f), 1.8f, 1.4f, Hide, 0.1f);
    FillBlob(Canvas, ShoulderX + 4.f + SwingA, Ground - 0.5f, 2.2f, 1.4f, Hoof, 0.f);

    if (Pose.Anim == AnimationType_Cast)
    {
        // NOTE(zoubir): a ribbon of aurora gathers over the rack
        float Strength = Pose.t;
        for(u32 Mote = 0; Mote < 7; Mote++)
        {
            float X = HeadPos.X - 15.f + 3.f * Mote;
            float Y = HeadPos.Y - 9.f + 1.5f * Sin(2.f * Pi32 * (Pose.t + 0.15f * Mote));
            u32 Color = (Mote < 3) ? Green : ((Mote < 5) ? MistDim : Violet);
            if ((float)Mote < 1.f + 7.f * Strength)
            {
                FillDot(Canvas, X, Maximum(2.f, Y), 0.7f + 0.5f * Strength, Color);
            }
        }
    }
    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): a spreading puff of frost breath, flecked with light
        v2 Mouth0 = HeadPos + V2(9.f, 5.f);
        float Puff = Minimum(1.f, 2.f * Pose.t);
        float Far = Minimum(62.f, Mouth0.X + 3.f + 4.f * Puff);
        float Half = 1.5f + 5.f * Puff;
        FillTriangle(Canvas, Mouth0 + V2(-1.f, 0.f), V2(Far, Mouth0.Y - Half),
                     V2(Far, Mouth0.Y + Half), Frost, 0.2f, 1.f);
        for(u32 Cloud = 0; Cloud < 3; Cloud++)
        {
            float Y = Mouth0.Y + Half * (0.7f * (float)Cloud - 0.7f);
            FillDot(Canvas, Far - 1.f, Y, 1.f + 0.4f * Puff, (Cloud % 2) ? MistDim : Mist);
        }
        FillDot(Canvas, Minimum(Mouth0.X + 5.f * Puff + 2.f, 60.f), Mouth0.Y + 2.f, 0.7f,
                (Pose.Frame % 2) ? Green : Violet);
    }
    if (Pose.Anim == AnimationType_Stop)
    {
        // NOTE(zoubir): panting mist drifting up off the muzzle
        float Drift = Pose.t;
        v2 Nose = HeadPos + V2(10.f, 4.f);
        FillDot(Canvas, Minimum(Nose.X + 1.f + 2.f * Drift, 61.f), Nose.Y - 2.f * Drift, 1.f + 1.f * Drift, Mist);
        FillDot(Canvas, Minimum(Nose.X + 3.f - 1.f * Drift, 61.f), Nose.Y - 4.f - 2.f * Drift, 0.8f + 0.6f * (1.f - Drift), MistDim);
    }

    OutlineFrame(Canvas, ART_RGB(14, 20, 40));
}

#endif
