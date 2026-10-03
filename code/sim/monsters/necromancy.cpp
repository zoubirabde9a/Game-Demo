/* The necromancy family, two kinds in one file.

   Bone Shaman: hangs back behind its dead. Mend pours green light into
   the most hurt monster near it. Raise Dead marks two graves between it
   and you, and Skeletal Thralls climb out when the windup ends (stand on
   a grave to stop that one). It keeps at most four thralls; kill the
   shaman and every thrall it raised falls apart.

   Skeletal Thrall: a quick, brittle skeleton with a rusted blade. Never
   spawns on its own (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Shaman)
MONSTER(Thrall)
#else

internal void
DefineMonster_Shaman(monster_def *Def)
{
    Def->Name = "Bone Shaman";
    Def->MaxHp = 60.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 440.f;
    Def->StopRange = 210.f;
    Def->AttackRange = 40.f;
    Def->AttackDamage = 5.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 48;
    Def->FrameCounts[MonsterRow_Windup] = 6;

    // NOTE(zoubir): healing comes first; it only starts when an ally is hurt
    monster_ability *Mend = AddMonsterAbility(Def, MonsterAbility_Mend, "Mend");
    Mend->MaxRange = 450.f;
    Mend->Cooldown = 6.f;
    Mend->Windup = 0.8f;
    Mend->Active = 0.3f;
    Mend->Recover = 0.4f;
    Mend->Radius = 220.f;
    Mend->Heal = 35.f;

    monster_ability *Raise = AddMonsterAbility(Def, MonsterAbility_Summon,
                                               "Raise Dead");
    Raise->MaxRange = 400.f;
    Raise->Cooldown = 7.f;
    Raise->Windup = 1.f;
    Raise->Active = 0.3f;
    Raise->Recover = 0.6f;
    Raise->SummonKind = MonsterKind_Thrall;
    Raise->Count = 2;
    Raise->MaxActive = 4;
    Raise->Spread = 45.f;
    Raise->Radius = 14.f;
}

internal void
DefineMonster_Thrall(monster_def *Def)
{
    Def->Name = "Skeletal Thrall";
    Def->MaxHp = 30.f;
    Def->Acceleration = 34000.f;
    Def->AggroRange = 500.f;
    Def->StopRange = 30.f;
    Def->AttackRange = 40.f;
    Def->AttackDamage = 6.f;
    Def->AttackInterval = 0.8f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 40;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.08f;
}

internal void
DrawMonster_Shaman(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Hide = Ramp(ART_RGB(44, 34, 28), ART_RGB(78, 60, 44),
                           ART_RGB(112, 88, 62), ART_RGB(146, 120, 86));
    color_ramp Skin = Ramp(ART_RGB(50, 60, 46), ART_RGB(80, 96, 70),
                           ART_RGB(110, 130, 94), ART_RGB(140, 160, 120));
    color_ramp Bone = Ramp(ART_RGB(120, 110, 94), ART_RGB(184, 174, 150),
                           ART_RGB(224, 216, 192), ART_RGB(250, 246, 230));
    color_ramp Wood = Ramp(ART_RGB(40, 26, 18), ART_RGB(70, 46, 30),
                           ART_RGB(100, 70, 44), ART_RGB(130, 96, 62));
    color_ramp Soul = Ramp(ART_RGB(30, 120, 60), ART_RGB(60, 200, 100),
                           ART_RGB(130, 250, 150), ART_RGB(220, 255, 220));

    float Bob = 0.f;
    float Lean = 0.f;
    // NOTE(zoubir): staff top, where the green skull sits
    v2 StaffTop = V2(36.f, 9.f);
    v2 Hand = V2(32.f, 28.f);
    float Glow = 0.f;
    float Stride = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Stride = 2.5f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
            StaffTop.X += 1.5f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): staff lifted high, the skull blazing brighter
            StaffTop = Lerp2(StaffTop, Pose.t, V2(30.f, 3.f));
            Hand = Lerp2(Hand, Pose.t, V2(29.f, 20.f));
            Glow = Pose.t;
            Lean = -1.5f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            StaffTop = Lerp2(V2(30.f, 3.f), Pose.t, V2(42.f, 12.f));
            Hand = V2(33.f, 24.f);
            Glow = 1.f - 0.5f * Pose.t;
            Lean = 2.f;
        } break;

        case AnimationType_Stop:
        {
            Lean = 2.f;
            Bob = 1.f + 0.5f * Pose.Wave;
        } break;

        default:
        {
            Bob = 0.7f * Pose.Wave;
            Glow = 0.2f + 0.2f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): feet, robe, dangling bone charms
    FillBlob(Canvas, 18.f - Stride, 42.f, 3.f, 1.6f, Skin, -0.2f);
    FillBlob(Canvas, 26.f + Stride, 42.f, 3.f, 1.6f, Skin);
    FillTriangle(Canvas, V2(22.f + Lean, 18.f + Bob), V2(10.f, 41.f),
                 V2(32.f, 41.f), Hide, 0.8f, 0.25f);
    FillBlob(Canvas, 21.f + Lean, 28.f + Bob, 9.f, 9.f, Hide);
    for(u32 Charm = 0; Charm < 3; Charm++)
    {
        float X = 15.f + 5.f * Charm + Lean;
        float Y = 33.f + Bob + (float)(Charm % 2);
        FillLimb(Canvas, V2(X, Y - 3.f), V2(X, Y), 0.4f, 0.4f, Hide, -0.4f);
        FillBlob(Canvas, X, Y + 1.f, 1.3f, 1.6f, Bone);
    }

    // NOTE(zoubir): staff behind the hand, skull on top
    FillLimb(Canvas, V2(Hand.X - 2.f, 43.f), StaffTop, 1.3f, 1.1f, Wood);
    float SkullR = 3.5f;
    FillBlob(Canvas, StaffTop.X, StaffTop.Y, SkullR, SkullR * 0.9f, Bone, 0.05f);
    FillDot(Canvas, StaffTop.X - 1.f, StaffTop.Y, 0.9f, Soul.C[2 + (Glow > 0.5f)]);
    FillDot(Canvas, StaffTop.X + 1.4f, StaffTop.Y, 0.9f, Soul.C[2 + (Glow > 0.5f)]);
    if (Glow > 0.f)
    {
        // NOTE(zoubir): souls circling the skull while it charges
        u32 Motes = 2 + (u32)(4.f * Glow);
        for(u32 Mote = 0; Mote < Motes; Mote++)
        {
            float Angle = 2.f * Pi32 * ((float)Mote / (float)Motes + 0.5f * Pose.t);
            float R = 5.f + 2.f * Glow;
            FillDot(Canvas, StaffTop.X + R * Cos(Angle),
                    StaffTop.Y + 0.7f * R * Sin(Angle), 0.6f + 0.6f * Glow,
                    Soul.C[1 + (Mote % 3)]);
        }
    }

    // NOTE(zoubir): hunched head under an antlered bone mask
    float HeadX = 27.f + Lean;
    float HeadY = 19.f + Bob;
    FillLimb(Canvas, V2(HeadX - 3.f, HeadY - 4.f), V2(HeadX - 7.f, HeadY - 11.f),
             1.f, 0.5f, Bone, -0.1f);
    FillLimb(Canvas, V2(HeadX - 6.f, HeadY - 8.f), V2(HeadX - 10.f, HeadY - 9.f),
             0.7f, 0.4f, Bone, -0.1f);
    FillLimb(Canvas, V2(HeadX + 1.f, HeadY - 5.f), V2(HeadX + 3.f, HeadY - 12.f),
             1.f, 0.5f, Bone);
    FillBlob(Canvas, HeadX - 2.f, HeadY + 1.f, 6.f, 6.f, Hide, -0.1f);
    FillBlob(Canvas, HeadX + 1.f, HeadY, 4.5f, 5.f, Bone, 0.05f);
    FillDot(Canvas, HeadX + 1.f, HeadY - 0.5f, 1.f, Soul.C[2]);
    FillDot(Canvas, HeadX + 3.5f, HeadY - 0.5f, 0.9f, Soul.C[2]);
    FillLimb(Canvas, V2(HeadX, HeadY + 3.f), V2(HeadX + 4.f, HeadY + 3.f),
             0.4f, 0.4f, Ramp(Bone.C[0], Bone.C[0], Bone.C[0], Bone.C[0]));

    // NOTE(zoubir): gnarled hand gripping the staff
    FillLimb(Canvas, V2(24.f + Lean, 24.f + Bob), Hand, 1.6f, 1.3f, Skin);
    FillBlob(Canvas, Hand.X, Hand.Y, 1.8f, 1.8f, Skin, 0.1f);

    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): the spell leaves the skull as a burst of light
        float Burst = 3.f + 6.f * Pose.t;
        for(u32 Ray = 0; Ray < 6; Ray++)
        {
            float Angle = 2.f * Pi32 * (float)Ray / 6.f;
            FillDot(Canvas, StaffTop.X + Burst * Cos(Angle),
                    StaffTop.Y + Burst * Sin(Angle), 1.f, Soul.C[2]);
        }
    }

    OutlineFrame(Canvas, ART_RGB(14, 12, 10));
}

internal void
DrawMonster_Thrall(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Bone = Ramp(ART_RGB(110, 100, 86), ART_RGB(176, 166, 142),
                           ART_RGB(214, 206, 182), ART_RGB(240, 236, 220));
    // NOTE(zoubir): pitted grey steel with a brown cast
    color_ramp Rust = Ramp(ART_RGB(58, 52, 50), ART_RGB(104, 96, 90),
                           ART_RGB(150, 140, 128), ART_RGB(206, 200, 190));
    u32 Eye = ART_RGB(120, 255, 150);
    u32 Hollow = ART_RGB(26, 22, 20);

    float Bob = 0.f;
    float Stride = 0.f;
    float Jaw = 0.f;
    // NOTE(zoubir): sword angle in radians, see the blade below
    float Swing = 0.6f;
    float Lean = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Stride = 3.f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
            Swing = 0.7f + 0.2f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            Swing = Lerp(0.6f, Pose.t, -1.6f);
            Lean = -1.f;
        } break;

        case AnimationType_Attack:
        {
            Swing = Lerp(-1.6f, Pose.t, 1.2f);
            Lean = 2.f;
            Jaw = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Bob = 2.f;
            Swing = 1.4f;
            Jaw = 1.f;
        } break;

        default:
        {
            Jaw = (Pose.Frame % 2) ? 1.f : 0.f;
        } break;
    }

    float Hip = 27.f + Bob;
    // NOTE(zoubir): legs are two thin bones each
    FillLimb(Canvas, V2(17.f, Hip), V2(16.f - Stride, 31.f), 1.f, 0.9f, Bone, -0.2f);
    FillLimb(Canvas, V2(16.f - Stride, 31.f), V2(15.f - Stride, 35.f), 0.9f, 0.8f, Bone, -0.2f);
    FillLimb(Canvas, V2(21.f, Hip), V2(22.f + Stride, 31.f), 1.f, 0.9f, Bone);
    FillLimb(Canvas, V2(22.f + Stride, 31.f), V2(23.f + Stride, 35.f), 0.9f, 0.8f, Bone);
    FillBlob(Canvas, 15.f - Stride, 35.5f, 1.8f, 0.9f, Bone, -0.2f);
    FillBlob(Canvas, 24.f + Stride, 35.5f, 1.8f, 0.9f, Bone);

    // NOTE(zoubir): pelvis, spine and a ribcage of curved bones
    FillBlob(Canvas, 19.f, Hip, 4.f, 2.f, Bone);
    v2 Neck = V2(21.f + Lean, 14.f + Bob);
    FillLimb(Canvas, V2(19.f, Hip), Neck, 0.9f, 0.9f, Bone, -0.1f);
    for(u32 Rib = 0; Rib < 3; Rib++)
    {
        float Y = 17.f + 2.5f * Rib + Bob;
        float X = 20.f + Lean * (1.f - 0.3f * Rib);
        FillLimb(Canvas, V2(X - 4.f + Rib * 0.5f, Y + 1.f), V2(X + 4.f - Rib * 0.5f, Y),
                 0.8f, 0.6f, Bone);
    }

    // NOTE(zoubir): skull with a loose jaw and green pinprick eyes
    float SkullX = Neck.X + 1.f;
    float SkullY = Neck.Y - 4.f;
    FillBlob(Canvas, SkullX, SkullY, 4.5f, 4.f, Bone, 0.1f);
    FillBlob(Canvas, SkullX + 1.5f, SkullY + 3.5f + Jaw, 3.f, 1.4f, Bone);
    FillDot(Canvas, SkullX + 0.5f, SkullY, 1.3f, Hollow);
    FillDot(Canvas, SkullX + 3.f, SkullY, 1.1f, Hollow);
    PutPixel(Canvas, (i32)(SkullX + 0.5f), (i32)SkullY, Eye);
    PutPixel(Canvas, (i32)(SkullX + 3.f), (i32)SkullY, Eye);

    // NOTE(zoubir): sword arm and a notched, rusted blade
    v2 Shoulder = V2(22.f + Lean, 17.f + Bob);
    v2 Hand = Shoulder + V2(5.f, 4.f);
    FillLimb(Canvas, Shoulder, Hand, 0.8f, 0.8f, Bone);
    // NOTE(zoubir): Swing 0 holds the blade straight up, positive tips it
    // forward, negative back over the shoulder
    v2 Tip = Hand + V2(13.f * Sin(Swing), -13.f * Cos(Swing));
    FillLimb(Canvas, Hand, Tip, 1.4f, 0.6f, Rust, 0.1f);
    FillLimb(Canvas, Hand - V2(1.5f, 0.f), Hand + V2(1.5f, 0.f), 0.6f, 0.6f, Rust, -0.2f);

    OutlineFrame(Canvas, ART_RGB(20, 16, 14));
}

#endif
