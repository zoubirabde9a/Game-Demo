/* Duskweb Matron: a spider as big as a cart from the Starless Deep
   (docs/dungeon-starless.md), glossy black with violet star-maps on her
   back and silk of starlight hanging off her. Hush of the Web
   (MonsterAbility_Gaze) is a stare out to 320: when she rears up and her
   eyes blaze, stop moving, because whoever still moves when it ends takes
   the hit and is rooted. Venom Spray poisons a short cone in front of her,
   so stand beside her rather than before her, and Starsilk Snare lobs
   three webs that slow anyone walking through them. Only the deep's
   encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Matron)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Matron(monster_def *Def)
{
    Def->Name = "Duskweb Matron";
    Def->MaxHp = 170.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 420.f;
    Def->StopRange = 46.f;
    Def->AttackRange = 60.f;
    Def->AttackDamage = 12.f;
    Def->AttackInterval = 1.1f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 72;

    // NOTE(zoubir): everyone out to 320 still moving faster than 60 when
    // the windup ends is hit and rooted
    monster_ability *Hush = AddMonsterAbility(Def, MonsterAbility_Gaze,
                                              "Hush of the Web");
    Hush->MaxRange = 320.f;
    Hush->Cooldown = 9.f;
    Hush->Windup = 1.5f;
    Hush->Active = 0.5f;
    Hush->Recover = 0.6f;
    Hush->Damage = 10.f;
    Hush->Radius = 320.f;
    Hush->Speed = 60.f;
    Hush->Status = StatusEffect_Rooted;
    Hush->StatusSeconds = 0.8f;

    monster_ability *Spray = AddMonsterAbility(Def, MonsterAbility_Cone,
                                               "Venom Spray");
    Spray->MaxRange = 140.f;
    Spray->Cooldown = 5.f;
    Spray->Windup = 0.7f;
    Spray->Active = 0.3f;
    Spray->Recover = 0.6f;
    Spray->Damage = 10.f;
    Spray->Radius = 160.f;
    Spray->Spread = 60.f;
    Spray->Knockback = 120.f;
    Spray->Status = StatusEffect_Poisoned;
    Spray->StatusSeconds = 4.f;

    monster_ability *Snare = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                               "Starsilk Snare");
    Snare->MinRange = 120.f;
    Snare->MaxRange = 380.f;
    Snare->Cooldown = 6.f;
    Snare->Windup = 0.9f;
    Snare->Active = 0.3f;
    Snare->Recover = 0.6f;
    Snare->Damage = 8.f;
    Snare->Radius = 55.f;
    Snare->Spread = 100.f;
    Snare->Count = 3;
    Snare->HazardSeconds = 5.f;
    Snare->HazardStyle = HazardStyle_Web;
    Snare->Status = StatusEffect_Slowed;
    Snare->StatusSeconds = 1.f;
}

#else

// NOTE(zoubir): one leg as hip -> knee -> foot; a standing leg's knee
// arches high over the hip, a raised leg's knee bends forward
internal void
DrawMatronLeg(sprite_canvas *Canvas, v2 Hip, v2 Foot, float KneeLift, float Raise,
              color_ramp Shell, u32 Band, float LightBias)
{
    v2 Arched = V2(Hip.X + 0.45f * (Foot.X - Hip.X), Hip.Y - KneeLift);
    v2 Bent = Lerp2(Hip, 0.5f, Foot) + V2(5.f, 1.f);
    v2 Knee = Lerp2(Arched, Raise, Bent);
    FillLimb(Canvas, Hip, Knee, 2.6f, 2.f, Shell, LightBias);
    FillLimb(Canvas, Knee, Foot, 2.f, 0.8f, Shell, LightBias);
    FillDot(Canvas, Knee.X, Knee.Y, 1.f, Band);
}

internal void
DrawMonster_Matron(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Shell = Ramp(ART_RGB(10, 8, 16), ART_RGB(26, 22, 38),
                            ART_RGB(52, 46, 72), ART_RGB(104, 96, 136));
    color_ramp Star = Ramp(ART_RGB(70, 30, 120), ART_RGB(130, 70, 210),
                           ART_RGB(190, 140, 255), ART_RGB(240, 220, 255));
    color_ramp Gold = Ramp(ART_RGB(110, 70, 10), ART_RGB(180, 130, 30),
                           ART_RGB(236, 196, 80), ART_RGB(255, 244, 180));
    color_ramp Fang = Ramp(ART_RGB(30, 20, 30), ART_RGB(70, 50, 60),
                           ART_RGB(150, 110, 70), ART_RGB(230, 190, 110));
    u32 Silk = ART_RGB(214, 204, 250);
    u32 Band = ART_RGB(120, 70, 190);

    float Bob = 0.f;
    float Gait = 0.f;
    // NOTE(zoubir): 0 standing, 1 reared on her back legs with the front
    // legs high; below 0 the front legs strike the ground
    float Rear = 0.f;
    float Blaze = 0.f;
    float FangOpen = 0.f;
    float Spray = -1.f;
    float Sprawl = 0.f;
    float Shift = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Gait = Pose.Wave;
            Bob = -1.2f * Absolute(Pose.Wave2);
        } break;

        case AnimationType_Cast:
        {
            Rear = Minimum(1.f, 1.4f * Pose.t);
            Blaze = Pose.t;
            FangOpen = 0.5f * Pose.t;
            Bob = 0.5f * Pose.Wave;
        } break;

        case AnimationType_Attack:
        {
            float Strike = Minimum(1.f, 3.f * Pose.t);
            Rear = 1.f - 1.25f * Strike;
            Blaze = 1.f - 0.6f * Pose.t;
            FangOpen = 1.f;
            Spray = Pose.t;
            Bob = 1.5f * Strike;
        } break;

        case AnimationType_Stop:
        {
            Rear = -0.25f * (1.f - Pose.t);
            Sprawl = 3.f * (1.f - Pose.t);
            Bob = 2.f * (1.f - Pose.t);
            FangOpen = 0.4f * (1.f - Pose.t);
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Shift = Pose.Wave2;
        } break;
    }

    float Ground = 63.f;
    float BodyY = 42.f + Bob;
    v2 Thorax = V2(41.f - 2.f * Rear, BodyY - 2.f - 9.f * Rear);
    v2 Abdomen = V2(21.f + 2.f * Rear, BodyY - 7.f + 6.f * Rear);

    // NOTE(zoubir): foot reach from the thorax; the front two pairs lift
    // to these spots when she rears
    float Reaches[4] = {23.f, 11.f, -8.f, -21.f};
    v2 Raised[2] = {V2(18.f, -20.f), V2(6.f, -27.f)};
    float RaiseShare = Maximum(0.f, Rear);

    // NOTE(zoubir): far legs first, darker; alternate legs swing opposite
    for(u32 Leg = 0; Leg < 4; Leg++)
    {
        float Phase = (Leg % 2) ? Gait : -Gait;
        float Step = 4.f * Phase + ((Leg % 2) ? 0.8f : -0.8f) * Shift;
        v2 Hip = Thorax + V2(-3.f + 1.5f * (3 - Leg) * 0.5f, -2.f);
        v2 Foot = V2(Thorax.X + Reaches[Leg] - 2.f + Step + Sprawl,
                     Ground - 3.f - 2.f * Maximum(0.f, Phase));
        if (Leg < 2)
        {
            v2 Up = Thorax + Raised[Leg] + V2(-6.f, 3.f);
            Foot = Lerp2(Foot, RaiseShare, Up);
            Foot.Y = Minimum(Foot.Y, Ground - 3.f) + 3.f * Minimum(0.f, Rear);
        }
        DrawMatronLeg(Canvas, Hip, Foot, 14.f - 2.f * Sprawl, Leg < 2 ? RaiseShare : 0.f,
                      Shell, Band, -0.4f);
    }

    // NOTE(zoubir): silk of starlight hanging from her belly and spinnerets
    for(u32 Thread = 0; Thread < 3; Thread++)
    {
        float Sway = 1.5f * Sin(2.f * Pi32 * Pose.t + 1.7f * Thread);
        v2 From = Abdomen + V2(-8.f + 7.f * Thread, 8.f);
        v2 To = V2(From.X + Sway, Minimum(Ground - 4.f, From.Y + 9.f + 3.f * (Thread % 2)));
        FillLimb(Canvas, From, To, 0.6f, 0.5f, Ramp(Silk, Silk, Silk, Silk));
        FillDot(Canvas, To.X, To.Y, 1.f, Gold.C[3]);
    }
    v2 Spinneret = Abdomen + V2(-13.f, 6.f);
    v2 Strand = Spinneret + V2(-2.f + Pose.Wave, 10.f);
    FillLimb(Canvas, Spinneret, Strand, 0.6f, 0.5f, Ramp(Silk, Silk, Silk, Silk));
    FillDot(Canvas, Strand.X, Strand.Y, 1.2f, Gold.C[3]);

    // NOTE(zoubir): the great abdomen with a star-map: violet stars joined
    // by thin lines, brighter as her stare gathers
    FillBlob(Canvas, Abdomen.X, Abdomen.Y, 16.f, 11.5f, Shell, 0.f);
    v2 Stars[7] = {V2(-0.62f, -0.15f), V2(-0.35f, -0.55f), V2(0.05f, -0.62f),
                   V2(0.45f, -0.35f), V2(0.2f, 0.05f), V2(-0.2f, 0.3f),
                   V2(0.55f, 0.3f)};
    u32 Lines[7][2] = {{0, 1}, {1, 2}, {2, 3}, {2, 4}, {4, 5}, {4, 6}, {5, 0}};
    v2 Map[7];
    for(u32 Index = 0; Index < 7; Index++)
    {
        Map[Index] = Abdomen + V2(13.f * Stars[Index].X, 9.f * Stars[Index].Y);
    }
    color_ramp LineRamp = Ramp(Star.C[0], Star.C[0], Star.C[1], Star.C[1]);
    for(u32 Index = 0; Index < 7; Index++)
    {
        FillLimb(Canvas, Map[Lines[Index][0]], Map[Lines[Index][1]], 0.5f, 0.5f, LineRamp);
    }
    for(u32 Index = 0; Index < 7; Index++)
    {
        float Twinkle = ((Pose.Frame + Index) % 3 == 0) ? 0.4f : 0.f;
        FillDot(Canvas, Map[Index].X, Map[Index].Y, 1.f + Twinkle + 0.5f * Blaze,
                Index % 3 ? Star.C[2] : Star.C[3]);
    }
    FillDot(Canvas, Map[2].X, Map[2].Y, 0.7f, Gold.C[3]);

    // NOTE(zoubir): the waist, then the cephalothorax and head
    FillLimb(Canvas, Abdomen + V2(12.f, 3.f), Thorax + V2(-6.f, 0.f), 4.f, 4.f, Shell, 0.1f);
    FillBlob(Canvas, Thorax.X, Thorax.Y, 10.f, 7.5f, Shell, 0.1f);
    FillDot(Canvas, Thorax.X - 2.f, Thorax.Y - 2.f, 1.2f, Star.C[1]);
    FillDot(Canvas, Thorax.X + 2.f, Thorax.Y - 3.f, 0.9f, Star.C[1]);

    // NOTE(zoubir): near legs over the body
    for(u32 Leg = 0; Leg < 4; Leg++)
    {
        float Phase = (Leg % 2) ? -Gait : Gait;
        float Step = 4.f * Phase + ((Leg % 2) ? -0.8f : 0.8f) * Shift;
        v2 Hip = Thorax + V2(-2.f + 1.5f * (3 - Leg) * 0.5f, 2.f);
        v2 Foot = V2(Thorax.X + Reaches[Leg] + Step + Sprawl,
                     Ground - 1.f - 2.f * Maximum(0.f, Phase));
        if (Leg < 2)
        {
            Foot = Lerp2(Foot, RaiseShare, Thorax + Raised[Leg]);
            Foot.Y = Minimum(Foot.Y, Ground - 1.f) + 2.f * Minimum(0.f, Rear);
        }
        DrawMatronLeg(Canvas, Hip, Foot, 12.f - 2.f * Sprawl, Leg < 2 ? RaiseShare : 0.f,
                      Shell, Band, 0.15f);
    }

    // NOTE(zoubir): head last, so no leg hides the cluster of gold eyes
    v2 Head = Thorax + V2(8.f, -1.f);
    FillBlob(Canvas, Head.X, Head.Y, 5.5f, 5.f, Shell, 0.2f);
    FillLimb(Canvas, Head + V2(3.f, 3.f), Head + V2(4.5f + FangOpen, 8.f + 1.5f * FangOpen),
             1.7f, 0.6f, Fang, 0.3f);
    FillLimb(Canvas, Head + V2(0.5f, 3.f), Head + V2(0.f - 1.5f * FangOpen, 8.f),
             1.5f, 0.5f, Fang, 0.f);
    // NOTE(zoubir): the eyes flare as the hush gathers
    if (Blaze > 0.2f)
    {
        FillDot(Canvas, Head.X + 2.f, Head.Y - 2.5f, 1.5f + 3.f * Blaze, Gold.C[1]);
        FillDot(Canvas, Head.X + 2.f, Head.Y - 2.5f, 1.f + 2.f * Blaze, Gold.C[2]);
    }
    float EyeGrow = 0.4f * Blaze;
    u32 EyeCore = Blaze > 0.5f ? ART_RGB(255, 255, 240) : Gold.C[3];
    FillDot(Canvas, Head.X + 4.f, Head.Y - 1.5f, 1.2f + EyeGrow, EyeCore);
    FillDot(Canvas, Head.X + 1.5f, Head.Y - 2.f, 1.2f + EyeGrow, EyeCore);
    FillDot(Canvas, Head.X + 3.f, Head.Y - 4.f, 0.8f + EyeGrow, Gold.C[2]);
    FillDot(Canvas, Head.X - 0.5f, Head.Y - 3.5f, 0.8f + EyeGrow, Gold.C[2]);
    FillDot(Canvas, Head.X + 5.f, Head.Y + 0.5f, 0.7f + EyeGrow, Gold.C[2]);

    // NOTE(zoubir): the venom spray fans forward and down from the fangs
    if (Spray >= 0.f)
    {
        v2 Mouth = Head + V2(4.f, 9.f);
        for(u32 Drop = 0; Drop < 6; Drop++)
        {
            float Angle = -0.2f + 0.25f * Drop;
            float Dist = 3.f + 10.f * Spray + 3.f * (Drop % 2);
            v2 P = Mouth + V2(Dist * Cos(Angle), Dist * Sin(Angle) * 0.6f + 2.f);
            P.X = Minimum(P.X, 68.f);
            P.Y = Minimum(P.Y, Ground);
            FillBlob(Canvas, P.X, P.Y, 2.4f - Spray, 2.f - Spray, Star, 0.5f);
        }
    }

    OutlineFrame(Canvas, ART_RGB(6, 4, 12));
}

#endif
