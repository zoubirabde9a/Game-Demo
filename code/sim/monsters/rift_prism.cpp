/* The Prism Warden: the Aurora Rift's second boss (sim/dungeon/,
   docs/dungeon-rift.md), in the Prism Sanctum. The heart of the fallen
   aurora: a great crystal turning in the air inside a crown of floating
   shards, ribbons of green and violet light trailing from it. Never
   roams (SpawnWeight 0).

   Calm:    Aurora Lance sweeps a beam across most of the hall; it stops
            at pillars, so the party lives through it in their shadow.
            Prism Shards fans five shards, Refraction folds it through the
            light to just behind its target, Starfall drops four stars of
            ice where the party is heading. Lightbrand burns whoever holds
            it, past any dodge.
   Enraged (below 30% health): faster, the lance sooner.
   At 70% and 35% it raises Aurora Pylons round the hall, two then three, and takes
   nothing while one stands (sim/dungeon/boss_wards.cpp); at 85% and 50%
   Aurora Wisps drift out of the light, and fly back into it if left
   alone (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Prism)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Prism(monster_def *Def)
{
    Def->Name = "The Prism Warden";
    Def->MaxHp = 1300.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 720.f;
    Def->StopRange = 60.f;
    Def->AttackRange = 76.f;
    Def->AttackDamage = 15.f;
    Def->AttackInterval = 1.f;
    Def->FlyHeight = 18.f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;

    Def->EnrageHpShare = 0.3f;
    Def->EnrageSpeedScale = 1.25f;
    Def->EnrageCooldownScale = 0.65f;
    Def->EnrageTint = 0xFFF0D0FF;

    // NOTE(zoubir): a 150 degree sweep across the hall in 2 s
    monster_ability *Lance = AddMonsterAbility(Def, MonsterAbility_Beam,
                                               "Aurora Lance");
    Lance->MaxRange = 640.f;
    Lance->Cooldown = 6.f;
    Lance->Windup = 1.f;
    Lance->Active = 2.f;
    Lance->Recover = 0.6f;
    Lance->Damage = 24.f;
    Lance->Radius = 18.f;
    Lance->Speed = 720.f;
    Lance->Spread = 150.f;
    Lance->Knockback = 120.f;

    monster_ability *Shards = AddMonsterAbility(Def, MonsterAbility_Volley,
                                                "Prism Shards");
    Shards->MaxRange = 520.f;
    Shards->Cooldown = 3.f;
    Shards->Windup = 0.6f;
    Shards->Active = 1.5f;
    Shards->Recover = 0.4f;
    Shards->Damage = 12.f;
    Shards->Radius = 16.f;
    Shards->Speed = 330.f;
    Shards->Knockback = 150.f;
    Shards->Count = 5;
    Shards->Spread = 60.f;
    Shards->ShotStyle = ShotStyle_Spine;

    monster_ability *Fold = AddMonsterAbility(Def, MonsterAbility_Blink,
                                              "Refraction");
    Fold->MinRange = 160.f;
    Fold->MaxRange = 560.f;
    Fold->Cooldown = 7.f;
    Fold->Windup = 0.85f;
    Fold->Active = 0.3f;
    Fold->Recover = 0.6f;
    Fold->Damage = 18.f;
    Fold->Radius = 120.f;
    Fold->Spread = 100.f;
    Fold->Knockback = 450.f;

    monster_ability *Starfall = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                                  "Starfall");
    Starfall->MaxRange = 600.f;
    Starfall->Cooldown = 6.f;
    Starfall->Windup = 1.1f;
    Starfall->Active = 0.3f;
    Starfall->Recover = 0.5f;
    Starfall->Damage = 15.f;
    Starfall->Radius = 60.f;
    Starfall->Knockback = 300.f;
    Starfall->Count = 4;
    Starfall->Spread = 160.f;

    monster_ability *Brand = AddMonsterAbility(Def, MonsterAbility_Smite,
                                               "Lightbrand");
    Brand->MaxRange = 560.f;
    Brand->Cooldown = 11.f;
    Brand->Windup = 1.f;
    Brand->Active = 0.3f;
    Brand->Recover = 0.6f;
    Brand->Damage = 28.f;
    Brand->Radius = 24.f;
    Brand->Knockback = 250.f;
}

#else

internal void
DrawMonster_Prism(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Crystal = Ramp(ART_RGB(40, 84, 130), ART_RGB(90, 170, 214),
                              ART_RGB(160, 228, 246), ART_RGB(240, 254, 255));
    color_ramp Green = Ramp(ART_RGB(20, 100, 80), ART_RGB(40, 180, 130),
                            ART_RGB(120, 245, 185), ART_RGB(220, 255, 236));
    color_ramp Violet = Ramp(ART_RGB(64, 32, 116), ART_RGB(124, 74, 196),
                             ART_RGB(186, 140, 246), ART_RGB(244, 228, 255));
    color_ramp Star = Ramp(ART_RGB(200, 230, 250), ART_RGB(230, 246, 255),
                           ART_RGB(248, 254, 255), ART_RGB(255, 255, 255));

    float Spin = Pose.t;
    float Hover = 1.5f * Pose.Wave;
    float Flare = 0.f;
    float Spread = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Cast:
        {
            Flare = Pose.t;
            Spread = 4.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Flare = 1.f;
            Spread = 4.f - 3.f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            Flare = 0.f;
            Hover = 2.f;
            Spread = -1.f;
        } break;

        default:
        {
        } break;
    }

    float CX = 32.f;
    float CY = 28.f + Hover;

    // NOTE(zoubir): ribbons of aurora trailing below
    for(u32 Ribbon = 0; Ribbon < 4; Ribbon++)
    {
        float X = CX - 9.f + 6.f * Ribbon;
        float Sway = 3.f * Sin(2.f * Pi32 * (Pose.t + 0.25f * Ribbon));
        FillLimb(Canvas, V2(X, CY + 8.f), V2(X + Sway, CY + 26.f - 3.f * (Ribbon % 2)),
                 2.2f, 0.6f, (Ribbon % 2) ? Violet : Green, 0.2f);
    }

    // NOTE(zoubir): the crown of shards behind, then the crystal: two
    // facets that turn, the lit one swapping sides as it spins
    for(u32 Shard = 0; Shard < 6; Shard++)
    {
        float Angle = 2.f * Pi32 * ((float)Shard / 6.f + 0.5f * Spin);
        float SX = CX + (17.f + Spread) * Cos(Angle);
        float SY = CY - 2.f + (7.f + 0.5f * Spread) * Sin(Angle);
        if (Sin(Angle) < 0.f)
        {
            FillTriangle(Canvas, V2(SX - 2.f, SY + 3.f), V2(SX + 2.f, SY + 3.f),
                         V2(SX, SY - 5.f), Crystal, 0.2f, 0.8f);
        }
    }
    float Turn = Cos(2.f * Pi32 * Spin);
    float Left = 9.f + 2.f * Turn;
    float Right = 9.f - 2.f * Turn;
    FillTriangle(Canvas, V2(CX - Left, CY), V2(CX, CY - 20.f), V2(CX, CY + 16.f), Crystal, 0.1f, 0.6f);
    FillTriangle(Canvas, V2(CX + Right, CY), V2(CX, CY - 20.f), V2(CX, CY + 16.f), Crystal, 0.4f, 1.f);
    FillBlob(Canvas, CX, CY - 1.f, 4.f + 3.f * Flare, 4.f + 3.f * Flare, Violet, 0.4f);
    FillBlob(Canvas, CX, CY - 1.f, 2.f + 2.f * Flare, 2.f + 2.f * Flare, Star, 0.6f);
    for(u32 Shard = 0; Shard < 6; Shard++)
    {
        float Angle = 2.f * Pi32 * ((float)Shard / 6.f + 0.5f * Spin);
        float SX = CX + (17.f + Spread) * Cos(Angle);
        float SY = CY - 2.f + (7.f + 0.5f * Spread) * Sin(Angle);
        if (Sin(Angle) >= 0.f)
        {
            FillTriangle(Canvas, V2(SX - 2.5f, SY + 3.f), V2(SX + 2.5f, SY + 3.f),
                         V2(SX, SY - 6.f), Crystal, 0.4f, 1.f);
        }
    }
    if (Flare > 0.7f)
    {
        FillDot(Canvas, CX, CY - 21.f, 1.5f, Green.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(10, 16, 34));
}

#endif
