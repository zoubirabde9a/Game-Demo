/* Void Seer: a hooded thing of the Starless Deep (docs/dungeon-starless.md)
   floating over the floor, its face a hole full of stars. Void Brand
   marks whoever stands farthest from it (MonsterAbility_Brand): when the
   brand bursts it hits them and everyone near them, so the branded runs
   from the party. Umbral Bolts throws three bolts of dark at its target.
   Fragile. Only the deep's encounters and its bosses bring it
   (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Seer)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Seer(monster_def *Def)
{
    Def->Name = "Void Seer";
    Def->MaxHp = 85.f;
    Def->Acceleration = 30000.f;
    Def->AggroRange = 380.f;
    Def->StopRange = 230.f;
    Def->AttackRange = 36.f;
    Def->AttackDamage = 6.f;
    Def->AttackInterval = 0.9f;
    Def->FlyHeight = 14.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 44;

    // NOTE(zoubir): 2.6 s to carry the brand out of the group
    monster_ability *Brand = AddMonsterAbility(Def, MonsterAbility_Brand,
                                               "Void Brand");
    Brand->MaxRange = 520.f;
    Brand->Cooldown = 9.f;
    Brand->Windup = 2.6f;
    Brand->Active = 0.3f;
    Brand->Recover = 0.6f;
    Brand->Damage = 12.f;
    Brand->Radius = 120.f;
    Brand->Knockback = 260.f;

    monster_ability *Bolts = AddMonsterAbility(Def, MonsterAbility_Volley,
                                               "Umbral Bolts");
    Bolts->MaxRange = 420.f;
    Bolts->Cooldown = 3.2f;
    Bolts->Windup = 0.7f;
    Bolts->Active = 1.3f;
    Bolts->Recover = 0.5f;
    Bolts->Damage = 7.f;
    Bolts->Radius = 13.f;
    Bolts->Speed = 330.f;
    Bolts->Count = 3;
    Bolts->Spread = 22.f;
    Bolts->Knockback = 120.f;
}

#else

internal void
DrawMonster_Seer(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Robe = Ramp(ART_RGB(14, 10, 26), ART_RGB(34, 24, 58),
                           ART_RGB(62, 44, 100), ART_RGB(104, 80, 150));
    color_ramp Trim = Ramp(ART_RGB(110, 70, 20), ART_RGB(180, 130, 40),
                           ART_RGB(230, 190, 80), ART_RGB(255, 240, 170));
    color_ramp Void = Ramp(ART_RGB(4, 2, 10), ART_RGB(12, 8, 26),
                           ART_RGB(26, 18, 50), ART_RGB(50, 36, 90));

    float Bob = 1.5f * Pose.Wave;
    float Reach = 0.f;
    float Glow = 0.3f + 0.2f * Pose.Wave2;
    switch(Pose.Anim)
    {
        case AnimationType_Cast:
        {
            Reach = Pose.t;
            Glow = 0.4f + 0.6f * Pose.t;
            Bob = -2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Reach = 1.2f;
            Glow = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Reach = 0.3f * (1.f - Pose.t);
            Bob = 1.f;
        } break;

        case AnimationType_Move:
        {
            Bob = 2.f * Pose.Wave;
        } break;

        default: break;
    }

    float Top = 12.f + Bob;
    // NOTE(zoubir): the robe tapers to rags over nothing
    FillTriangle(Canvas, V2(15.f, Top + 8.f), V2(31.f, Top + 8.f),
                 V2(22.f + Pose.Wave2, Top + 27.f), Robe, 0.f, -0.3f);
    for(u32 Rag = 0; Rag < 4; Rag++)
    {
        float X = 16.f + 4.f * Rag + 1.5f * Pose.Wave;
        FillTriangle(Canvas, V2(X, Top + 20.f), V2(X + 3.f, Top + 20.f),
                     V2(X + 1.f + Pose.Wave2, Top + 28.f + (Rag % 2) * 2.f), Robe, -0.1f, -0.4f);
    }
    FillBlob(Canvas, 23.f, Top + 12.f, 8.f, 9.f, Robe, 0.f);
    // NOTE(zoubir): a gold hem and a band down the front
    FillLimb(Canvas, V2(17.f, Top + 19.f), V2(29.f, Top + 19.f), 1.f, 1.f, Trim, 0.2f);
    FillLimb(Canvas, V2(25.f, Top + 6.f), V2(25.f, Top + 19.f), 0.8f, 0.8f, Trim, 0.3f);

    // NOTE(zoubir): the hood, and the stars inside it
    FillBlob(Canvas, 25.f, Top + 1.f, 6.5f, 6.5f, Robe, 0.2f);
    FillBlob(Canvas, 27.f, Top + 2.f, 4.f, 4.5f, Void, -0.3f);
    FillDot(Canvas, 26.f, Top + 1.f, 0.6f, ART_RGB(250, 240, 200));
    FillDot(Canvas, 28.5f, Top + 3.f, 0.6f, ART_RGB(220, 200, 255));
    FillDot(Canvas, 27.f, Top + 4.5f, 0.5f, ART_RGB(250, 250, 255));

    // NOTE(zoubir): the hand reaches out with the brand in it
    v2 Shoulder = V2(28.f, Top + 8.f);
    v2 Hand = Shoulder + V2(5.f + 5.f * Reach, 4.f - 6.f * Reach);
    FillLimb(Canvas, Shoulder, Hand, 2.5f, 1.8f, Robe, 0.1f);
    FillBlob(Canvas, Hand.X + 1.f, Hand.Y, 2.f + 1.5f * Glow, 2.f + 1.5f * Glow, Void, 0.3f);
    FillDot(Canvas, Hand.X + 1.f, Hand.Y, 1.f + Glow, ART_RGB(180, 120, 255));
    FillDot(Canvas, Hand.X + 1.f, Hand.Y, 0.6f + 0.5f * Glow, ART_RGB(255, 230, 140));

    OutlineFrame(Canvas, ART_RGB(6, 4, 12));
}

#endif
