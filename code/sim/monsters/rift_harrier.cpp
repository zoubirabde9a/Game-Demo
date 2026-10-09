/* Shardwing Harrier: a frost hawk the size of a dog that hunts over the
   Aurora Rift (docs/dungeon-rift.md), every feather a blade of ice, pale
   blue tipped with aurora green. Icicle Rake sweeps its wings up and
   drops three strips of icicles along its aim, centred on its target
   (MonsterAbility_Lanes): they fall from above, so a jump does not help,
   and the way out is the gap between two strips. From range it flicks a
   Quill Flurry, three ice spines in a narrow fan, that a sidestep clears.
   Fragile, so close in and bring it down between flurries. Only the
   rift's encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Harrier)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Harrier(monster_def *Def)
{
    Def->Name = "Shardwing Harrier";
    Def->MaxHp = 80.f;
    Def->Acceleration = 36000.f;
    Def->AggroRange = 440.f;
    Def->StopRange = 160.f;
    Def->AttackRange = 40.f;
    Def->AttackDamage = 6.f;
    Def->AttackInterval = 0.8f;
    Def->FlyHeight = 24.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 56;

    // NOTE(zoubir): three strips 80 apart, so the gaps are wide enough to
    // stand in once the player reads them
    monster_ability *Rake = AddMonsterAbility(Def, MonsterAbility_Lanes,
                                              "Icicle Rake");
    Rake->MaxRange = 360.f;
    Rake->Cooldown = 6.5f;
    Rake->Windup = 1.f;
    Rake->Active = 0.4f;
    Rake->Recover = 0.6f;
    Rake->Damage = 10.f;
    Rake->Radius = 20.f;
    Rake->Speed = 300.f;
    Rake->Spread = 80.f;
    Rake->Count = 3;

    monster_ability *Flurry = AddMonsterAbility(Def, MonsterAbility_Volley,
                                                "Quill Flurry");
    Flurry->MinRange = 100.f;
    Flurry->MaxRange = 380.f;
    Flurry->Cooldown = 4.f;
    Flurry->Windup = 0.6f;
    Flurry->Active = 1.f;
    Flurry->Recover = 0.5f;
    Flurry->Damage = 8.f;
    Flurry->Radius = 14.f;
    Flurry->Speed = 420.f;
    Flurry->Spread = 30.f;
    Flurry->Count = 3;
    Flurry->ShotStyle = ShotStyle_Spine;
}

#else

// NOTE(zoubir): a wing of ice blades fanned from the shoulder. Lift 1 is
// swept straight up, 0 spread back in a glide, -1 driven down. Tip is how
// far the green tips run out past each blade
internal void
DrawShardWing(sprite_canvas *Canvas, v2 Shoulder, float Lift, float Span,
              float Tip, color_ramp Blade, color_ramp Aurora)
{
    float Lead = (-165.f + 95.f * Lift) * Pi32 / 180.f;
    u32 FeatherCount = 5;
    for(u32 Feather = 0; Feather < FeatherCount; Feather++)
    {
        // NOTE(zoubir): the leading blade is the longest, the rest fan back
        float Angle = Lead - 0.3f * (float)Feather;
        float Length = Span * (1.f - 0.11f * (float)Feather);
        v2 Dir = V2(Cos(Angle), Sin(Angle));
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 Root = Shoulder + 1.5f * (float)Feather * Dir;
        v2 End = Shoulder + Length * Dir;
        FillTriangle(Canvas, Root - 2.6f * Side, Root + 2.6f * Side, End, Blade, 0.1f, 0.9f);
        v2 TipBase = Shoulder + (Length - 4.f) * Dir;
        FillTriangle(Canvas, TipBase - 1.4f * Side, TipBase + 1.4f * Side,
                     End + (1.f + Tip) * Dir, Aurora, 0.4f, 1.f);
    }
    // NOTE(zoubir): the bony arm along the leading edge
    v2 Wrist = Shoulder + 0.45f * Span * V2(Cos(Lead), Sin(Lead));
    FillLimb(Canvas, Shoulder, Wrist, 1.6f, 0.9f, Blade, 0.5f);
}

internal void
DrawMonster_Harrier(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Blade = Ramp(ART_RGB(36, 58, 108), ART_RGB(70, 116, 182),
                            ART_RGB(130, 182, 230), ART_RGB(214, 238, 255));
    color_ramp FarBlade = Ramp(ART_RGB(24, 36, 74), ART_RGB(44, 72, 126),
                               ART_RGB(78, 116, 170), ART_RGB(120, 160, 208));
    color_ramp Breast = Ramp(ART_RGB(120, 160, 210), ART_RGB(176, 210, 240),
                             ART_RGB(220, 238, 252), ART_RGB(248, 252, 255));
    color_ramp Aurora = Ramp(ART_RGB(20, 100, 86), ART_RGB(46, 184, 140),
                             ART_RGB(126, 244, 190), ART_RGB(220, 255, 236));
    color_ramp Horn = Ramp(ART_RGB(44, 36, 76), ART_RGB(86, 70, 136),
                           ART_RGB(146, 124, 196), ART_RGB(214, 200, 244));
    u32 Eye = ART_RGB(150, 255, 200);
    u32 EyeHot = ART_RGB(250, 255, 255);
    u32 Glint = ART_RGB(250, 254, 255);

    float Lift = 0.3f + 0.6f * Pose.Wave;
    float Bob = -1.5f * Pose.Wave;
    float Lunge = 0.f;
    float Reach = 0.f;
    float Tip = 0.f;
    float Glow = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            // NOTE(zoubir): full beats, leaning into the flight
            Lift = 0.35f + 0.75f * Pose.Wave;
            Bob = -2.f * Pose.Wave;
            Lunge = 1.5f;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): wings swept high, icicles growing on every tip
            Lift = 0.6f + 0.45f * Minimum(1.f, 2.f * Pose.t);
            Bob = -1.5f * Pose.t;
            Lunge = -1.5f * Pose.t;
            Tip = 3.f * Pose.t;
            Glow = Pose.t;
        } break;

        case AnimationType_Attack:
        {
            // NOTE(zoubir): one hard downstroke, talons thrown forward
            float Drive = Minimum(1.f, 2.5f * Pose.t);
            Lift = 1.05f - 2.05f * Drive;
            Bob = -3.f + 6.f * Drive;
            Lunge = 4.f * Drive;
            Reach = Drive;
            Glow = 1.f;
        } break;

        case AnimationType_Stop:
        {
            // NOTE(zoubir): wings held out flat, drifting
            Lift = 0.f + 0.08f * Pose.Wave;
            Bob = 2.f + 0.8f * Pose.Wave2;
            Lunge = 0.5f;
            Reach = 0.3f * (1.f - Pose.t);
        } break;

        default:
        {
        } break;
    }

    float BodyX = 26.f + Lunge;
    float BodyY = 28.f + Bob;

    // NOTE(zoubir): speed streaks behind the downstroke
    if (Pose.Anim == AnimationType_Attack)
    {
        for(u32 Streak = 0; Streak < 3; Streak++)
        {
            float Y = BodyY - 3.f + 4.f * Streak;
            float Length = 5.f + 3.f * ((Streak + Pose.Frame) % 3);
            FillLimb(Canvas, V2(4.f, Y), V2(4.f + Length, Y), 0.5f, 0.5f, Blade, 0.8f);
        }
    }

    // NOTE(zoubir): the far wing, a beat behind and darker
    DrawShardWing(Canvas, V2(BodyX - 1.f, BodyY - 4.f), Lift * 0.85f + 0.1f, 17.f,
                  Tip, FarBlade, Aurora);

    // NOTE(zoubir): a fanned tail of three long blades
    for(u32 Feather = 0; Feather < 3; Feather++)
    {
        float Angle = (165.f - 12.f * (float)Feather) * Pi32 / 180.f;
        v2 Dir = V2(Cos(Angle), Sin(Angle));
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 Root = V2(BodyX - 6.f, BodyY + 2.f);
        float Length = 13.f - 1.5f * (float)Feather;
        FillTriangle(Canvas, Root - 2.f * Side, Root + 2.f * Side, Root + Length * Dir,
                     Feather == 1 ? Aurora : Blade, 0.2f, 0.9f);
    }

    // NOTE(zoubir): talons, tucked in flight, flung forward to strike
    for(u32 Leg = 0; Leg < 2; Leg++)
    {
        float Offset = 2.5f * (float)Leg;
        v2 Hip = V2(BodyX + 1.f + Offset, BodyY + 5.f);
        v2 Foot = Hip + V2(-1.f + 9.f * Reach, 4.f + 5.f * Reach);
        FillLimb(Canvas, Hip, Foot, 1.4f, 1.f, Horn, 0.2f * (float)Leg);
        float Open = 1.f + 2.f * Reach;
        for(u32 Claw = 0; Claw < 3; Claw++)
        {
            float ClawX = Foot.X - 1.f + Open * ((float)Claw - 0.5f);
            FillTriangle(Canvas, V2(ClawX - 0.8f, Foot.Y), V2(ClawX + 0.8f, Foot.Y),
                         V2(ClawX + 0.5f * Reach + 0.5f, Foot.Y + 3.f), Aurora, 0.6f, 1.f);
        }
    }

    // NOTE(zoubir): a lean body, pale breast barred with ice scales
    FillBlob(Canvas, BodyX, BodyY, 10.f, 5.5f, Blade, 0.1f);
    FillBlob(Canvas, BodyX + 4.f, BodyY + 1.5f, 6.f, 3.5f, Breast, 0.4f);
    for(u32 Bar = 0; Bar < 3; Bar++)
    {
        float X = BodyX + 2.f + 2.5f * Bar;
        FillDot(Canvas, X, BodyY + 1.f + (float)(Bar % 2), 0.6f, Blade.C[1]);
    }

    // NOTE(zoubir): head with an ice crest, hooked violet beak, green eye
    float HeadX = BodyX + 9.f + 0.5f * Lunge;
    float HeadY = BodyY - 4.f;
    FillTriangle(Canvas, V2(HeadX - 3.f, HeadY - 3.f), V2(HeadX, HeadY - 4.f),
                 V2(HeadX - 9.f - 2.f * Glow, HeadY - 7.f - 2.f * Glow), Blade, 0.4f, 1.f);
    FillBlob(Canvas, HeadX, HeadY, 4.5f, 4.f, Blade, 0.3f);
    FillBlob(Canvas, HeadX + 1.5f, HeadY + 1.5f, 3.f, 2.5f, Breast, 0.5f);
    // NOTE(zoubir): a dark falcon stripe down from the eye
    FillLimb(Canvas, V2(HeadX + 1.f, HeadY - 0.5f), V2(HeadX + 0.5f, HeadY + 3.f), 1.2f, 0.7f, Horn, 0.f);
    FillTriangle(Canvas, V2(HeadX + 3.f, HeadY - 1.5f), V2(HeadX + 3.f, HeadY + 1.5f),
                 V2(HeadX + 8.f, HeadY + 1.f), Horn, 0.5f, 1.f);
    FillTriangle(Canvas, V2(HeadX + 6.f, HeadY), V2(HeadX + 8.f, HeadY + 1.f),
                 V2(HeadX + 6.5f, HeadY + 3.f), Horn, 0.2f, 0.4f);
    FillDot(Canvas, HeadX + 1.5f, HeadY - 1.f, 1.3f + 0.5f * Glow, Glow > 0.5f ? EyeHot : Eye);

    // NOTE(zoubir): the near wing over everything
    DrawShardWing(Canvas, V2(BodyX + 1.f, BodyY - 3.f), Lift, 20.f, Tip, Blade, Aurora);

    // NOTE(zoubir): glints shed off the raised wings as the ice forms
    if (Pose.Anim == AnimationType_Cast)
    {
        for(u32 Spark = 0; Spark < 4; Spark++)
        {
            u32 Seed = (Spark * 7 + Pose.Frame * 3) % 11;
            float X = BodyX - 17.f + 1.2f * (float)Seed + 3.f * (float)Spark;
            float Y = BodyY - 18.f + 1.5f * (float)((Seed * 5) % 7) + 4.f * Pose.t;
            FillDot(Canvas, X, Y, 1.f, Glint);
            FillDot(Canvas, X, Y, 0.5f, Aurora.C[3]);
        }
    }

    OutlineFrame(Canvas, ART_RGB(10, 20, 40));
}

#endif
