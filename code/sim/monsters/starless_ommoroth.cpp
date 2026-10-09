/* Ommoroth the Hungering Dark: the Starless Deep's first boss (sim/dungeon/,
   docs/dungeon-starless.md), in the Maw. A mountain of black stone with
   a mouth for a body, rubble orbiting it, the dead star's light deep in
   its throat. Never roams (SpawnWeight 0).

   Event Horizon: the whole Maw bends toward him for 2.6 s
            (MonsterAbility_Pull at his own feet, out to 560), then the
            pull collapses on whoever is still within 150 of him. The
            party runs outward the whole time, the tank included.
   Gravity Well: a smaller well under someone farther off.
   Crushing Gravity: a slam round him. Rubble Rain: three stones on the
            back line. Devour: a bite nobody dodges on whoever holds him.
   Enraged (below 30% health): faster, and the horizon comes sooner.
   The dungeon brings Collapsars at 75% and 25%, which walk back into him
   if left alone, and Void Seers at 50% (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Ommoroth)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Ommoroth(monster_def *Def)
{
    Def->Name = "Ommoroth the Hungering Dark";
    Def->MaxHp = 2300.f;
    Def->Acceleration = 20000.f;
    Def->AggroRange = 720.f;
    Def->StopRange = 60.f;
    Def->AttackRange = 76.f;
    Def->AttackDamage = 22.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 80;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.3f;
    Def->EnrageSpeedScale = 1.25f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFFE0C8FF;

    // NOTE(zoubir): a player running straight out at full speed loses
    // about a third of their pace to the pull, so one who starts at his
    // feet when the windup ends is out of the collapse with time to spare,
    // and one who stands still is dragged in from anywhere in the Maw
    monster_ability *Horizon = AddMonsterAbility(Def, MonsterAbility_Pull,
                                                 "Event Horizon");
    Horizon->MaxRange = 650.f;
    Horizon->Cooldown = 9.f;
    Horizon->Windup = 1.3f;
    Horizon->Active = 2.6f;
    Horizon->Recover = 0.9f;
    Horizon->Damage = 44.f;
    Horizon->Radius = 560.f;
    Horizon->InnerRadius = 150.f;
    Horizon->Speed = 1600.f;
    Horizon->Spread = 0.f;
    Horizon->Knockback = 600.f;

    monster_ability *Crush = AddMonsterAbility(Def, MonsterAbility_Slam,
                                               "Crushing Gravity");
    Crush->MaxRange = 100.f;
    Crush->Cooldown = 4.f;
    Crush->Windup = 0.8f;
    Crush->Active = 0.3f;
    Crush->Recover = 0.6f;
    Crush->Damage = 38.f;
    Crush->Radius = 120.f;
    Crush->Knockback = 560.f;
    Crush->Status = StatusEffect_Slowed;
    Crush->StatusSeconds = 2.f;

    monster_ability *Well = AddMonsterAbility(Def, MonsterAbility_Pull,
                                              "Gravity Well");
    Well->MinRange = 160.f;
    Well->MaxRange = 620.f;
    Well->Cooldown = 7.f;
    Well->Windup = 0.9f;
    Well->Active = 1.8f;
    Well->Recover = 0.5f;
    Well->Damage = 22.f;
    Well->Radius = 220.f;
    Well->InnerRadius = 70.f;
    Well->Speed = 1500.f;
    Well->Spread = 1000.f;
    Well->Knockback = 380.f;

    monster_ability *Rubble = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                                "Rubble Rain");
    Rubble->MinRange = 140.f;
    Rubble->MaxRange = 600.f;
    Rubble->Cooldown = 5.f;
    Rubble->Windup = 1.f;
    Rubble->Active = 0.3f;
    Rubble->Recover = 0.5f;
    Rubble->Damage = 22.f;
    Rubble->Radius = 64.f;
    Rubble->Knockback = 340.f;
    Rubble->Count = 3;
    Rubble->Spread = 150.f;

    monster_ability *Devour = AddMonsterAbility(Def, MonsterAbility_Smite,
                                                "Devour");
    Devour->MaxRange = 560.f;
    Devour->Cooldown = 11.f;
    Devour->Windup = 1.f;
    Devour->Active = 0.3f;
    Devour->Recover = 0.6f;
    Devour->Damage = 40.f;
    Devour->Radius = 26.f;
    Devour->Spread = 70.f;
    Devour->Knockback = 300.f;
}

#else

internal void
DrawMonster_Ommoroth(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Stone = Ramp(ART_RGB(14, 12, 20), ART_RGB(34, 30, 44),
                            ART_RGB(60, 54, 74), ART_RGB(100, 92, 118));
    color_ramp Maw = Ramp(ART_RGB(20, 4, 20), ART_RGB(60, 14, 50),
                          ART_RGB(110, 30, 80), ART_RGB(170, 60, 110));
    color_ramp Star = Ramp(ART_RGB(130, 70, 200), ART_RGB(210, 150, 255),
                           ART_RGB(255, 220, 160), ART_RGB(255, 255, 240));
    color_ramp Gold = Ramp(ART_RGB(100, 60, 10), ART_RGB(170, 120, 30),
                           ART_RGB(230, 190, 70), ART_RGB(255, 240, 170));

    float Bob = 0.f;
    // NOTE(zoubir): how wide the maw gapes, and how far the rubble swings
    float Gape = 0.3f + 0.1f * Pose.Wave2;
    float Orbit = 1.f;
    float Step = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Gape = 0.3f + 0.7f * Pose.t;
            Orbit = 1.f - 0.4f * Pose.t;
            Bob = -3.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Gape = 1.f - 0.6f * Pose.t;
            Orbit = 0.6f;
            Bob = 2.f * Minimum(1.f, 3.f * Pose.t);
        } break;

        case AnimationType_Stop:
        {
            Gape = 0.4f;
            Bob = 2.f;
        } break;

        default:
        {
            Bob = 0.8f * Pose.Wave;
        } break;
    }

    float Ground = 70.f;
    float CY = 42.f + Bob;
    // NOTE(zoubir): four squat legs of stacked slab
    FillLimb(Canvas, V2(26.f, CY + 12.f), V2(23.f - Step, Ground - 2.f), 6.f, 6.5f, Stone, -0.4f);
    FillLimb(Canvas, V2(50.f, CY + 12.f), V2(53.f + Step, Ground - 2.f), 6.f, 6.5f, Stone, 0.f);
    FillLimb(Canvas, V2(33.f, CY + 14.f), V2(32.f + Step, Ground - 1.f), 5.f, 5.5f, Stone, -0.2f);
    FillLimb(Canvas, V2(44.f, CY + 14.f), V2(45.f - Step, Ground - 1.f), 5.f, 5.5f, Stone, 0.1f);

    // NOTE(zoubir): the body is a ring of stone round a mouth
    FillBlob(Canvas, 38.f, CY, 22.f, 19.f, Stone, 0.f);
    FillBlob(Canvas, 42.f, CY - 1.f, 9.f + 6.f * Gape, 7.f + 7.f * Gape, Maw, -0.2f);
    for(u32 Tooth = 0; Tooth < 7; Tooth++)
    {
        float Angle = 2.f * Pi32 * (float)Tooth / 7.f;
        float RX = 8.f + 6.f * Gape;
        float RY = 6.f + 7.f * Gape;
        v2 Root = V2(42.f + RX * Cos(Angle), CY - 1.f + RY * Sin(Angle));
        v2 Tip = V2(42.f + 0.5f * RX * Cos(Angle), CY - 1.f + 0.5f * RY * Sin(Angle));
        FillTriangle(Canvas, Root + V2(-1.5f * Sin(Angle), 1.5f * Cos(Angle)),
                     Root - V2(-1.5f * Sin(Angle), 1.5f * Cos(Angle)), Tip, Gold, 0.2f, 0.6f);
    }
    // NOTE(zoubir): the dead star burning in the throat
    FillBlob(Canvas, 43.f, CY - 1.f, 2.f + 3.f * Gape, 2.f + 3.f * Gape, Star, 0.4f);
    FillDot(Canvas, 43.f, CY - 1.f, 1.f + 1.5f * Gape, ART_RGB(255, 255, 240));
    // NOTE(zoubir): crags along the crown, veined with gold
    for(u32 Crag = 0; Crag < 4; Crag++)
    {
        float X = 24.f + 8.f * Crag;
        FillTriangle(Canvas, V2(X - 3.f, CY - 15.f), V2(X + 3.f, CY - 15.f),
                     V2(X + 1.f, CY - 24.f - 2.f * (Crag % 2)), Stone, 0.4f, 0.8f);
    }
    FillLimb(Canvas, V2(20.f, CY - 6.f), V2(26.f, CY + 10.f), 0.8f, 0.8f, Gold, 0.3f);

    // NOTE(zoubir): rubble circling him, pulled in tight when he feeds
    for(u32 Rock = 0; Rock < 6; Rock++)
    {
        float Angle = 2.f * Pi32 * ((float)Rock / 6.f + 0.25f * Pose.t);
        float RX = 38.f + (30.f * Orbit) * Cos(Angle);
        float RY = CY - 6.f + (10.f * Orbit) * Sin(Angle);
        FillBlob(Canvas, RX, RY, 3.f - 0.3f * (Rock % 3), 2.5f, Stone, 0.3f);
    }

    OutlineFrame(Canvas, ART_RGB(6, 4, 10));
}

#endif
