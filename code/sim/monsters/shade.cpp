/* Hollow Shade: a hooded wraith that drifts above the ground. Veil Step
   dissolves it while a mark glows behind you, then it reappears on the
   mark and rakes everything around it. Turn around and step away from
   the mark. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Shade)
#else

internal void
DefineMonster_Shade(monster_def *Def)
{
    Def->Name = "Hollow Shade";
    Def->MaxHp = 55.f;
    Def->Acceleration = 40000.f;
    Def->AggroRange = 450.f;
    Def->StopRange = 30.f;
    Def->AttackRange = 40.f;
    Def->AttackDamage = 7.f;
    Def->AttackInterval = 0.8f;
    Def->FlyHeight = 8.f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 48;

    monster_ability *Step = AddMonsterAbility(Def, MonsterAbility_Blink,
                                              "Veil Step");
    Step->MinRange = 60.f;
    Step->MaxRange = 300.f;
    Step->Cooldown = 4.f;
    Step->Windup = 0.6f;
    Step->Active = 0.2f;
    Step->Recover = 0.7f;
    Step->Damage = 16.f;
    // NOTE(zoubir): must reach past Spread, where the player stands
    Step->Radius = 62.f;
    Step->Spread = 50.f;
    Step->Knockback = 300.f;
}

internal void
DrawMonster_Shade(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Cloak = Ramp(ART_RGB(16, 14, 30), ART_RGB(34, 30, 60),
                            ART_RGB(56, 50, 92), ART_RGB(84, 78, 128));
    color_ramp Spirit = Ramp(ART_RGB(40, 110, 130), ART_RGB(70, 180, 200),
                             ART_RGB(130, 230, 240), ART_RGB(210, 255, 255));
    u32 Void = ART_RGB(6, 4, 12);
    u32 Eye = ART_RGB(150, 255, 255);

    float Sway = 1.5f * Pose.Wave;
    float Lean = 0.f;
    v2 Claw = V2(36.f, 30.f);
    float Fade = 0.f;
    bool32 Slash = false;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Lean = 2.f;
            Claw = V2(34.f, 32.f + Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): coming apart into the veil
            Fade = 0.85f * Pose.t;
            Claw = V2(30.f, 26.f);
        } break;

        case AnimationType_Attack:
        {
            Slash = true;
            Lean = 4.f;
            Claw = Lerp2(V2(30.f, 14.f), Pose.t, V2(42.f, 36.f));
            Fade = 0.5f * (1.f - Pose.t);
        } break;

        case AnimationType_Stop:
        {
            Lean = -1.f;
            Claw = V2(32.f, 36.f);
            Fade = (Pose.Frame % 2) ? 0.3f : 0.1f;
        } break;

        default:
        {
        } break;
    }

    float HoodX = 24.f + Lean + Sway * 0.3f;
    float HoodY = 16.f;

    // NOTE(zoubir): robe falling from the hood, hem torn into points that
    // ripple with the sway
    FillTriangle(Canvas, V2(HoodX + 1.f, HoodY - 2.f), V2(10.f + Sway, 40.f),
                 V2(36.f + Sway * 0.5f + Lean, 40.f), Cloak, 0.8f, 0.2f);
    FillBlob(Canvas, HoodX - 1.f, 30.f, 9.f, 10.f, Cloak);
    for(u32 Point = 0; Point < 5; Point++)
    {
        float X = 11.f + 6.f * Point + Sway * (1.f - 0.2f * Point);
        float Drop = 4.f + 3.f * (float)((Point + Pose.Frame) % 2);
        FillTriangle(Canvas, V2(X - 3.f, 38.f), V2(X + 0.5f, 40.f + Drop),
                     V2(X + 3.f, 38.f), Cloak, 0.5f, 0.15f);
    }

    // NOTE(zoubir): wisps rising off the cloak
    for(u32 Wisp = 0; Wisp < 3; Wisp++)
    {
        float Rise = Pose.t + 0.33f * Wisp;
        Rise -= (float)(i32)Rise;
        float X = 12.f + 9.f * Wisp + 2.f * Sin(2.f * Pi32 * Rise);
        float Y = 36.f - 26.f * Rise;
        FillDot(Canvas, X, Y, 1.5f * (1.f - Rise) + 0.3f, Spirit.C[1]);
    }

    // NOTE(zoubir): hood with an empty face and two cold eyes
    FillBlob(Canvas, HoodX, HoodY, 8.f, 9.f, Cloak, 0.1f);
    FillTriangle(Canvas, V2(HoodX - 6.f, HoodY - 2.f), V2(HoodX - 9.f, HoodY - 12.f),
                 V2(HoodX - 1.f, HoodY - 7.f), Cloak, 0.6f, 0.3f);
    FillBlob(Canvas, HoodX + 3.f, HoodY + 1.f, 4.f, 5.5f,
             Ramp(Void, Void, Void, Void));
    FillDot(Canvas, HoodX + 2.5f, HoodY, 1.f, Eye);
    FillDot(Canvas, HoodX + 5.5f, HoodY, 1.f, Eye);

    // NOTE(zoubir): spectral claw reaching out of the sleeve
    v2 Sleeve = V2(HoodX + 4.f, HoodY + 12.f);
    FillLimb(Canvas, Sleeve, Claw, 3.5f, 2.f, Cloak, 0.05f);
    for(u32 Finger = 0; Finger < 3; Finger++)
    {
        float Angle = -0.6f + 0.6f * Finger;
        v2 Tip = Claw + V2(6.f * Cos(Angle), 6.f * Sin(Angle));
        FillLimb(Canvas, Claw, Tip, 1.f, 0.4f, Spirit);
    }

    if (Slash)
    {
        // NOTE(zoubir): the rake, an arc trailing behind the claw
        for(u32 Spark = 0; Spark < 7; Spark++)
        {
            float Along = Pose.t - 0.08f * Spark;
            if (Along < 0.f)
            {
                continue;
            }
            v2 P = Lerp2(V2(30.f, 12.f), Along, V2(44.f, 38.f));
            P.X += 6.f * Sin(Pi32 * Along);
            FillDot(Canvas, P.X, P.Y, 2.f - 0.25f * Spark, Spirit.C[3 - Spark / 3]);
        }
    }

    OutlineFrame(Canvas, ART_RGB(4, 2, 10));
    if (Fade > 0.f)
    {
        DissolveFrame(Canvas, Fade);
    }
}

#endif
