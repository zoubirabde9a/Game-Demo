/* Starfall Acolyte: a tall robed cultist of the dead star in the Starless
   Deep (docs/dungeon-starless.md), faceless under a gold-lined hood, a
   shard of the star cupped at its chest. It keeps its distance and calls
   the star down in rows (MonsterAbility_Lanes): three strips laid along
   its aim and centred on its target, things falling on them from above,
   so a jump does not help. Stand in the gap between two strips. When a
   player closes in it steps away through the dark and lands beside them
   with a burst. Only the deep's encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Acolyte)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Acolyte(monster_def *Def)
{
    Def->Name = "Starfall Acolyte";
    Def->MaxHp = 95.f;
    Def->Acceleration = 28000.f;
    Def->AggroRange = 440.f;
    Def->StopRange = 180.f;
    Def->AttackRange = 44.f;
    Def->AttackDamage = 8.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 56;

    monster_ability *Step = AddMonsterAbility(Def, MonsterAbility_Blink,
                                              "Falling Step");
    Step->MaxRange = 160.f;
    Step->Cooldown = 8.f;
    Step->Windup = 0.7f;
    Step->Active = 0.3f;
    Step->Recover = 0.7f;
    Step->Damage = 11.f;
    Step->Radius = 80.f;
    Step->Spread = 70.f;
    Step->Knockback = 300.f;

    // NOTE(zoubir): three rows 100 apart, the middle one on the target
    monster_ability *Rows = AddMonsterAbility(Def, MonsterAbility_Lanes,
                                              "Starfall Rows");
    Rows->MaxRange = 400.f;
    Rows->Cooldown = 7.f;
    Rows->Windup = 1.1f;
    Rows->Active = 0.4f;
    Rows->Recover = 0.6f;
    Rows->Damage = 10.f;
    Rows->Radius = 22.f;
    Rows->Speed = 340.f;
    Rows->Spread = 100.f;
    Rows->Count = 3;
    Rows->Status = StatusEffect_Burning;
    Rows->StatusSeconds = 2.f;
}

#else

internal void
DrawAcolyteShard(sprite_canvas *Canvas, v2 P, float Size, float Glow,
                 color_ramp Gold)
{
    if (Glow > 0.f)
    {
        FillDot(Canvas, P.X, P.Y, 0.5f * Size + 2.f * Glow, ART_RGB(96, 52, 140));
        // NOTE(zoubir): motes of starlight drawn in round it
        for(u32 Mote = 0; Mote < 4; Mote++)
        {
            float Angle = 0.5f * Pi32 * (float)Mote + 0.8f * Glow;
            float Reach = Size + 1.f + 2.f * (1.f - Glow);
            FillDot(Canvas, P.X + Reach * Cos(Angle), P.Y + Reach * Sin(Angle),
                    0.5f + 0.4f * Glow, ART_RGB(255, 230, 140));
        }
    }
    float W = 0.6f * Size;
    FillTriangle(Canvas, V2(P.X, P.Y - Size), V2(P.X - W, P.Y), V2(P.X + W, P.Y),
                 Gold, 1.f, 0.5f);
    FillTriangle(Canvas, V2(P.X, P.Y + Size), V2(P.X - W, P.Y), V2(P.X + W, P.Y),
                 Gold, 0.2f, 0.6f);
    FillDot(Canvas, P.X, P.Y - 0.3f, 0.4f + 0.35f * Size * (0.4f + 0.6f * Glow),
            ART_RGB(255, 250, 225));
}

internal void
DrawMonster_Acolyte(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Robe = Ramp(ART_RGB(18, 10, 30), ART_RGB(44, 24, 72),
                           ART_RGB(78, 44, 118), ART_RGB(118, 76, 168));
    color_ramp Black = Ramp(ART_RGB(8, 6, 12), ART_RGB(20, 16, 28),
                            ART_RGB(36, 30, 48), ART_RGB(58, 50, 74));
    color_ramp Gold = Ramp(ART_RGB(110, 66, 12), ART_RGB(180, 128, 34),
                           ART_RGB(232, 192, 74), ART_RGB(255, 242, 176));

    // NOTE(zoubir): where the near hand is for each pose, the shard rides it
    v2 HandRest = V2(36.f, 27.f);
    v2 HandHigh = V2(35.f, 13.5f);
    v2 HandThrust = V2(44.f, 31.f);

    float Bob = 0.f;
    float Sway = 0.f;
    float Lean = 0.f;
    v2 Hand = HandRest;
    float ShardSize = 2.6f;
    float Glow = 0.15f + 0.1f * Pose.Wave2;
    float Flare = 0.f;
    float Feet = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Bob = -0.8f * Absolute(Pose.Wave);
            Sway = -2.5f * Pose.Wave2;
            Lean = 1.5f;
            Feet = Pose.Wave;
            Hand = HandRest + V2(0.f, 0.5f * Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Hand = Lerp2(HandRest, Pose.t, HandHigh);
            ShardSize = 2.6f + 2.4f * Pose.t;
            Glow = 0.3f + 0.7f * Pose.t;
            Bob = -1.f * Pose.t;
            Lean = -1.f * Pose.t;
            Sway = 1.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            float Snap = Minimum(1.f, 3.f * Pose.t);
            Hand = Lerp2(HandHigh, Snap, HandThrust);
            ShardSize = 5.f - 1.f * Pose.t;
            Glow = 1.f;
            Flare = 1.f - 0.6f * Pose.t;
            Lean = 2.5f * Snap;
            Sway = 1.f - 3.f * Snap;
        } break;

        case AnimationType_Stop:
        {
            Hand = Lerp2(HandThrust, Pose.t, HandRest);
            ShardSize = 3.5f - 0.9f * Pose.t;
            Glow = 0.4f * (1.f - Pose.t);
            Lean = 2.5f * (1.f - Pose.t);
            Sway = -2.f + 2.f * Pose.t;
            Bob = 1.f - Pose.t;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Sway = 1.2f * Pose.Wave2;
        } break;
    }

    float Ground = 49.f;
    float NeckY = 19.f + Bob;
    float BodyX = 27.f + Lean;

    // NOTE(zoubir): toes of black slippers peek from under the hem in step
    if (Pose.Anim == AnimationType_Move)
    {
        FillBlob(Canvas, 26.f + 4.f * Feet, Ground - 1.f, 3.f, 1.6f, Black, 0.f);
    }

    // NOTE(zoubir): the far sleeve, behind the robe
    v2 FarShoulder = V2(BodyX - 2.f, NeckY + 3.f);
    v2 FarHand = Hand + V2(-3.f, 1.5f);
    FillLimb(Canvas, FarShoulder, FarHand, 2.5f, 3.2f, Robe, -0.5f);
    FillBlob(Canvas, FarHand.X + 0.5f, FarHand.Y, 1.6f, 1.6f, Black, -0.2f);

    // NOTE(zoubir): the robe falls from the shoulders to a wide hem that
    // swings behind the walk
    float HemL = 15.f + Sway;
    float HemR = 40.f + Sway + 0.5f * Lean;
    FillTriangle(Canvas, V2(BodyX + 0.5f, NeckY), V2(HemL, Ground),
                 V2(HemR, Ground), Robe, 0.55f, 0.35f);
    FillLimb(Canvas, V2(BodyX, NeckY + 2.f), V2(27.f + 0.5f * Sway + 0.5f * Lean, Ground - 9.f),
             5.5f, 8.5f, Robe, 0.f);
    // NOTE(zoubir): the black underrobe shows down the front opening
    FillTriangle(Canvas, V2(BodyX + 4.f, NeckY + 6.f), V2(HemR - 7.f, Ground),
                 V2(HemR - 2.f, Ground), Black, 0.5f, 0.3f);
    // NOTE(zoubir): gold trim along the hem and down the opening
    FillLimb(Canvas, V2(HemL + 0.5f, Ground - 0.8f), V2(HemR - 0.5f, Ground - 0.8f),
             0.9f, 0.9f, Gold, 0.2f);
    FillLimb(Canvas, V2(BodyX + 4.f, NeckY + 6.f), V2(HemR - 7.f, Ground - 1.f),
             0.7f, 0.8f, Gold, 0.3f);
    // NOTE(zoubir): a few dead stars stitched into the cloth
    FillDot(Canvas, BodyX - 4.f, NeckY + 14.f, 0.7f, ART_RGB(232, 192, 74));
    FillDot(Canvas, 22.f + 0.7f * Sway, Ground - 6.f, 0.7f, ART_RGB(232, 192, 74));
    FillDot(Canvas, BodyX - 1.f, NeckY + 22.f, 0.6f, ART_RGB(180, 128, 34));

    // NOTE(zoubir): the pointed hood, its tip bent back
    float HeadX = BodyX + 3.f;
    float HeadY = NeckY - 5.f;
    FillTriangle(Canvas, V2(HeadX - 9.f, HeadY - 12.f + 0.5f * Sway), V2(HeadX - 5.f, HeadY + 3.f),
                 V2(HeadX + 2.f, HeadY - 4.f), Robe, 0.9f, 0.4f);
    FillBlob(Canvas, HeadX, HeadY, 6.f, 6.5f, Robe, 0.2f);
    // NOTE(zoubir): a gold-lined opening with nothing in it but two glints
    FillBlob(Canvas, HeadX + 2.5f, HeadY + 0.8f, 3.8f, 4.6f, Gold, 0.3f);
    FillDot(Canvas, HeadX + 3.f, HeadY + 1.f, 3.f, ART_RGB(6, 4, 10));
    FillDot(Canvas, HeadX + 3.6f, HeadY, 3.f, ART_RGB(6, 4, 10));
    u32 Glint = (Glow > 0.6f) ? ART_RGB(255, 240, 170) : ART_RGB(214, 170, 60);
    FillDot(Canvas, HeadX + 2.6f, HeadY + 0.5f, 0.6f, Glint);
    FillDot(Canvas, HeadX + 4.8f, HeadY + 0.5f, 0.6f, Glint);

    // NOTE(zoubir): the star shard, it swells over the head before the rows fall
    v2 ShardP = Hand + V2(0.5f, -1.5f - 0.8f * ShardSize);
    DrawAcolyteShard(Canvas, ShardP, ShardSize, Glow, Gold);
    if (Flare > 0.f)
    {
        float Ray = ShardSize + 2.f + 5.f * Flare;
        FillTriangle(Canvas, V2(ShardP.X + Ray + 2.f, ShardP.Y), V2(ShardP.X + 1.f, ShardP.Y - 1.f),
                     V2(ShardP.X + 1.f, ShardP.Y + 1.f), Gold, 1.f, 0.7f);
        FillTriangle(Canvas, V2(ShardP.X, ShardP.Y - Ray), V2(ShardP.X - 1.f, ShardP.Y - 1.f),
                     V2(ShardP.X + 1.f, ShardP.Y - 1.f), Gold, 1.f, 0.7f);
        FillTriangle(Canvas, V2(ShardP.X, ShardP.Y + Ray), V2(ShardP.X - 1.f, ShardP.Y + 1.f),
                     V2(ShardP.X + 1.f, ShardP.Y + 1.f), Gold, 1.f, 0.7f);
        FillTriangle(Canvas, V2(ShardP.X + 0.7f * Ray, ShardP.Y + 0.7f * Ray),
                     V2(ShardP.X + 1.f, ShardP.Y), V2(ShardP.X, ShardP.Y + 1.f), Gold, 1.f, 0.7f);
        FillTriangle(Canvas, V2(ShardP.X + 0.7f * Ray, ShardP.Y - 0.7f * Ray),
                     V2(ShardP.X + 1.f, ShardP.Y), V2(ShardP.X, ShardP.Y - 1.f), Gold, 1.f, 0.7f);
    }

    // NOTE(zoubir): the near sleeve, a bell of violet with a gold cuff
    v2 Shoulder = V2(BodyX + 2.f, NeckY + 3.f);
    FillLimb(Canvas, Shoulder, Hand, 2.8f, 3.6f, Robe, 0.2f);
    v2 Cuff = Shoulder + 0.9f * (Hand - Shoulder);
    FillLimb(Canvas, Cuff, Hand, 3.3f, 3.6f, Gold, 0.f);
    FillBlob(Canvas, Hand.X + 1.f, Hand.Y - 0.5f, 1.8f, 1.8f, Black, 0.1f);

    OutlineFrame(Canvas, ART_RGB(6, 4, 10));
}

#endif
