/* Aurora Siren: a woman of green and violet aurora light who floats over
   the Aurora Rift (docs/dungeon-rift.md), her body thinning into a veil
   that ripples like the sky. Stillsong is a long note that freezes the
   air (MonsterAbility_Gaze): when the windup ends every player in reach
   who is still moving is hit, past any dodge or jump, so let go of the
   keys while she sings. Lullaby mends the most hurt monster near her,
   and between songs she flicks single notes of light at her target.
   Kill her first or the fight drags on. Only the Aurora Rift's
   encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Siren)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Siren(monster_def *Def)
{
    Def->Name = "Aurora Siren";
    Def->MaxHp = 90.f;
    Def->Acceleration = 30000.f;
    Def->AggroRange = 440.f;
    Def->StopRange = 220.f;
    Def->AttackRange = 36.f;
    Def->AttackDamage = 5.f;
    Def->AttackInterval = 0.9f;
    Def->FlyHeight = 16.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 56;

    // NOTE(zoubir): anyone within 340 still moving faster than 60 when
    // the song ends is hit
    monster_ability *Song = AddMonsterAbility(Def, MonsterAbility_Gaze,
                                              "Stillsong");
    Song->MaxRange = 340.f;
    Song->Cooldown = 9.f;
    Song->Windup = 1.6f;
    Song->Active = 0.5f;
    Song->Recover = 0.6f;
    Song->Damage = 15.f;
    Song->Radius = 340.f;
    Song->Speed = 60.f;

    monster_ability *Lullaby = AddMonsterAbility(Def, MonsterAbility_Mend,
                                                 "Lullaby");
    Lullaby->MaxRange = 600.f;
    Lullaby->Cooldown = 8.f;
    Lullaby->Windup = 1.f;
    Lullaby->Active = 0.3f;
    Lullaby->Recover = 0.5f;
    Lullaby->Radius = 300.f;
    Lullaby->Heal = 40.f;

    monster_ability *Note = AddMonsterAbility(Def, MonsterAbility_Volley,
                                              "Shimmer Note");
    Note->MaxRange = 360.f;
    Note->Cooldown = 3.f;
    Note->Windup = 0.5f;
    Note->Active = 1.1f;
    Note->Recover = 0.4f;
    Note->Damage = 9.f;
    Note->Radius = 12.f;
    Note->Speed = 380.f;
    Note->Spread = 0.f;
    Note->Count = 1;
    Note->ShotStyle = ShotStyle_Spine;
}

#else

internal void
DrawMonster_Siren(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Veil = Ramp(ART_RGB(20, 90, 80), ART_RGB(40, 170, 130),
                           ART_RGB(110, 240, 180), ART_RGB(210, 255, 230));
    color_ramp Violet = Ramp(ART_RGB(60, 30, 110), ART_RGB(120, 70, 190),
                             ART_RGB(180, 130, 240), ART_RGB(240, 220, 255));
    color_ramp Skin = Ramp(ART_RGB(110, 150, 190), ART_RGB(170, 210, 235),
                           ART_RGB(214, 238, 250), ART_RGB(248, 254, 255));
    color_ramp Hair = Ramp(ART_RGB(40, 40, 120), ART_RGB(80, 90, 190),
                           ART_RGB(130, 170, 240), ART_RGB(200, 230, 255));

    float Bob = 1.5f * Pose.Wave;
    float Sway = 2.f * Pose.Wave2;
    // NOTE(zoubir): 0 arms by her sides, 1 spread wide and high
    float Spread = 0.f;
    // NOTE(zoubir): how far her head tips back to sing
    float Tilt = 0.f;
    float Mouth = 0.f;
    // NOTE(zoubir): the rings of song round her, and the burst of light
    float Song = 0.f;
    float Burst = 0.f;
    float Lean = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Lean = 2.f;
            Sway = 3.5f * Pose.Wave;
            Bob = 1.f * Pose.Wave2;
        } break;

        case AnimationType_Cast:
        {
            Spread = Pose.t;
            Tilt = Pose.t;
            Mouth = 0.3f + 0.7f * Pose.t;
            Song = Pose.t;
            Bob = -2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Spread = 1.f;
            Tilt = 1.f - 0.4f * Pose.t;
            Mouth = 1.f;
            Burst = Pose.t;
            Bob = -2.f;
        } break;

        case AnimationType_Stop:
        {
            Spread = 0.6f * (1.f - Pose.t);
            Tilt = 0.4f * (1.f - Pose.t);
            Bob = -1.f + 2.f * Pose.t;
            Sway = 1.f * Pose.Wave;
        } break;

        default: break;
    }

    float CX = 29.f + Lean;
    float WaistY = 28.f + Bob;
    float ChestY = WaistY - 7.f;
    float HeadX = CX + 2.f - 2.5f * Tilt;
    float HeadY = ChestY - 8.f - 1.f * Tilt;

    // NOTE(zoubir): a long violet veil streams off her shoulders behind her
    v2 CapeTop = V2(CX - 3.f, ChestY - 3.f);
    v2 CapeOut = V2(CX - 15.f - Lean + 1.5f * Sway, 42.f);
    v2 CapeIn = V2(CX - 6.f + 0.5f * Sway, 46.f);
    FillTriangle(Canvas, CapeTop, CapeOut, CapeIn, Violet, 0.6f, -0.1f);
    FillTriangle(Canvas, CapeTop, CapeIn, V2(CX, WaistY + 4.f), Violet, 0.5f, 0.f);

    // NOTE(zoubir): her hair, long and loose, falling back over the veil
    FillLimb(Canvas, V2(HeadX - 1.f, HeadY - 1.f), V2(HeadX - 6.f + 0.5f * Sway, HeadY + 10.f),
             4.f, 2.5f, Hair, 0.f);
    FillLimb(Canvas, V2(HeadX - 6.f + 0.5f * Sway, HeadY + 10.f),
             V2(HeadX - 9.f + Sway, HeadY + 19.f), 2.5f, 0.8f, Hair, -0.1f);
    FillLimb(Canvas, V2(HeadX - 3.f, HeadY), V2(HeadX - 11.f + 1.2f * Sway, HeadY + 13.f),
             2.f, 0.8f, Hair, 0.2f);

    // NOTE(zoubir): the far arm, behind the body
    v2 FarShoulder = V2(CX - 3.f, ChestY - 2.f);
    v2 FarHand = FarShoulder + V2(-2.f - 8.f * Spread, 8.f - 17.f * Spread);
    FillLimb(Canvas, FarShoulder, FarHand, 1.6f, 1.1f, Skin, -0.3f);
    FillDot(Canvas, FarHand.X, FarHand.Y, 0.8f + Song + Burst, ART_RGB(220, 200, 255));

    // NOTE(zoubir): the bodice of light
    FillBlob(Canvas, CX, ChestY + 1.f, 4.5f, 5.5f, Veil, 0.25f);

    // NOTE(zoubir): below the waist a robe of light flares out, then
    // thins to a point instead of legs, streaked violet like the sky
    v2 WaistL = V2(CX - 4.f, WaistY);
    v2 WaistR = V2(CX + 4.f, WaistY);
    v2 FlareL = V2(CX - 8.f + 0.3f * Sway, WaistY + 8.f);
    v2 FlareR = V2(CX + 6.f + 0.3f * Sway, WaistY + 8.f);
    v2 Tip = V2(CX - 3.f - Lean + Sway, 49.f);
    FillTriangle(Canvas, WaistL, WaistR, FlareR, Veil, 0.9f, 0.6f);
    FillTriangle(Canvas, WaistL, FlareR, FlareL, Veil, 0.8f, 0.4f);
    FillTriangle(Canvas, FlareL, FlareR, Tip, Veil, 0.6f, 0.f);
    for(u32 Streak = 0; Streak < 2; Streak++)
    {
        float X = CX - 3.f + 5.f * Streak;
        FillTriangle(Canvas, V2(X - 1.f, WaistY + 2.f), V2(X + 1.f, WaistY + 2.f),
                     V2(Tip.X + 2.f * Streak - Sway * (Streak ? 0.6f : -0.4f), Tip.Y - 4.f - 4.f * Streak),
                     Violet, 0.9f, 0.4f);
    }
    FillLimb(Canvas, WaistL + V2(0.f, 0.5f), WaistR + V2(0.f, 0.5f), 0.8f, 0.8f, Violet, 0.6f);

    // NOTE(zoubir): neck and head, tipping back as she sings
    FillLimb(Canvas, V2(CX + 0.5f, ChestY - 4.f), V2(HeadX, HeadY + 3.f), 1.3f, 1.3f, Skin, 0.f);
    FillBlob(Canvas, HeadX, HeadY, 3.8f, 4.3f, Skin, 0.3f);
    FillBlob(Canvas, HeadX - 2.5f, HeadY - 2.f, 3.f, 2.8f, Hair, 0.3f);
    FillDot(Canvas, HeadX + 1.5f, HeadY - 0.5f - 0.5f * Tilt, 0.7f, ART_RGB(40, 60, 110));
    if (Mouth > 0.f)
    {
        FillDot(Canvas, HeadX + 2.f, HeadY + 2.f - Tilt, 0.4f + 0.8f * Mouth, ART_RGB(30, 30, 70));
    }

    // NOTE(zoubir): the near arm sweeps up and out as the song builds, a
    // sleeve of veil hanging off it
    v2 Shoulder = V2(CX + 3.f, ChestY - 2.f);
    v2 Hand = Shoulder + V2(2.f + 7.f * Spread, 9.f - 18.f * Spread);
    FillTriangle(Canvas, Shoulder, Shoulder + 0.7f * (Hand - Shoulder),
                 Shoulder + 0.5f * (Hand - Shoulder) + V2(-1.f, 5.f), Veil, 0.4f, -0.1f);
    FillLimb(Canvas, Shoulder, Hand, 1.8f, 1.2f, Skin, 0.2f);
    FillDot(Canvas, Hand.X, Hand.Y, 0.8f + Song + Burst, ART_RGB(220, 255, 240));

    // NOTE(zoubir): rings of song swell round her through the windup
    v2 Center = V2(CX, ChestY);
    if (Song > 0.f)
    {
        for(u32 Ring = 0; Ring < 2; Ring++)
        {
            float R = 7.f + 8.f * Song + 5.f * Ring;
            u32 Count = 10 + 4 * Ring;
            for(u32 Mote = 0; Mote < Count; Mote++)
            {
                float Angle = 2.f * Pi32 * ((float)Mote / (float)Count) + 0.6f * Pose.t + 0.3f * Ring;
                float X = Center.X + R * Cos(Angle);
                float Y = Center.Y + 0.75f * R * Sin(Angle);
                u32 Color = ((Mote + Ring) % 2) ? ART_RGB(140, 255, 200) : ART_RGB(200, 160, 255);
                FillDot(Canvas, X, Y, 0.5f + 0.4f * Song, Color);
            }
        }
        // NOTE(zoubir): two notes of light rising off her
        for(u32 Tone = 0; Tone < 2; Tone++)
        {
            float X = Center.X + (Tone ? 13.f : -15.f);
            float Y = Center.Y - 2.f - 8.f * Song + 4.f * Tone;
            FillDot(Canvas, X, Y, 1.4f, Tone ? ART_RGB(150, 255, 210) : ART_RGB(210, 170, 255));
            FillLimb(Canvas, V2(X + 1.f, Y), V2(X + 1.f, Y - 4.f), 0.5f, 0.5f, Tone ? Veil : Violet, 0.8f);
        }
    }

    // NOTE(zoubir): the pulse, a bright ring bursting outward
    if (Pose.Anim == AnimationType_Attack)
    {
        FillDot(Canvas, Center.X, Center.Y, 5.f * (1.f - Burst) + 1.f, ART_RGB(240, 255, 250));
        float R = 6.f + 15.f * Burst;
        u32 Count = 22;
        for(u32 Mote = 0; Mote < Count; Mote++)
        {
            float Angle = 2.f * Pi32 * ((float)Mote / (float)Count);
            u32 Color = (Mote % 2) ? ART_RGB(170, 255, 220) : ART_RGB(220, 190, 255);
            FillDot(Canvas, Center.X + R * Cos(Angle), Center.Y + 0.8f * R * Sin(Angle),
                    1.4f - 0.6f * Burst, Color);
        }
    }

    OutlineFrame(Canvas, ART_RGB(10, 20, 40));
}

#endif
