/* Aurora Wisp: a mote of the fallen aurora that drifts over the Aurora
   Rift (docs/dungeon-rift.md), a cold star in a veil of green and violet
   light. Aurora Ray holds a thin beam on its target, then sweeps it
   across them (MonsterAbility_Beam): a jump does not clear it, so the
   way out is to step ahead of the sweep or behind a pillar. Fragile, so
   the striker burns it first. Only the rift's encounters and its bosses
   bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Wisp)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Wisp(monster_def *Def)
{
    Def->Name = "Aurora Wisp";
    Def->MaxHp = 60.f;
    Def->Acceleration = 34000.f;
    Def->AggroRange = 440.f;
    Def->StopRange = 200.f;
    Def->AttackRange = 36.f;
    Def->AttackDamage = 5.f;
    Def->AttackInterval = 0.8f;
    Def->FlyHeight = 22.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 40;

    monster_ability *Ray = AddMonsterAbility(Def, MonsterAbility_Beam,
                                             "Aurora Ray");
    Ray->MaxRange = 300.f;
    Ray->Cooldown = 5.5f;
    Ray->Windup = 0.9f;
    Ray->Active = 1.2f;
    Ray->Recover = 0.6f;
    Ray->Damage = 14.f;
    Ray->Radius = 12.f;
    Ray->Speed = 320.f;
    Ray->Spread = 70.f;
    Ray->Knockback = 60.f;
}

#else

internal void
DrawMonster_Wisp(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Veil = Ramp(ART_RGB(20, 90, 80), ART_RGB(40, 170, 130),
                           ART_RGB(110, 240, 180), ART_RGB(210, 255, 230));
    color_ramp Violet = Ramp(ART_RGB(60, 30, 110), ART_RGB(120, 70, 190),
                             ART_RGB(180, 130, 240), ART_RGB(240, 220, 255));
    color_ramp Star = Ramp(ART_RGB(150, 210, 240), ART_RGB(200, 240, 255),
                           ART_RGB(236, 252, 255), ART_RGB(255, 255, 255));

    float Pulse = 0.5f + 0.5f * Pose.Wave;
    float Sway = 2.f * Pose.Wave2;
    float Flare = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Cast:
        {
            Flare = Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Flare = 1.f;
            Pulse = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Flare = 0.f;
            Pulse = 0.2f;
        } break;

        default:
        {
        } break;
    }

    float CX = 20.f;
    float CY = 18.f;

    // NOTE(zoubir): ribbons of aurora trailing below and behind it
    for(u32 Ribbon = 0; Ribbon < 3; Ribbon++)
    {
        float X = CX - 6.f + 4.f * Ribbon + Sway * (Ribbon == 1 ? -1.f : 1.f);
        color_ramp Color = (Ribbon == 1) ? Violet : Veil;
        FillLimb(Canvas, V2(X + 2.f, CY + 2.f), V2(X - 2.f + Sway, CY + 15.f - 2.f * Ribbon),
                 2.f, 0.6f, Color, 0.2f);
    }

    // NOTE(zoubir): the veil round the star, flaring as the ray builds
    float Size = 7.f + 1.5f * Pulse + 3.f * Flare;
    FillBlob(Canvas, CX, CY, Size, Size * 0.9f, Veil, 0.1f);
    FillBlob(Canvas, CX + 1.f, CY - 1.f, 0.6f * Size, 0.55f * Size, Violet, 0.3f);
    FillBlob(Canvas, CX + 1.5f, CY - 1.f, 2.5f + 1.5f * Flare, 2.5f + 1.5f * Flare, Star, 0.6f);
    // NOTE(zoubir): four points of the star
    float Spike = 4.f + 5.f * Flare;
    FillTriangle(Canvas, V2(CX + 0.5f, CY - 2.f), V2(CX + 2.5f, CY - 2.f), V2(CX + 1.5f, CY - 2.f - Spike), Star, 0.8f, 1.f);
    FillTriangle(Canvas, V2(CX + 0.5f, CY), V2(CX + 2.5f, CY), V2(CX + 1.5f, CY + Spike), Star, 0.8f, 1.f);
    FillTriangle(Canvas, V2(CX + 1.5f, CY - 2.f), V2(CX + 1.5f, CY), V2(CX + 1.5f + Spike, CY - 1.f), Star, 0.8f, 1.f);
    FillTriangle(Canvas, V2(CX + 1.5f, CY - 2.f), V2(CX + 1.5f, CY), V2(CX + 1.5f - Spike, CY - 1.f), Star, 0.8f, 1.f);

    OutlineFrame(Canvas, ART_RGB(8, 24, 30));
}

#endif
