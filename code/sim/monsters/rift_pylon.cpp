/* Aurora Pylon: a spire of aurora crystal that the rift's bosses raise
   out of the floor (sim/dungeon/boss_scripts.cpp, docs/dungeon-rift.md).
   While one stands, the boss that raised it takes nothing
   (sim/dungeon/boss_wards.cpp), so the party has to turn from the boss
   and break them. It never moves; Pylon Lance sweeps a long beam across
   whoever comes for it, so the party breaks it from behind a pillar or
   from the side the sweep has passed. Only boss scripts raise it
   (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(AuroraPylon)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_AuroraPylon(monster_def *Def)
{
    Def->Name = "Aurora Pylon";
    Def->MaxHp = 130.f;
    Def->Acceleration = 0.f;
    Def->AggroRange = 900.f;
    Def->StopRange = 1.f;
    Def->AttackRange = 0.f;
    Def->AttackDamage = 0.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 64;

    monster_ability *Lance = AddMonsterAbility(Def, MonsterAbility_Beam,
                                               "Pylon Lance");
    Lance->MaxRange = 760.f;
    Lance->Cooldown = 6.f;
    Lance->Windup = 1.1f;
    Lance->Active = 2.f;
    Lance->Recover = 0.6f;
    Lance->Damage = 12.f;
    Lance->Radius = 14.f;
    Lance->Speed = 720.f;
    Lance->Spread = 100.f;
    Lance->Knockback = 80.f;
}

#else

internal void
DrawMonster_AuroraPylon(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Crystal = Ramp(ART_RGB(40, 80, 120), ART_RGB(80, 160, 200),
                              ART_RGB(150, 220, 240), ART_RGB(232, 252, 255));
    color_ramp Light = Ramp(ART_RGB(30, 120, 100), ART_RGB(60, 200, 150),
                            ART_RGB(150, 250, 200), ART_RGB(230, 255, 240));
    color_ramp Violet = Ramp(ART_RGB(70, 40, 120), ART_RGB(130, 80, 200),
                             ART_RGB(190, 150, 250), ART_RGB(245, 230, 255));
    color_ramp Rock = Ramp(ART_RGB(30, 34, 48), ART_RGB(56, 62, 82),
                           ART_RGB(86, 94, 118), ART_RGB(120, 128, 152));

    float Glow = 0.4f + 0.2f * Pose.Wave;
    switch(Pose.Anim)
    {
        case AnimationType_Cast: Glow = 0.5f + 0.5f * Pose.t; break;
        case AnimationType_Attack: Glow = 1.f; break;
        case AnimationType_Stop: Glow = 0.25f; break;
        default: break;
    }

    float Ground = 56.f;
    // NOTE(zoubir): a cracked base of rock
    FillBlob(Canvas, 32.f, Ground - 3.f, 13.f, 4.f, Rock, -0.1f);
    FillTriangle(Canvas, V2(20.f, Ground - 3.f), V2(26.f, Ground - 6.f), V2(22.f, Ground - 12.f), Crystal, 0.1f, 0.8f);
    FillTriangle(Canvas, V2(38.f, Ground - 6.f), V2(44.f, Ground - 3.f), V2(43.f, Ground - 11.f), Crystal, 0.1f, 0.8f);

    // NOTE(zoubir): the spire, two crystals leaning together
    FillTriangle(Canvas, V2(25.f, Ground - 4.f), V2(37.f, Ground - 4.f), V2(30.f, 6.f), Crystal, 0.1f, 1.f);
    FillTriangle(Canvas, V2(31.f, Ground - 5.f), V2(40.f, Ground - 4.f), V2(36.f, 16.f), Crystal, 0.3f, 0.9f);

    // NOTE(zoubir): the light caught inside, climbing as it charges
    float Core = 30.f - 14.f * Glow;
    FillLimb(Canvas, V2(31.f, Ground - 10.f), V2(31.f, Core), 2.f + Glow, 1.f, Light, 0.4f);
    FillBlob(Canvas, 31.f, Core, 2.5f + 2.5f * Glow, 2.5f + 2.5f * Glow, Violet, 0.4f);
    if (Glow > 0.8f)
    {
        FillDot(Canvas, 31.f, Core, 2.f, Light.C[3]);
        FillDot(Canvas, 30.5f, 8.f, 1.5f, Light.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(10, 16, 30));
}

#endif
