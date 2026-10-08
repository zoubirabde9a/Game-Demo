/* Rimeglass Sentinel: a slow golem of aurora crystal from the Aurora Rift
   (docs/dungeon-rift.md). Crystal Crush brings both fists down round it.
   When it dies it shatters: six shards fly out all the way round
   (DeathEffect_Shatter), so whoever lands the kill next to it eats one.
   The tank pulls it off the party before it falls; melee steps back for
   the last blow. Only the rift's encounters and its queen bring it
   (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Sentinel)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Sentinel(monster_def *Def)
{
    Def->Name = "Rimeglass Sentinel";
    Def->MaxHp = 175.f;
    Def->Acceleration = 19000.f;
    Def->AggroRange = 380.f;
    Def->StopRange = 42.f;
    Def->AttackRange = 54.f;
    Def->AttackDamage = 11.f;
    Def->AttackInterval = 1.2f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 56;

    monster_ability *Crush = AddMonsterAbility(Def, MonsterAbility_Slam,
                                               "Crystal Crush");
    Crush->MaxRange = 75.f;
    Crush->Cooldown = 5.f;
    Crush->Windup = 0.8f;
    Crush->Active = 0.25f;
    Crush->Recover = 0.8f;
    Crush->Damage = 20.f;
    Crush->Radius = 85.f;
    Crush->Knockback = 550.f;

    // NOTE(zoubir): never cast in a fight: the shards it bursts into
    monster_ability *Shatter = AddMonsterAbility(Def, MonsterAbility_Volley,
                                                 "Shatter");
    Shatter->Cooldown = 1.f;
    Shatter->Windup = 0.3f;
    Shatter->Active = 0.9f;
    Shatter->Damage = 14.f;
    Shatter->Radius = 16.f;
    Shatter->Speed = 260.f;
    Shatter->Knockback = 200.f;
    Shatter->Count = 6;
    Shatter->ShotStyle = ShotStyle_Spine;
    Shatter->Status = StatusEffect_Slowed;
    Shatter->StatusSeconds = 1.5f;
    Shatter->PhaseMask = PHASE_DEATH;

    Def->DeathEffect = DeathEffect_Shatter;
    Def->ShatterAbility = 1;
}

#else

internal void
DrawMonster_Sentinel(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Crystal = Ramp(ART_RGB(40, 92, 120), ART_RGB(86, 170, 196),
                              ART_RGB(156, 224, 236), ART_RGB(232, 252, 255));
    color_ramp Core = Ramp(ART_RGB(70, 30, 120), ART_RGB(130, 70, 200),
                           ART_RGB(190, 140, 250), ART_RGB(250, 230, 255));
    color_ramp Rock = Ramp(ART_RGB(30, 34, 48), ART_RGB(56, 62, 82),
                           ART_RGB(86, 94, 118), ART_RGB(120, 128, 152));

    float Bob = 0.f;
    float Step = 0.f;
    // NOTE(zoubir): fists, 0 at its sides, 1 raised over its head
    float Raise = 0.f;
    float Glow = 0.4f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.f * Pose.Wave;
            Bob = -1.2f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Raise = Pose.t;
            Glow = 0.4f + 0.6f * Pose.t;
            Bob = -1.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Raise = 1.f - 1.3f * Minimum(1.f, 2.f * Pose.t);
            Glow = 1.f;
            Bob = 2.f * Minimum(1.f, 2.f * Pose.t);
        } break;

        case AnimationType_Stop:
        {
            Raise = -0.25f;
            Bob = 2.f;
            Glow = 0.3f + 0.1f * Pose.Wave;
        } break;

        default:
        {
            Bob = 0.5f * Pose.Wave;
            Glow = 0.4f + 0.15f * Pose.Wave2;
        } break;
    }

    float Ground = 49.f;
    float HipY = 36.f + Bob;
    float BodyY = HipY - 9.f;

    // NOTE(zoubir): stubby legs of dark rock crusted with crystal
    FillLimb(Canvas, V2(23.f, HipY), V2(22.f - Step, Ground - 2.f), 4.5f, 4.f, Rock, -0.3f);
    FillBlob(Canvas, 22.f - Step, Ground - 1.f, 5.f, 2.f, Rock, -0.2f);
    FillLimb(Canvas, V2(33.f, HipY), V2(34.f + Step, Ground - 2.f), 4.5f, 4.f, Rock, 0.1f);
    FillBlob(Canvas, 34.f + Step, Ground - 1.f, 5.f, 2.f, Rock, 0.1f);

    // NOTE(zoubir): the far arm and its fist of crystal
    v2 FarShoulder = V2(19.f, BodyY - 4.f);
    v2 FarFist = FarShoulder + V2(-3.f + 2.f * Raise, 13.f - 24.f * Raise);
    FillLimb(Canvas, FarShoulder, FarFist, 3.5f, 3.f, Rock, -0.4f);
    FillBlob(Canvas, FarFist.X, FarFist.Y, 5.f, 5.f, Crystal, -0.2f);

    // NOTE(zoubir): a hulking body of rock, a cage of crystal over a
    // violet core
    FillBlob(Canvas, 28.f, BodyY, 12.f, 11.f, Rock, 0.f);
    FillBlob(Canvas, 29.f, BodyY - 1.f, 6.f * (0.8f + 0.3f * Glow), 6.f * (0.8f + 0.3f * Glow), Core, 0.3f);
    FillTriangle(Canvas, V2(18.f, BodyY - 6.f), V2(24.f, BodyY - 8.f), V2(19.f, BodyY - 22.f), Crystal, 0.2f, 1.f);
    FillTriangle(Canvas, V2(24.f, BodyY - 9.f), V2(30.f, BodyY - 10.f), V2(27.f, BodyY - 26.f), Crystal, 0.3f, 1.f);
    FillTriangle(Canvas, V2(31.f, BodyY - 9.f), V2(37.f, BodyY - 7.f), V2(36.f, BodyY - 20.f), Crystal, 0.2f, 1.f);
    FillTriangle(Canvas, V2(36.f, BodyY + 2.f), V2(40.f, BodyY - 4.f), V2(44.f, BodyY - 8.f), Crystal, 0.1f, 0.9f);
    FillTriangle(Canvas, V2(20.f, BodyY + 6.f), V2(26.f, BodyY + 8.f), V2(16.f, BodyY + 10.f), Crystal, 0.1f, 0.6f);

    // NOTE(zoubir): a small head sunk between the shoulders, two cold eyes
    FillBlob(Canvas, 34.f, BodyY - 8.f, 5.f, 4.f, Rock, 0.2f);
    FillDot(Canvas, 36.f, BodyY - 8.5f, 1.2f, Core.C[3]);
    FillDot(Canvas, 33.f, BodyY - 8.5f, 1.f, Core.C[2]);

    // NOTE(zoubir): the near arm and fist
    v2 Shoulder = V2(37.f, BodyY - 3.f);
    v2 Fist = Shoulder + V2(4.f - 2.f * Raise, 13.f - 24.f * Raise);
    FillLimb(Canvas, Shoulder, Fist, 4.f, 3.5f, Rock, 0.2f);
    FillBlob(Canvas, Fist.X, Fist.Y, 6.f, 6.f, Crystal, 0.3f);
    FillTriangle(Canvas, Fist + V2(-2.f, -4.f), Fist + V2(3.f, -4.f), Fist + V2(1.f, -10.f), Crystal, 0.3f, 1.f);
    if (Glow > 0.85f)
    {
        FillDot(Canvas, Fist.X, Fist.Y, 2.f, Core.C[3]);
        FillDot(Canvas, FarFist.X, FarFist.Y, 1.5f, Core.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(10, 14, 28));
}

#endif
