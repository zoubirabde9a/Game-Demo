/* Ithrel the Rimeheart: the Rimeheart Vault's last boss (sim/dungeon/,
   docs/dungeon-vault.md), on the Rimeheart Throne. A dead king in armour
   rimed white, a tattered cloak of frost, a crown of icicles on a bare
   skull and a heart of blue ice beating in the cage of his ribs; it is
   the cold that put out the forge above. Never roams (SpawnWeight 0).

   Calm:    Frost Nova bursts round him and slows everyone near; Rime
            Comet drops one great block of ice, slow to fall and hard to
            survive; Raise the Frozen Dead pulls Skeletal Thralls out of
            the floor, four at most. Soul Rime: he steps beside whoever
            holds him and freezes them, past any dodge.
   Enraged (below 30% health): faster, and Shatterstorm: shards of ice
            out in every direction round his target.
   The dungeon raises Bone Shamans at 80% and 45% that mend him, and binds
   a Rime Champion at 60% and 30% that erupts if not killed in time
   (sim/dungeon/boss_scripts.cpp). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Rimeheart)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Rimeheart(monster_def *Def)
{
    Def->Name = "Ithrel the Rimeheart";
    Def->MaxHp = 2000.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 720.f;
    Def->StopRange = 52.f;
    Def->AttackRange = 68.f;
    Def->AttackDamage = 17.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Attack] = 5;

    Def->EnrageHpShare = 0.3f;
    Def->EnrageSpeedScale = 1.3f;
    Def->EnrageCooldownScale = 0.65f;
    Def->EnrageTint = 0xFFFFF0D0;

    monster_ability *Storm = AddMonsterAbility(Def, MonsterAbility_Volley,
                                               "Shatterstorm");
    Storm->MaxRange = 520.f;
    Storm->Cooldown = 5.f;
    Storm->Windup = 0.7f;
    Storm->Active = 1.6f;
    Storm->Recover = 0.5f;
    Storm->Damage = 11.f;
    Storm->Radius = 20.f;
    Storm->Speed = 280.f;
    Storm->Knockback = 150.f;
    Storm->Count = MAX_ABILITY_POINTS;
    Storm->Spread = 270.f;
    Storm->ShotStyle = ShotStyle_Spine;
    Storm->Status = StatusEffect_Slowed;
    Storm->StatusSeconds = 2.f;
    Storm->PhaseMask = PHASE_ENRAGED;

    monster_ability *Nova = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Frost Nova");
    Nova->MaxRange = 95.f;
    Nova->Cooldown = 4.5f;
    Nova->Windup = 0.85f;
    Nova->Active = 0.3f;
    Nova->Recover = 0.6f;
    Nova->Damage = 19.f;
    Nova->Radius = 130.f;
    Nova->Knockback = 650.f;
    Nova->Status = StatusEffect_Slowed;
    Nova->StatusSeconds = 2.5f;

    // NOTE(zoubir): one big block on one player, with a long fall to see
    // it coming: whoever stands in it when it lands loses most of their
    // health
    monster_ability *Comet = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                               "Rime Comet");
    Comet->MinRange = 100.f;
    Comet->MaxRange = 600.f;
    Comet->Cooldown = 8.f;
    Comet->Windup = 1.4f;
    Comet->Active = 0.3f;
    Comet->Recover = 0.6f;
    Comet->Damage = 26.f;
    Comet->Radius = 100.f;
    Comet->Knockback = 500.f;
    Comet->Count = 1;
    Comet->Spread = 0.f;
    Comet->Status = StatusEffect_Slowed;
    Comet->StatusSeconds = 3.f;

    monster_ability *Raise = AddMonsterAbility(Def, MonsterAbility_Summon,
                                               "Raise the Frozen Dead");
    Raise->MaxRange = 600.f;
    Raise->Cooldown = 12.f;
    Raise->Windup = 1.f;
    Raise->Active = 0.3f;
    Raise->Recover = 0.5f;
    Raise->SummonKind = MonsterKind_Thrall;
    Raise->Count = 2;
    Raise->MaxActive = 4;
    Raise->Spread = 90.f;
    Raise->Radius = 14.f;

    monster_ability *Rime = AddMonsterAbility(Def, MonsterAbility_Smite,
                                              "Soul Rime");
    Rime->MaxRange = 560.f;
    Rime->Cooldown = 13.f;
    Rime->Windup = 1.1f;
    Rime->Active = 0.3f;
    Rime->Recover = 0.6f;
    Rime->Damage = 25.f;
    Rime->Radius = 24.f;
    Rime->Spread = 60.f;
    Rime->Knockback = 300.f;
    Rime->Status = StatusEffect_Slowed;
    Rime->StatusSeconds = 2.f;
}

#else

internal void
DrawMonster_Rimeheart(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Plate = Ramp(ART_RGB(40, 52, 70), ART_RGB(84, 100, 124),
                            ART_RGB(150, 170, 196), ART_RGB(220, 234, 248));
    color_ramp Cloak = Ramp(ART_RGB(14, 20, 40), ART_RGB(28, 40, 70),
                            ART_RGB(48, 66, 104), ART_RGB(76, 98, 140));
    color_ramp Bone = Ramp(ART_RGB(110, 110, 104), ART_RGB(164, 164, 154),
                           ART_RGB(208, 208, 196), ART_RGB(240, 240, 230));
    color_ramp Heart = Ramp(ART_RGB(20, 90, 170), ART_RGB(60, 170, 235),
                            ART_RGB(150, 230, 255), ART_RGB(240, 255, 255));
    color_ramp Ice = Ramp(ART_RGB(40, 80, 120), ART_RGB(90, 150, 200),
                          ART_RGB(160, 210, 240), ART_RGB(230, 250, 255));

    float Bob = 0.f;
    float Step = 0.f;
    // NOTE(zoubir): the sceptre's angle from straight up, forward positive
    float Swing = 0.4f;
    // NOTE(zoubir): the far hand, 0 at his side, 1 raised over his head
    float Lift = 0.f;
    float Glow = 0.5f;
    float Billow = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 2.5f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
            Billow = 2.f + Pose.Wave2;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): the far hand rises, the heart flares, the
            // cloak lifts in the cold
            Lift = Pose.t;
            Glow = 0.5f + 0.5f * Pose.t;
            Swing = 0.4f - 0.6f * Pose.t;
            Bob = -1.5f * Pose.t;
            Billow = 3.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Swing = -0.2f + 2.4f * Minimum(1.f, 2.f * Pose.t);
            Lift = 1.f - Pose.t;
            Glow = 1.f;
            Bob = 2.f * Pose.t;
            Billow = 3.f;
        } break;

        case AnimationType_Stop:
        {
            Swing = 1.8f;
            Bob = 2.f + 0.5f * Pose.Wave;
            Glow = 0.3f;
        } break;

        default:
        {
            Bob = 0.6f * Pose.Wave;
            Glow = 0.45f + 0.2f * Pose.Wave2;
            Billow = Pose.Wave2;
        } break;
    }

    float Ground = 56.f;
    float HipY = 39.f + Bob;
    float BodyY = HipY - 11.f;

    // NOTE(zoubir): the cloak behind everything, its hem torn
    FillTriangle(Canvas, V2(27.f, BodyY - 8.f), V2(13.f - Billow, Ground - 1.f),
                 V2(33.f, Ground - 3.f), Cloak, 0.4f, -0.2f);
    for(u32 Tear = 0; Tear < 4; Tear++)
    {
        float X = 15.f - Billow + 5.f * Tear;
        FillTriangle(Canvas, V2(X - 2.f, Ground - 3.f), V2(X + 2.f, Ground - 3.f),
                     V2(X, Ground + 0.5f - (Tear % 2)), Cloak, 0.f, -0.3f);
    }

    // NOTE(zoubir): long legs in rimed plate, the far one darker
    FillLimb(Canvas, V2(29.f, HipY), V2(27.f - Step, Ground - 2.f), 3.5f, 3.f, Plate, -0.4f);
    FillBlob(Canvas, 27.5f - Step, Ground - 1.f, 4.f, 2.f, Plate, -0.3f);
    FillLimb(Canvas, V2(35.f, HipY), V2(37.f + Step, Ground - 2.f), 3.5f, 3.f, Plate, 0.1f);
    FillBlob(Canvas, 37.5f + Step, Ground - 1.f, 4.f, 2.f, Plate, 0.1f);

    // NOTE(zoubir): the far arm, raised as he casts
    v2 FarShoulder = V2(27.f, BodyY - 5.f);
    v2 FarHand = FarShoulder + V2(-2.f + 4.f * Lift, 11.f - 24.f * Lift);
    FillLimb(Canvas, FarShoulder, FarHand, 3.f, 2.5f, Plate, -0.4f);
    if (Lift > 0.3f)
    {
        FillBlob(Canvas, FarHand.X, FarHand.Y - 2.f, 2.f + 2.f * Lift, 2.f + 2.f * Lift, Heart, 0.4f);
    }

    // NOTE(zoubir): a breastplate open over the ribs, and the heart of ice
    // beating inside
    FillBlob(Canvas, 32.f, BodyY, 9.f, 11.f, Plate, 0.05f);
    FillBlob(Canvas, 33.f, BodyY - 1.f, 5.5f, 6.f, Cloak, -0.2f);
    float Beat = 1.f + 0.6f * Glow + ((Pose.Frame % 2) ? 0.4f : 0.f);
    FillBlob(Canvas, 33.5f, BodyY - 1.f, 2.5f * Beat, 2.8f * Beat, Heart, 0.4f);
    for(u32 Rib = 0; Rib < 3; Rib++)
    {
        float Y = BodyY - 5.f + 3.5f * Rib;
        FillLimb(Canvas, V2(28.f, Y), V2(38.f, Y + 0.5f), 0.8f, 0.8f, Bone, 0.3f);
    }
    // NOTE(zoubir): pauldrons of ice
    FillBlob(Canvas, 26.f, BodyY - 8.f, 5.f, 3.5f, Ice, 0.2f);
    FillBlob(Canvas, 38.f, BodyY - 8.f, 5.f, 3.5f, Ice, 0.3f);
    FillTriangle(Canvas, V2(23.f, BodyY - 9.f), V2(27.f, BodyY - 10.f), V2(21.f, BodyY - 17.f), Ice, 0.2f, 1.f);
    FillTriangle(Canvas, V2(38.f, BodyY - 10.f), V2(42.f, BodyY - 9.f), V2(43.f, BodyY - 17.f), Ice, 0.2f, 1.f);

    // NOTE(zoubir): a bare skull, cold eyes, a crown of icicles
    float HeadX = 34.f;
    float HeadY = BodyY - 15.f;
    FillBlob(Canvas, HeadX, HeadY, 5.f, 5.5f, Bone, 0.25f);
    FillBlob(Canvas, HeadX + 1.5f, HeadY + 4.f, 3.f, 2.f, Bone, 0.f);
    FillDot(Canvas, HeadX + 2.5f, HeadY, 1.4f, Heart.C[2 + (Glow > 0.7f ? 1 : 0)]);
    FillDot(Canvas, HeadX - 1.f, HeadY, 1.2f, Heart.C[1]);
    for(u32 Spike = 0; Spike < 4; Spike++)
    {
        float X = HeadX - 4.5f + 3.f * Spike;
        float Tall = (Spike == 1 || Spike == 2) ? 9.f : 6.f;
        FillTriangle(Canvas, V2(X - 1.3f, HeadY - 4.f), V2(X + 1.3f, HeadY - 4.f),
                     V2(X, HeadY - 4.f - Tall), Ice, 0.3f, 1.f);
    }

    // NOTE(zoubir): the near arm and the sceptre, a shard of ice on a haft
    // of bone
    v2 Shoulder = V2(39.f, BodyY - 5.f);
    v2 Hand = Shoulder + V2(4.f, 9.f);
    FillLimb(Canvas, Shoulder, Hand, 3.f, 2.5f, Plate, 0.2f);
    v2 Up = V2(Sin(Swing), -Cos(Swing));
    v2 Butt = Hand - 8.f * Up;
    v2 Top = Hand + 16.f * Up;
    FillLimb(Canvas, Butt, Top, 1.2f, 1.2f, Bone, 0.1f);
    v2 Across = V2(-Up.Y, Up.X);
    FillTriangle(Canvas, Top - 3.f * Across, Top + 3.f * Across, Top + 9.f * Up, Ice, 0.2f, 1.f);
    if (Glow > 0.8f)
    {
        FillDot(Canvas, Top.X + 3.f * Up.X, Top.Y + 3.f * Up.Y, 2.f, Heart.C[3]);
    }

    OutlineFrame(Canvas, ART_RGB(8, 12, 22));
}

#endif
