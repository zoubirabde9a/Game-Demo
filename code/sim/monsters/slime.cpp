/* The slime family, two kinds in one file because they share a body.

   Gloomslime: a heavy blob with something dead floating inside. Belly
   Flop rises tall, then slams flat and leaves a slowing goo puddle.
   When it dies it splits into two Slimelets, so killing one in a crowd
   makes the crowd bigger.

   Slimelet: what a Gloomslime splits into. Small, quick, weak, no
   abilities; never spawns on its own (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Slime)
MONSTER(Slimelet)
#else

internal void
DefineMonster_Slime(monster_def *Def)
{
    Def->Name = "Gloomslime";
    Def->MaxHp = 90.f;
    Def->Acceleration = 20000.f;
    Def->AggroRange = 340.f;
    Def->StopRange = 36.f;
    Def->AttackRange = 46.f;
    Def->AttackDamage = 8.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 2;
    Def->FrameSize = 48;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.1f;

    monster_ability *Flop = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Belly Flop");
    Flop->MaxRange = 70.f;
    Flop->Cooldown = 5.f;
    Flop->Windup = 0.7f;
    Flop->Active = 0.3f;
    Flop->Recover = 0.8f;
    Flop->Damage = 14.f;
    Flop->Radius = 58.f;
    Flop->Knockback = 250.f;
    Flop->Status = StatusEffect_Slowed;
    Flop->StatusSeconds = 1.f;
    Flop->HazardSeconds = 4.f;
    Flop->HazardStyle = HazardStyle_Goo;

    Def->DeathEffect = DeathEffect_Split;
    Def->SplitKind = MonsterKind_Slimelet;
    Def->SplitCount = 2;
}

internal void
DefineMonster_Slimelet(monster_def *Def)
{
    Def->Name = "Slimelet";
    Def->MaxHp = 22.f;
    Def->Acceleration = 36000.f;
    Def->AggroRange = 380.f;
    Def->StopRange = 24.f;
    Def->AttackRange = 32.f;
    Def->AttackDamage = 4.f;
    Def->AttackInterval = 0.7f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 32;
    Def->SecondsPerFrame[MonsterRow_Move] = 0.07f;
}

// NOTE(zoubir): the shared body. Scale 1 fills a 48 pixel frame; Core
// says whether the drowned skull shows inside
internal void
DrawSlimeBody(sprite_canvas *Canvas, monster_pose Pose, float Scale, bool32 Core)
{
    color_ramp Goo = Ramp(ART_RGB(36, 30, 74), ART_RGB(62, 54, 120),
                          ART_RGB(96, 90, 168), ART_RGB(160, 170, 230));
    color_ramp Deep = Ramp(ART_RGB(26, 20, 52), ART_RGB(40, 34, 80),
                           ART_RGB(58, 50, 104), ART_RGB(80, 72, 130));
    color_ramp Bone = Ramp(ART_RGB(110, 104, 110), ART_RGB(170, 164, 160),
                           ART_RGB(210, 204, 196), ART_RGB(236, 232, 224));
    u32 EyeWhite = ART_RGB(236, 240, 255);
    u32 Pupil = ART_RGB(16, 10, 30);

    // NOTE(zoubir): squash and stretch drive everything: Wide > 1 is
    // flattened, < 1 is stretched up
    float Wide = 1.f;
    float Lift = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            // NOTE(zoubir): a hop: squash, launch tall, land squashed
            float Arc = Sin(Pi32 * Pose.t);
            Lift = 6.f * Arc;
            Wide = Arc < 0.3f ? 1.2f : 0.88f;
        } break;

        case AnimationType_Cast:
        {
            Wide = 1.f - 0.3f * Pose.t;
            Lift = 3.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Wide = 1.45f - 0.25f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            Wide = 1.1f + 0.08f * Pose.Wave;
        } break;

        default:
        {
            Wide = 1.f + 0.06f * Pose.Wave;
        } break;
    }

    float Ground = 42.f * Scale;
    float RX = 15.f * Scale * Wide;
    float RY = 13.f * Scale / Wide;
    float CX = 24.f * Scale;
    float CY = Ground - RY - Lift * Scale;

    // NOTE(zoubir): drips hanging off the sides
    FillLimb(Canvas, V2(CX - RX * 0.8f, CY + RY * 0.3f),
             V2(CX - RX * 0.82f, CY + RY * 0.95f), 2.f * Scale, 1.4f * Scale, Goo);
    FillBlob(Canvas, CX, CY, RX, RY, Goo);
    if (Core)
    {
        // NOTE(zoubir): a skull drowned in the goo, seen through it
        float SX = CX - 3.f * Scale;
        float SY = CY + 1.f * Scale;
        FillBlob(Canvas, SX, SY + 1.f * Scale, 8.f * Scale, 7.f * Scale, Deep);
        FillBlob(Canvas, SX, SY, 5.f * Scale, 4.5f * Scale, Bone, -0.3f);
        FillBlob(Canvas, SX, SY + 4.f * Scale, 3.2f * Scale, 2.f * Scale, Bone, -0.35f);
        FillDot(Canvas, SX - 2.f * Scale, SY, 1.5f * Scale, Deep.C[0]);
        FillDot(Canvas, SX + 2.f * Scale, SY, 1.5f * Scale, Deep.C[0]);
        FillDot(Canvas, SX, SY + 2.f * Scale, 0.6f * Scale, Deep.C[0]);
        for(u32 Tooth = 0; Tooth < 3; Tooth++)
        {
            PutPixel(Canvas, (i32)(SX - 1.5f * Scale + 1.5f * Scale * Tooth),
                     (i32)(SY + 4.5f * Scale), Deep.C[0]);
        }
    }
    // NOTE(zoubir): glossy highlight and eyes on the front
    FillFlatEllipse(Canvas, CX - RX * 0.35f, CY - RY * 0.55f, 3.f * Scale,
                    1.3f * Scale, Goo.C[3]);
    float EyeY = CY - RY * 0.15f;
    if (Pose.Anim == AnimationType_Stop)
    {
        // NOTE(zoubir): dizzy, eyes squeezed into lines
        FillLimb(Canvas, V2(CX + 4.f * Scale, EyeY), V2(CX + 7.f * Scale, EyeY),
                 0.6f * Scale, 0.6f * Scale, Ramp(Pupil, Pupil, Pupil, Pupil));
        FillLimb(Canvas, V2(CX + 9.f * Scale, EyeY), V2(CX + 12.f * Scale, EyeY),
                 0.6f * Scale, 0.6f * Scale, Ramp(Pupil, Pupil, Pupil, Pupil));
    }
    else
    {
        FillDot(Canvas, CX + 6.f * Scale, EyeY, 2.8f * Scale, EyeWhite);
        FillDot(Canvas, CX + 11.f * Scale, EyeY, 2.f * Scale, EyeWhite);
        FillDot(Canvas, CX + 7.f * Scale, EyeY + 0.5f * Scale, 1.1f * Scale, Pupil);
        FillDot(Canvas, CX + 11.8f * Scale, EyeY + 0.5f * Scale, 1.f * Scale, Pupil);
    }
    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): goo thrown out by the flop
        for(u32 Splash = 0; Splash < 4; Splash++)
        {
            float Side = (Splash % 2) ? 1.f : -1.f;
            float Out = (14.f + 8.f * Pose.t + 3.f * (float)(Splash / 2)) * Scale;
            float Up = (6.f - 10.f * Pose.t * Pose.t + 3.f * (float)(Splash / 2)) * Scale;
            FillBlob(Canvas, CX + Side * Out, Ground - 4.f * Scale - Up,
                     1.8f * Scale, 1.8f * Scale, Goo, 0.1f);
        }
    }
    OutlineFrame(Canvas, ART_RGB(12, 8, 26));
}

internal void
DrawMonster_Slime(sprite_canvas *Canvas, monster_pose Pose)
{
    DrawSlimeBody(Canvas, Pose, 1.f, true);
}

internal void
DrawMonster_Slimelet(sprite_canvas *Canvas, monster_pose Pose)
{
    DrawSlimeBody(Canvas, Pose, 32.f / 48.f, false);
}

#endif
