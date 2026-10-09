/* Hollow Glutton: a bloated squat thing of black basalt in the Starless
   Deep (docs/dungeon-starless.md), almost all mouth, with the gold of a
   swallowed star shard glowing through the cracks in its belly. Gorge
   rings the player farthest from it with a circle (MonsterAbility_Share) and bites down on
   it: the blow is split between everyone standing in the circle, so the
   party gathers in it and each takes a share, and they bleed. Gulp opens
   a well at its own feet (MonsterAbility_Pull) that drags the party
   toward its mouth and snaps shut on whoever reaches it: run against the
   pull or dash out. Only the Starless Deep's encounters bring it
   (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Glutton)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Glutton(monster_def *Def)
{
    Def->Name = "Hollow Glutton";
    Def->MaxHp = 200.f;
    Def->Acceleration = 20000.f;
    Def->AggroRange = 380.f;
    Def->StopRange = 48.f;
    Def->AttackRange = 62.f;
    Def->AttackDamage = 15.f;
    Def->AttackInterval = 1.3f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 72;

    // NOTE(zoubir): split between everyone in the circle, so the party
    // gathers on whoever it picks
    monster_ability *Gorge = AddMonsterAbility(Def, MonsterAbility_Share,
                                               "Gorge");
    Gorge->MaxRange = 380.f;
    Gorge->Cooldown = 13.f;
    Gorge->Windup = 2.f;
    Gorge->Active = 0.4f;
    Gorge->Recover = 1.f;
    Gorge->Damage = 11.f;
    Gorge->Radius = 90.f;
    Gorge->Status = StatusEffect_Bleeding;
    Gorge->StatusSeconds = 3.f;

    // NOTE(zoubir): the well opens at its own feet (Spread 0)
    monster_ability *Gulp = AddMonsterAbility(Def, MonsterAbility_Pull,
                                              "Gulp");
    Gulp->MaxRange = 150.f;
    Gulp->Cooldown = 8.f;
    Gulp->Windup = 0.8f;
    Gulp->Active = 1.4f;
    Gulp->Recover = 0.8f;
    Gulp->Damage = 12.f;
    Gulp->Radius = 170.f;
    Gulp->InnerRadius = 60.f;
    Gulp->Speed = 800.f;
    Gulp->Spread = 0.f;
    Gulp->Knockback = 340.f;
}

#else

inline v2
GluttonArm(float Angle, float Reach)
{
    v2 Result = V2(Reach * Cos(Angle), Reach * Sin(Angle));
    return Result;
}

internal void
DrawMonster_Glutton(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Stone = Ramp(ART_RGB(14, 12, 20), ART_RGB(34, 30, 44),
                            ART_RGB(58, 52, 72), ART_RGB(92, 84, 110));
    color_ramp Gold = Ramp(ART_RGB(110, 60, 10), ART_RGB(190, 130, 30),
                           ART_RGB(240, 200, 80), ART_RGB(255, 246, 190));
    color_ramp Maw = Ramp(ART_RGB(20, 6, 26), ART_RGB(48, 14, 58),
                          ART_RGB(86, 30, 96), ART_RGB(130, 60, 140));
    color_ramp Tooth = Ramp(ART_RGB(78, 72, 96), ART_RGB(128, 120, 146),
                            ART_RGB(182, 176, 196), ART_RGB(226, 222, 236));

    float Bob = 0.f;
    float Step = 0.f;
    float Lean = 0.f;
    // NOTE(zoubir): 0 shut, 1 gaping as wide as it goes
    float Open = 0.15f;
    // NOTE(zoubir): how bright the swallowed gold burns, 0..1
    float Glow = 0.5f;
    float Dust = 0.f;
    float Swallow = -1.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 4.f * Pose.Wave;
            Bob = -2.f * Absolute(Pose.Wave);
            Lean = 1.5f * Pose.Wave;
            Open = 0.12f + 0.05f * Pose.Wave2;
        } break;

        case AnimationType_Cast:
        {
            Open = 0.2f + 0.8f * Pose.t;
            Lean = -2.f * Pose.t;
            Bob = -2.5f * Pose.t;
            Glow = 0.5f + 0.5f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            float Snap = Minimum(1.f, 3.f * Pose.t);
            Open = 1.f - Snap;
            Lean = -2.f + 5.f * Snap;
            Bob = -2.5f + 5.f * Snap;
            Glow = 1.f;
            Dust = (Pose.t > 0.2f) ? Pose.t : 0.f;
        } break;

        case AnimationType_Stop:
        {
            Open = 0.05f + 0.25f * Absolute(Pose.Wave);
            Bob = 2.f - 1.5f * Pose.t;
            Lean = 2.f * (1.f - Pose.t);
            Glow = 0.4f + 0.6f * Pose.t;
            Swallow = Pose.t;
        } break;

        default:
        {
            Bob = 0.8f * Pose.Wave;
            Open = 0.18f + 0.07f * Pose.Wave2;
            Glow = 0.5f + 0.4f * Pose.Wave;
        } break;
    }

    float Ground = 63.f;
    float CY = 41.f + Bob;
    float CX = 35.f + 0.5f * Lean;

    FillFlatEllipse(Canvas, CX, Ground + 0.5f, 23.f, 2.5f, ART_RGB(10, 8, 14));

    // NOTE(zoubir): four stubby legs, mostly hidden under the belly
    float LegX[4] = {CX - 16.f, CX + 8.f, CX - 9.f, CX + 15.f};
    for(u32 Leg = 0; Leg < 4; Leg++)
    {
        bool32 Near = (Leg >= 2);
        float Swing = ((Leg == 0) || (Leg == 3)) ? Step : -Step;
        float FootX = LegX[Leg] + Swing;
        float Shade = Near ? 0.f : -0.45f;
        FillLimb(Canvas, V2(LegX[Leg], CY + 10.f), V2(FootX, Ground - 3.f), 5.5f, 5.f, Stone, Shade);
        FillBlob(Canvas, FootX + 1.f, Ground - 1.5f, 6.f, 2.5f, Stone, Shade + 0.1f);
        if (Near)
        {
            FillTriangle(Canvas, V2(FootX + 5.f, Ground - 3.f), V2(FootX + 5.f, Ground),
                         V2(FootX + 8.5f, Ground), Tooth, 0.2f, 0.7f);
            FillTriangle(Canvas, V2(FootX + 2.f, Ground - 2.f), V2(FootX + 2.f, Ground),
                         V2(FootX + 5.f, Ground), Tooth, 0.2f, 0.7f);
        }
    }

    // NOTE(zoubir): the bloated belly, gold leaking through its cracks
    FillBlob(Canvas, CX, CY + 5.f, 22.f, 15.f, Stone, -0.05f);
    float CrackLight = -0.4f + 0.8f * Glow;
    float CrackR = 0.7f + 0.6f * Glow;
    v2 Crack[6] =
    {
        V2(CX - 18.f, CY + 3.f), V2(CX - 12.f, CY + 9.f), V2(CX - 6.f, CY + 6.f),
        V2(CX - 1.f, CY + 13.f), V2(CX + 6.f, CY + 10.f), V2(CX + 12.f, CY + 16.f),
    };
    for(u32 Seg = 0; Seg + 1 < 6; Seg++)
    {
        FillLimb(Canvas, Crack[Seg], Crack[Seg + 1], CrackR, CrackR, Gold, CrackLight);
    }
    FillLimb(Canvas, Crack[1], V2(CX - 14.f, CY + 15.f), CrackR * 0.8f, 0.5f, Gold, CrackLight);
    FillLimb(Canvas, Crack[2], V2(CX - 4.f, CY + 1.f), CrackR * 0.8f, 0.5f, Gold, CrackLight);
    FillLimb(Canvas, Crack[4], V2(CX + 4.f, CY + 17.f), CrackR * 0.8f, 0.5f, Gold, CrackLight);
    FillBlob(Canvas, CX - 6.f, CY + 9.f, 2.f + 2.f * Glow, 1.5f + 1.5f * Glow, Gold, 0.2f + 0.4f * Glow);
    if (Glow > 0.8f)
    {
        FillDot(Canvas, CX - 6.f, CY + 9.f, 1.2f, ART_RGB(255, 252, 230));
    }
    // NOTE(zoubir): in the chew, a lump of gold sinks into the belly
    if (Swallow >= 0.f)
    {
        FillBlob(Canvas, CX + 6.f - 12.f * Swallow, CY + 1.f + 7.f * Swallow,
                 2.5f, 2.f, Gold, 0.4f);
    }

    // NOTE(zoubir): the mouth runs nearly the width of it. The jaws hinge
    // at the back; the upper one rears up as it gapes, the lower one drops
    v2 Hinge = V2(CX - 9.f, CY - 3.f);
    float UpperAngle = -0.05f - 0.7f * Open;
    float LowerAngle = 0.08f + 0.4f * Open;
    v2 UpDir = GluttonArm(UpperAngle, 1.f);
    v2 UpOut = GluttonArm(UpperAngle - 0.5f * Pi32, 1.f);
    v2 LowDir = GluttonArm(LowerAngle, 1.f);
    v2 LowOut = GluttonArm(LowerAngle + 0.5f * Pi32, 1.f);
    float UpperLength = 28.f;
    float LowerLength = 27.f;
    v2 UpperTip = Hinge + UpperLength * UpDir;
    v2 LowerTip = Hinge + LowerLength * LowDir;

    // NOTE(zoubir): the gullet, dark violet, the gold glowing deep in it
    FillTriangle(Canvas, Hinge, UpperTip, LowerTip, Maw, 0.f, 0.4f);
    FillBlob(Canvas, Hinge.X + 6.f, Hinge.Y + 1.f, 6.f, 2.f + 5.f * Open, Maw, -0.3f);
    FillDot(Canvas, Hinge.X + 5.f, Hinge.Y + 1.f, 0.5f + 2.5f * Open * Glow, ART_RGB(250, 200, 90));
    if (Open > 0.3f)
    {
        v2 TongueTip = Hinge + (12.f + 5.f * Open) * LowDir + 2.f * LowOut;
        FillLimb(Canvas, Hinge + V2(4.f, 2.f), TongueTip, 2.5f, 2.f, Maw, 0.7f);
    }

    // NOTE(zoubir): the lower jaw, a heavy lip of slab, teeth pointing up
    v2 LowA = Hinge + 5.f * LowOut;
    for(u32 Fang = 0; Fang < 5; Fang++)
    {
        float S = 7.f + 4.5f * Fang;
        float R = Lerp(8.f, S / LowerLength, 5.f);
        v2 Base = LowA + S * LowDir - (R - 1.f) * LowOut;
        float Height = (Fang % 2) ? 3.f : 4.5f;
        FillTriangle(Canvas, Base - 1.7f * LowDir, Base + 1.7f * LowDir,
                     Base - Height * LowOut, Tooth, 0.1f, 0.9f);
    }
    FillLimb(Canvas, LowA, LowA + LowerLength * LowDir, 8.f, 5.f, Stone, 0.05f);

    // NOTE(zoubir): the upper jaw and skull, one heavy dome with jagged
    // fangs hanging from its edge
    v2 UpA = Hinge + 7.f * UpOut;
    for(u32 Fang = 0; Fang < 6; Fang++)
    {
        float S = 6.f + 4.2f * Fang;
        float R = Lerp(13.f, S / UpperLength, 6.f);
        v2 Base = UpA + S * UpDir - (R - 1.f) * UpOut;
        float Height = (Fang % 2) ? 3.5f : 5.5f;
        FillTriangle(Canvas, Base - 1.8f * UpDir, Base + 1.8f * UpDir,
                     Base - Height * UpOut, Tooth, 0.2f, 1.f);
    }
    FillLimb(Canvas, UpA, UpA + UpperLength * UpDir, 13.f, 6.f, Stone, 0.2f);
    FillLimb(Canvas, UpA - 6.f * UpDir + 2.f * UpOut, UpA + 4.f * UpDir - 4.f * UpOut,
             0.6f, 0.6f, Gold, CrackLight);
    // NOTE(zoubir): basalt spurs along its back
    for(u32 Spur = 0; Spur < 3; Spur++)
    {
        v2 Base = UpA + (-6.f + 6.f * Spur) * UpDir + (11.f - 0.5f * Spur) * UpOut;
        FillTriangle(Canvas, Base - 2.5f * UpDir, Base + 2.5f * UpDir,
                     Base + (4.5f - (float)(Spur % 2)) * UpOut - 1.f * UpDir, Stone, 0.5f, 0.1f);
    }

    // NOTE(zoubir): small violet eyes set high on the dome, flaring as
    // the jaw gapes
    v2 Eye = UpA + 14.f * UpDir + 5.f * UpOut;
    float EyeR = 1.3f + 0.6f * Open;
    v2 FarEye = Eye - 4.f * UpDir + 0.5f * UpOut;
    FillDot(Canvas, FarEye.X, FarEye.Y, EyeR, ART_RGB(110, 50, 180));
    FillDot(Canvas, Eye.X, Eye.Y, EyeR + 0.7f, ART_RGB(30, 10, 40));
    FillDot(Canvas, Eye.X, Eye.Y, EyeR, ART_RGB(176, 96, 255));
    FillDot(Canvas, Eye.X + 0.4f, Eye.Y - 0.4f, 0.6f, ART_RGB(240, 222, 255));

    // NOTE(zoubir): the bite kicks up grit in front of it
    if (Dust > 0.f)
    {
        for(u32 Puff = 0; Puff < 3; Puff++)
        {
            float X = 56.f + (3.f + 4.f * Dust) * (float)Puff;
            float Y = Ground - 2.f - 4.f * Dust - (float)(Puff % 2);
            FillBlob(Canvas, Minimum(X, 68.f), Y, 2.5f - 0.5f * Puff, 2.f - 0.4f * Puff, Stone, 0.5f);
        }
    }

    OutlineFrame(Canvas, ART_RGB(6, 4, 10));
}

#endif
