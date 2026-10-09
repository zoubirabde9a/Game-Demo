/* Nyxara the Black Sun: the Starless Deep's last boss (sim/dungeon/,
   docs/dungeon-starless.md), on the Throne of the Black Sun. What is left
   of the star: a woman of shadow floating inside a black disc ringed with
   gold fire, her hair a corona. Never roams (SpawnWeight 0).

   Eclipse: the light goes out but for two circles round the throne
            room (MonsterAbility_Eclipse); 3 s to reach one, and whoever
            is outside every circle when the dark falls is hit hard.
   Corona Flare: two rings of fire roll out through the hall
            (MonsterAbility_Wave), to be jumped.
   Void Brand: a brand on whoever stands farthest from her, which has to
            be carried away from the party; with an eclipse coming, it
            has to be carried into a light of its own.
   Gravity Well: a well under someone away from her.
   Sunset: a blow nobody dodges on whoever holds her.
   Enraged (below 30% health): faster, and all of it comes sooner.
   The dungeon brings Collapsars at 80%, Obsidian Knights at 60%, a Star
   Shard at 50% that erupts unless it is broken in 22 s, Void Seers at
   40% and frenzied Collapsars at 20%
   (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Nyxara)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Nyxara(monster_def *Def)
{
    Def->Name = "Nyxara the Black Sun";
    Def->MaxHp = 2600.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 760.f;
    Def->StopRange = 56.f;
    Def->AttackRange = 72.f;
    Def->AttackDamage = 24.f;
    Def->AttackInterval = 0.95f;
    Def->FlyHeight = 10.f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 80;

    Def->EnrageHpShare = 0.3f;
    Def->EnrageSpeedScale = 1.2f;
    Def->EnrageCooldownScale = 0.75f;
    Def->EnrageTint = 0xFFC8D8FF;

    monster_ability *Eclipse = AddMonsterAbility(Def, MonsterAbility_Eclipse,
                                                 "Eclipse");
    Eclipse->MaxRange = 700.f;
    Eclipse->Cooldown = 14.f;
    Eclipse->Windup = 3.f;
    Eclipse->Active = 0.6f;
    Eclipse->Recover = 0.8f;
    Eclipse->Damage = 52.f;
    Eclipse->Radius = 66.f;
    Eclipse->Spread = 420.f;
    Eclipse->Count = 2;

    // NOTE(zoubir): two rings 140 apart out to 620
    monster_ability *Flare = AddMonsterAbility(Def, MonsterAbility_Wave,
                                               "Corona Flare");
    Flare->MaxRange = 650.f;
    Flare->Cooldown = 9.f;
    Flare->Windup = 1.f;
    Flare->Active = 2.7f;
    Flare->Recover = 0.6f;
    Flare->Damage = 20.f;
    Flare->Radius = 620.f;
    Flare->Speed = 290.f;
    Flare->Knockback = 240.f;
    Flare->Count = 2;
    Flare->Spread = 140.f;
    Flare->Status = StatusEffect_Burning;
    Flare->StatusSeconds = 2.f;

    monster_ability *Brand = AddMonsterAbility(Def, MonsterAbility_Brand,
                                               "Void Brand");
    Brand->MaxRange = 680.f;
    Brand->Cooldown = 13.f;
    Brand->Windup = 3.2f;
    Brand->Active = 0.3f;
    Brand->Recover = 0.5f;
    Brand->Damage = 38.f;
    Brand->Radius = 140.f;
    Brand->Knockback = 300.f;

    monster_ability *Well = AddMonsterAbility(Def, MonsterAbility_Pull,
                                              "Gravity Well");
    Well->MinRange = 160.f;
    Well->MaxRange = 620.f;
    Well->Cooldown = 8.f;
    Well->Windup = 0.9f;
    Well->Active = 1.8f;
    Well->Recover = 0.5f;
    Well->Damage = 22.f;
    Well->Radius = 220.f;
    Well->InnerRadius = 70.f;
    Well->Speed = 1500.f;
    Well->Spread = 1000.f;
    Well->Knockback = 380.f;

    monster_ability *Sunset = AddMonsterAbility(Def, MonsterAbility_Smite,
                                                "Sunset");
    Sunset->MaxRange = 560.f;
    Sunset->Cooldown = 12.f;
    Sunset->Windup = 1.f;
    Sunset->Active = 0.3f;
    Sunset->Recover = 0.6f;
    Sunset->Damage = 46.f;
    Sunset->Radius = 26.f;
    Sunset->Spread = 60.f;
    Sunset->Knockback = 320.f;
}

#else

internal void
DrawMonster_Nyxara(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Shadow = Ramp(ART_RGB(6, 4, 12), ART_RGB(20, 14, 34),
                             ART_RGB(44, 32, 70), ART_RGB(84, 64, 120));
    color_ramp Corona = Ramp(ART_RGB(150, 70, 10), ART_RGB(230, 140, 30),
                             ART_RGB(255, 210, 90), ART_RGB(255, 250, 210));

    float Bob = 1.5f * Pose.Wave;
    float Flare = 0.4f + 0.2f * Pose.Wave2;
    float Arms = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Cast:
        {
            Flare = 0.4f + 0.6f * Pose.t;
            Arms = Pose.t;
            Bob = -2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Flare = 1.f;
            Arms = 1.f - 0.5f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            Flare = 0.6f;
            Arms = 0.3f * (1.f - Pose.t);
        } break;

        case AnimationType_Move:
        {
            Bob = 2.f * Pose.Wave;
        } break;

        default: break;
    }

    float CX = 40.f;
    float CY = 34.f + Bob;
    // NOTE(zoubir): the corona: flames of gold round a black disc
    u32 Flames = 14;
    for(u32 Flame = 0; Flame < Flames; Flame++)
    {
        float Angle = 2.f * Pi32 * ((float)Flame / (float)Flames + 0.05f * Pose.t);
        float Long = 21.f + 5.f * Flare + 3.f * (float)((Flame * 7) % 3);
        v2 Out = V2(Cos(Angle), Sin(Angle));
        v2 Side = V2(-Out.Y, Out.X);
        v2 Root = V2(CX, CY) + 16.f * Out;
        FillTriangle(Canvas, Root + 3.f * Side, Root - 3.f * Side, V2(CX, CY) + Long * Out,
                     Corona, 0.6f, 0.1f);
    }
    FillBlob(Canvas, CX, CY, 17.f, 17.f, Corona, 0.5f);
    FillBlob(Canvas, CX, CY, 15.f, 15.f, Shadow, -0.4f);

    // NOTE(zoubir): she floats inside it in violet robes that trail into
    // the dark, lighter than the disc so she reads against it
    color_ramp Robe = Ramp(ART_RGB(44, 26, 80), ART_RGB(86, 54, 140),
                           ART_RGB(136, 96, 200), ART_RGB(196, 160, 246));
    color_ramp Face = Ramp(ART_RGB(150, 130, 190), ART_RGB(200, 186, 230),
                           ART_RGB(236, 228, 250), ART_RGB(255, 252, 255));
    float BodyY = CY + 3.f;
    FillTriangle(Canvas, V2(CX - 8.f, BodyY - 2.f), V2(CX + 8.f, BodyY - 2.f),
                 V2(CX + 2.f * Pose.Wave2, BodyY + 24.f), Robe, 0.2f, -0.4f);
    FillBlob(Canvas, CX, BodyY - 1.f, 6.f, 9.f, Robe, 0.2f);
    FillLimb(Canvas, V2(CX - 5.f, BodyY + 6.f), V2(CX + 5.f, BodyY + 6.f), 0.8f, 0.8f, Corona, 0.4f);
    // NOTE(zoubir): arms raised to the sun as she casts
    v2 LeftHand = V2(CX - 8.f - 5.f * Arms, BodyY + 4.f - 15.f * Arms);
    v2 RightHand = V2(CX + 8.f + 5.f * Arms, BodyY + 4.f - 15.f * Arms);
    FillLimb(Canvas, V2(CX - 5.f, BodyY - 5.f), LeftHand, 2.f, 1.4f, Robe, 0.1f);
    FillLimb(Canvas, V2(CX + 5.f, BodyY - 5.f), RightHand, 2.f, 1.4f, Robe, 0.3f);
    FillDot(Canvas, LeftHand.X, LeftHand.Y, 1.5f + Arms, ART_RGB(255, 220, 120));
    FillDot(Canvas, RightHand.X, RightHand.Y, 1.5f + Arms, ART_RGB(255, 220, 120));
    // NOTE(zoubir): hair streaming up as fire behind a pale face, eyes of
    // violet light
    for(u32 Strand = 0; Strand < 5; Strand++)
    {
        float X = CX - 5.f + 2.5f * Strand;
        FillTriangle(Canvas, V2(X - 1.5f, BodyY - 13.f), V2(X + 1.5f, BodyY - 13.f),
                     V2(X + Pose.Wave * (float)(Strand % 2), BodyY - 24.f - 4.f * Flare),
                     Corona, 0.6f, 0.9f);
    }
    FillBlob(Canvas, CX + 1.f, BodyY - 12.f, 4.5f, 5.f, Face, 0.3f);
    FillDot(Canvas, CX + 2.5f, BodyY - 12.5f, 0.9f, ART_RGB(170, 90, 255));
    FillDot(Canvas, CX - 0.5f, BodyY - 12.5f, 0.7f, ART_RGB(170, 90, 255));

    OutlineFrame(Canvas, ART_RGB(4, 2, 8));
}

#endif
