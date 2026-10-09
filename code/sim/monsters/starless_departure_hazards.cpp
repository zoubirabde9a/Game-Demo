/* Departure hazards: what falls on the party while a Starless Deep boss
   is away from its fight (sim/dungeon/boss_departures.cpp,
   docs/dungeon-starless.md). The script puts one over every player in
   the room every few seconds; each winds up one Slam on the spot below
   it and is taken away after it lands, so whoever stands still is hit
   and whoever keeps moving is not.

   Void Maw:     Ommoroth's, while he is sunk into the dark under the
                 Maw: jaws of black stone close up out of the floor.
   Falling Star: Nyxara's, while she is up in the sky: a shard of her
                 burnt-out sun comes down.

   Both hang far over the room (FlyHeight past OUT_OF_SIGHT_HEIGHT), so
   nothing bumps into them and no blow reaches them; clients draw them
   with client/dungeon/boss_departure_fx.cpp from the ring on the floor
   and the windup, not from these sprites, which only show on the sheet.
   Only that script makes them (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(VoidMaw)
MONSTER(FallingStar)
#elif !defined(MONSTER_ART_PASS)

// NOTE(zoubir): the numbers both share: a ring the size of Ommoroth's
// Rubble Rain, a windup long enough to walk out of
internal monster_ability *
DefineDepartureHazard(monster_def *Def, char *Name, char *Ability)
{
    Def->Name = Name;
    Def->MaxHp = 50.f;
    Def->Acceleration = 0.f;
    Def->AggroRange = 2000.f;
    Def->StopRange = 1.f;
    Def->AttackRange = 0.f;
    Def->AttackDamage = 0.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FlyHeight = 700.f;
    Def->FrameSize = 48;

    monster_ability *Strike = AddMonsterAbility(Def, MonsterAbility_Slam, Ability);
    Strike->MaxRange = 2000.f;
    Strike->Cooldown = 30.f;
    Strike->Windup = 1.3f;
    Strike->Active = 0.2f;
    Strike->Recover = 0.4f;
    Strike->Damage = 10.f;
    Strike->Radius = 56.f;
    Strike->Knockback = 160.f;
    return Strike;
}

internal void
DefineMonster_VoidMaw(monster_def *Def)
{
    DefineDepartureHazard(Def, "Void Maw", "Maw from Below");
}

internal void
DefineMonster_FallingStar(monster_def *Def)
{
    DefineDepartureHazard(Def, "Falling Star", "Starfall");
}

#else

// NOTE(zoubir): how far each row has the hazard come: 0 not yet, 1 landed
inline float
DepartureHazardReach(monster_pose Pose)
{
    float Result = 0.15f + 0.05f * Pose.Wave;
    switch(Pose.Anim)
    {
        case AnimationType_Cast: Result = 0.2f + 0.8f * Pose.t; break;
        case AnimationType_Attack: Result = 1.f; break;
        case AnimationType_Stop: Result = 1.f - 0.6f * Pose.t; break;
        default: break;
    }
    return Result;
}

internal void
DrawMonster_VoidMaw(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Stone = Ramp(ART_RGB(12, 8, 22), ART_RGB(34, 24, 56),
                            ART_RGB(66, 50, 104), ART_RGB(120, 96, 170));
    color_ramp Throat = Ramp(ART_RGB(60, 10, 30), ART_RGB(140, 30, 60),
                             ART_RGB(230, 80, 110), ART_RGB(255, 190, 200));
    float Reach = DepartureHazardReach(Pose);
    float Ground = 42.f;
    // NOTE(zoubir): the hole in the floor, wider as the jaws rise
    FillFlatEllipse(Canvas, 24.f, Ground, 10.f + 8.f * Reach, 3.f + 2.f * Reach,
                    ART_RGB(6, 4, 12));
    FillBlob(Canvas, 24.f, Ground - 1.f, 4.f + 4.f * Reach, 1.5f + Reach, Throat, 0.3f);
    // NOTE(zoubir): fangs either side, closing over the middle as they land
    float Rise = 6.f + 22.f * Reach;
    float Close = 11.f - 8.f * Reach * Reach;
    for(u32 Fang = 0; Fang < 3; Fang++)
    {
        float Step = 4.f * (float)Fang;
        float Tall = Rise - 3.f * (float)Fang;
        FillTriangle(Canvas, V2(24.f - Close - Step - 4.f, Ground), V2(24.f - Close - Step + 2.f, Ground),
                     V2(24.f - Close * 0.5f - Step, Ground - Tall), Stone, 0.1f, 0.9f);
        FillTriangle(Canvas, V2(24.f + Close + Step - 2.f, Ground), V2(24.f + Close + Step + 4.f, Ground),
                     V2(24.f + Close * 0.5f + Step, Ground - Tall), Stone, 0.1f, 0.9f);
    }
    OutlineFrame(Canvas, ART_RGB(4, 2, 10));
}

internal void
DrawMonster_FallingStar(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Fire = Ramp(ART_RGB(150, 70, 10), ART_RGB(230, 140, 30),
                           ART_RGB(255, 214, 110), ART_RGB(255, 248, 220));
    color_ramp Husk = Ramp(ART_RGB(16, 10, 20), ART_RGB(40, 26, 40),
                           ART_RGB(80, 56, 60), ART_RGB(140, 110, 90));
    float Reach = DepartureHazardReach(Pose);
    float Ground = 42.f;
    // NOTE(zoubir): the shard comes down from the top of the frame, its
    // tail behind it
    float Y = 18.f + (Ground - 24.f) * Reach;
    if (Pose.Anim != AnimationType_Attack)
    {
        FillLimb(Canvas, V2(30.f, Y - 14.f), V2(24.f, Y), 1.f, 4.f, Fire, 0.4f);
        FillBlob(Canvas, 24.f, Y, 5.f, 5.f, Husk, -0.2f);
        FillDot(Canvas, 22.5f, Y - 1.5f, 1.5f, Fire.C[2]);
    }
    else
    {
        // NOTE(zoubir): the landing: a flash and the shard broken on the
        // floor
        FillFlatEllipse(Canvas, 24.f, Ground, 16.f + 3.f * Pose.t, 5.f, Fire.C[1]);
        FillBlob(Canvas, 24.f, Ground - 6.f, 9.f - 4.f * Pose.t, 8.f - 4.f * Pose.t, Fire, 0.5f);
        FillBlob(Canvas, 20.f, Ground - 2.f, 3.f, 2.f, Husk, 0.f);
        FillBlob(Canvas, 29.f, Ground - 2.f, 2.5f, 2.f, Husk, 0.f);
    }
    OutlineFrame(Canvas, ART_RGB(20, 10, 4));
}

#endif
