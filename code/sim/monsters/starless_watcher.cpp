/* Lidless Watcher: a floating eye the size of a shield from the Starless
   Deep (docs/dungeon-starless.md), black stone lids round a violet iris
   and a pupil of dead-star gold, optic nerves trailing under it. Unblinking
   Stare (MonsterAbility_Gaze) opens the eye wide: when it is fully open
   everyone near it who is still moving is hit, so stop and stand still
   before the stare lands. Lidless Ray sweeps a beam across its target
   from range; get behind it or close in. Only the Starless Deep's
   encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Watcher)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Watcher(monster_def *Def)
{
    Def->Name = "Lidless Watcher";
    Def->MaxHp = 110.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 440.f;
    Def->StopRange = 200.f;
    Def->AttackRange = 40.f;
    Def->AttackDamage = 6.f;
    Def->AttackInterval = 0.9f;
    Def->FlyHeight = 18.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 56;

    // NOTE(zoubir): 1.5 s to let go of the keys; anyone in 380 still moving
    // faster than 60 when the eye opens is hit
    monster_ability *Stare = AddMonsterAbility(Def, MonsterAbility_Gaze,
                                               "Unblinking Stare");
    Stare->MaxRange = 380.f;
    Stare->Cooldown = 8.5f;
    Stare->Windup = 1.5f;
    Stare->Active = 0.6f;
    Stare->Recover = 0.6f;
    Stare->Damage = 15.f;
    Stare->Radius = 380.f;
    Stare->Speed = 60.f;

    monster_ability *Ray = AddMonsterAbility(Def, MonsterAbility_Beam,
                                             "Lidless Ray");
    Ray->MinRange = 80.f;
    Ray->MaxRange = 340.f;
    Ray->Cooldown = 6.f;
    Ray->Windup = 0.9f;
    Ray->Active = 1.2f;
    Ray->Recover = 0.7f;
    Ray->Damage = 13.f;
    Ray->Radius = 16.f;
    Ray->Speed = 340.f;
    Ray->Spread = 70.f;
    Ray->Knockback = 60.f;
}

#else

internal void
DrawMonster_Watcher(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Stone = Ramp(ART_RGB(14, 12, 20), ART_RGB(34, 30, 44),
                            ART_RGB(60, 54, 74), ART_RGB(96, 88, 112));
    color_ramp Nerve = Ramp(ART_RGB(30, 12, 44), ART_RGB(64, 30, 92),
                            ART_RGB(110, 60, 150), ART_RGB(170, 120, 210));
    color_ramp White = Ramp(ART_RGB(90, 80, 104), ART_RGB(150, 140, 162),
                            ART_RGB(200, 192, 206), ART_RGB(236, 230, 236));
    color_ramp Iris = Ramp(ART_RGB(40, 10, 70), ART_RGB(90, 30, 150),
                           ART_RGB(150, 80, 220), ART_RGB(210, 160, 255));
    color_ramp Gold = Ramp(ART_RGB(100, 60, 10), ART_RGB(170, 120, 30),
                           ART_RGB(230, 190, 70), ART_RGB(255, 240, 170));

    float Bob = 1.5f * Pose.Wave;
    float Open = 0.45f + 0.05f * Pose.Wave2;
    float Glow = 0.2f;
    float Flash = 0.f;
    float Trail = 0.f;
    float Splay = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Trail = 1.f;
            Open = 0.55f;
            Bob = 2.f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): the lids drag open and the pupil kindles
            Open = 0.45f + 0.6f * Pose.t;
            Glow = 0.2f + 0.8f * Pose.t;
            Splay = Pose.t;
            Bob = -2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Open = 1.1f;
            Glow = 1.f;
            Flash = 1.f - Pose.t;
            Splay = 1.f;
            Bob = -2.f;
        } break;

        case AnimationType_Stop:
        {
            // NOTE(zoubir): a blink, shut at the middle, then half open
            Open = (Pose.t < 0.5f) ? 1.f - 2.f * Pose.t : 0.9f * (Pose.t - 0.5f);
            Glow = 0.6f * (1.f - Pose.t);
            Splay = 0.5f * (1.f - Pose.t);
            Bob = -1.f + Pose.t;
        } break;

        default: break;
    }

    float CX = 28.f;
    float CY = 25.f + Bob;

    // NOTE(zoubir): optic nerves hang below like a jellyfish's tendrils,
    // swaying, trailing behind when it drifts, spreading when it stares
    for(u32 Strand = 0; Strand < 5; Strand++)
    {
        float Side = (float)Strand - 2.f;
        float Phase = 2.f * Pi32 * (Pose.t + 0.17f * (float)Strand);
        float Sway = 1.5f * Sin(Phase);
        float Length = 18.f - 3.f * Absolute(Side) - 4.f * Splay;
        v2 Root = V2(CX + 3.5f * Side, CY + 8.f);
        v2 Mid = Root + V2(Side * (1.f + 2.f * Splay) + Sway - 4.f * Trail,
                           0.5f * Length);
        v2 Tip = Mid + V2(Side * (1.f + 2.f * Splay) - 1.5f * Sway - 5.f * Trail,
                          0.5f * Length - 2.f * Trail);
        FillLimb(Canvas, Root, Mid, 1.8f, 1.2f, Nerve, 0.1f);
        FillLimb(Canvas, Mid, Tip, 1.2f, 0.7f, Nerve, 0.f);
        FillDot(Canvas, Tip.X, Tip.Y, 0.8f + 0.4f * Glow, Gold.C[1 + (Strand % 2)]);
    }

    // NOTE(zoubir): the stone husk the eye sits in
    FillBlob(Canvas, CX - 1.f, CY, 15.f, 13.f, Stone, -0.2f);

    // NOTE(zoubir): the eye looks right, the way it faces
    float EX = CX + 2.f;
    float EY = CY;
    FillBlob(Canvas, EX, EY, 10.f, 9.f, White, 0.1f);
    float IrisR = 5.5f + 0.8f * Glow;
    FillBlob(Canvas, EX + 2.f, EY, IrisR, IrisR, Iris, 0.2f + 0.3f * Glow);
    FillDot(Canvas, EX + 2.f, EY, IrisR - 1.5f, Iris.C[1 + (Glow > 0.6f ? 1 : 0)]);
    FillDot(Canvas, EX + 2.f, EY, 1.6f + 1.2f * Glow, Gold.C[2]);
    FillDot(Canvas, EX + 2.f, EY, 0.7f + 0.9f * Glow, ART_RGB(255, 246, 200));
    FillDot(Canvas, EX - 1.f, EY - 3.f, 0.8f, ART_RGB(250, 250, 255));

    // NOTE(zoubir): the lids, thick slabs of stone that slide over the eye
    float Gap = 8.f * Open;
    FillBlob(Canvas, EX, EY - Gap - 4.f, 12.f, 5.f, Stone, 0.3f);
    FillBlob(Canvas, EX, EY + 0.6f * Gap + 5.f, 11.f, 5.f, Stone, -0.1f);
    // NOTE(zoubir): stone spines along the lid edges
    for(u32 Spine = 0; Spine < 3; Spine++)
    {
        float X = EX - 6.f + 6.f * (float)Spine;
        float Top = EY - Gap - 8.f + (Spine == 1 ? -1.f : 0.f);
        FillTriangle(Canvas, V2(X - 2.f, Top + 2.f), V2(X + 2.f, Top + 2.f),
                     V2(X - 1.f, Top - 3.f), Stone, 0.4f, -0.1f);
    }
    // NOTE(zoubir): cracks of gold in the stone
    FillLimb(Canvas, V2(CX - 13.f, CY - 3.f), V2(CX - 10.f, CY + 4.f), 0.6f, 0.6f, Gold, 0.2f);
    FillLimb(Canvas, V2(EX - 4.f, EY - Gap - 5.f), V2(EX + 2.f, EY - Gap - 6.f), 0.6f, 0.6f, Gold, 0.2f);
    FillLimb(Canvas, V2(EX + 3.f, EY + 0.6f * Gap + 7.f), V2(EX + 7.f, EY + 0.6f * Gap + 5.f),
             0.6f, 0.6f, Gold, 0.1f);

    // NOTE(zoubir): the stare lands: a flash of light out of the pupil
    if (Flash > 0.f)
    {
        v2 Pupil = V2(EX + 2.f, EY);
        for(u32 Ray = 0; Ray < 6; Ray++)
        {
            float Angle = 2.f * Pi32 * ((float)Ray / 6.f + 0.08f);
            float Reach = 5.f + 8.f * Flash;
            FillLimb(Canvas, Pupil + V2(3.f * Cos(Angle), 3.f * Sin(Angle)),
                     Pupil + V2(Reach * Cos(Angle), Reach * Sin(Angle)),
                     0.9f, 0.3f, Gold, 0.5f);
        }
        FillDot(Canvas, Pupil.X, Pupil.Y, 2.f + 2.f * Flash, ART_RGB(255, 250, 220));
    }

    OutlineFrame(Canvas, ART_RGB(6, 4, 10));
}

#endif
