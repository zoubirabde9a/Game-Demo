/* Collapsar: a husk of black stone in the Starless Deep
   (docs/dungeon-starless.md) curled round a shard of the dead star, so
   heavy the floor bends toward it. Gravity Well opens a well under its
   target (MonsterAbility_Pull) that drags everyone near it in, then
   collapses on whoever it caught: run against the pull, or dash out.
   Close up it brings its mass down round it. Only the deep's encounters
   and its bosses bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Collapsar)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Collapsar(monster_def *Def)
{
    Def->Name = "Collapsar";
    Def->MaxHp = 190.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 360.f;
    Def->StopRange = 44.f;
    Def->AttackRange = 56.f;
    Def->AttackDamage = 10.f;
    Def->AttackInterval = 1.1f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 56;

    // NOTE(zoubir): a well under the target, out to 230, for 2 s; someone
    // running flat out away from its middle still gets out of it
    monster_ability *Well = AddMonsterAbility(Def, MonsterAbility_Pull,
                                              "Gravity Well");
    Well->MinRange = 90.f;
    Well->MaxRange = 380.f;
    Well->Cooldown = 7.5f;
    Well->Windup = 0.9f;
    Well->Active = 2.f;
    Well->Recover = 0.8f;
    Well->Damage = 16.f;
    Well->Radius = 230.f;
    Well->InnerRadius = 70.f;
    Well->Speed = 1500.f;
    Well->Spread = 1000.f;
    Well->Knockback = 380.f;
    Well->Status = StatusEffect_Slowed;
    Well->StatusSeconds = 2.f;

    monster_ability *Mass = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Crushing Mass");
    Mass->MaxRange = 80.f;
    Mass->Cooldown = 4.5f;
    Mass->Windup = 0.8f;
    Mass->Active = 0.3f;
    Mass->Recover = 0.6f;
    Mass->Damage = 15.f;
    Mass->Radius = 95.f;
    Mass->Knockback = 420.f;
}

#else

internal void
DrawMonster_Collapsar(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Stone = Ramp(ART_RGB(16, 14, 22), ART_RGB(36, 32, 46),
                            ART_RGB(62, 56, 76), ART_RGB(98, 90, 116));
    color_ramp Core = Ramp(ART_RGB(60, 20, 90), ART_RGB(130, 60, 200),
                           ART_RGB(210, 150, 255), ART_RGB(255, 240, 210));
    color_ramp Gold = Ramp(ART_RGB(100, 60, 10), ART_RGB(170, 120, 30),
                           ART_RGB(230, 190, 70), ART_RGB(255, 240, 170));

    float Bob = 0.f;
    float Open = 0.25f + 0.1f * Pose.Wave2;
    float Step = 0.f;
    float Lift = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Open = 0.3f + 0.7f * Pose.t;
            Lift = Pose.t;
            Bob = -2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Open = 1.f;
            Lift = 1.f - 1.5f * Minimum(1.f, 3.f * Pose.t);
            Bob = 2.f * Minimum(1.f, 3.f * Pose.t);
        } break;

        case AnimationType_Stop:
        {
            Open = 0.5f * (1.f - Pose.t);
            Bob = 2.f;
        } break;

        default:
        {
            Bob = 0.5f * Pose.Wave;
        } break;
    }

    float Ground = 49.f;
    float CY = 31.f + Bob;
    // NOTE(zoubir): stubby legs of slab under the husk
    FillLimb(Canvas, V2(21.f, CY + 8.f), V2(20.f - Step, Ground - 2.f), 4.f, 4.5f, Stone, -0.3f);
    FillLimb(Canvas, V2(34.f, CY + 8.f), V2(35.f + Step, Ground - 2.f), 4.f, 4.5f, Stone, 0.f);

    // NOTE(zoubir): the plates of the husk part round the star shard
    float Part = 3.f + 5.f * Open;
    FillBlob(Canvas, 27.f - Part, CY, 10.f, 13.f, Stone, -0.2f);
    FillBlob(Canvas, 29.f + Part, CY - 1.f, 10.f, 13.f, Stone, 0.1f);
    FillBlob(Canvas, 28.f, CY - 11.f - 2.f * Open, 9.f, 6.f, Stone, 0.3f);
    FillBlob(Canvas, 28.f, CY, 4.f + 3.f * Open, 4.f + 3.f * Open, Core, 0.4f);
    FillDot(Canvas, 28.f, CY, 1.5f + 1.5f * Open, ART_RGB(255, 250, 230));
    // NOTE(zoubir): gold veins in the stone, and rubble caught circling it
    FillLimb(Canvas, V2(22.f - Part, CY - 6.f), V2(20.f - Part, CY + 7.f), 0.7f, 0.7f, Gold, 0.3f);
    FillLimb(Canvas, V2(35.f + Part, CY - 7.f), V2(37.f + Part, CY + 5.f), 0.7f, 0.7f, Gold, 0.3f);
    for(u32 Rock = 0; Rock < 4; Rock++)
    {
        float Angle = 2.f * Pi32 * ((float)Rock / 4.f + 0.25f * Pose.t);
        float RX = 28.f + (17.f - 3.f * Lift) * Cos(Angle);
        float RY = CY - 4.f + (7.f - 2.f * Lift) * Sin(Angle) - 6.f * Lift;
        FillBlob(Canvas, RX, RY, 2.f, 1.8f, Stone, 0.2f);
    }

    OutlineFrame(Canvas, ART_RGB(6, 4, 10));
}

#endif
