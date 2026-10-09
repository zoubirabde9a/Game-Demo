/* Obsidian Knight: a knight in armour of black glass from the Starless
   Deep (docs/dungeon-starless.md), with a shield polished to a mirror.
   Mirror Guard (MonsterAbility_Reflect): it plants the shield and for a
   few seconds every blow on it is turned back on whoever struck, so the
   party has to stop hitting it in time. Shield Rush charges along a
   locked line. Only the deep's encounters and its bosses bring it
   (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(ObsidianKnight)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_ObsidianKnight(monster_def *Def)
{
    Def->Name = "Obsidian Knight";
    Def->MaxHp = 160.f;
    Def->Acceleration = 26000.f;
    Def->AggroRange = 360.f;
    Def->StopRange = 40.f;
    Def->AttackRange = 54.f;
    Def->AttackDamage = 11.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 52;

    // NOTE(zoubir): 0.8 s to see the shield come up, then 2.4 s of
    // mirror; half of each blow comes back, at most 18
    monster_ability *Guard = AddMonsterAbility(Def, MonsterAbility_Reflect,
                                               "Mirror Guard");
    Guard->MaxRange = 400.f;
    Guard->Cooldown = 10.f;
    Guard->Windup = 0.8f;
    Guard->Active = 2.4f;
    Guard->Recover = 0.5f;
    Guard->Damage = 18.f;
    Guard->Radius = 30.f;
    Guard->Spread = 0.5f;

    monster_ability *Rush = AddMonsterAbility(Def, MonsterAbility_Charge,
                                              "Shield Rush");
    Rush->MinRange = 140.f;
    Rush->MaxRange = 380.f;
    Rush->Cooldown = 6.f;
    Rush->Windup = 0.7f;
    Rush->Active = 0.5f;
    Rush->Recover = 0.8f;
    Rush->Damage = 14.f;
    Rush->Radius = 34.f;
    Rush->Speed = 600.f;
    Rush->Knockback = 480.f;
}

#else

internal void
DrawMonster_ObsidianKnight(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Glass = Ramp(ART_RGB(10, 8, 18), ART_RGB(28, 24, 44),
                            ART_RGB(58, 52, 86), ART_RGB(130, 120, 170));
    color_ramp Mirror = Ramp(ART_RGB(90, 96, 120), ART_RGB(160, 168, 196),
                             ART_RGB(214, 220, 240), ART_RGB(255, 255, 255));
    color_ramp Gold = Ramp(ART_RGB(100, 60, 10), ART_RGB(170, 120, 30),
                           ART_RGB(230, 190, 70), ART_RGB(255, 240, 170));

    float Step = 0.f;
    float Bob = 0.f;
    // NOTE(zoubir): 0 the shield at its side, 1 planted in front
    float Guard = 0.f;
    float Lean = 0.f;
    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 3.5f * Pose.Wave;
            Bob = -1.f * Absolute(Pose.Wave);
        } break;

        case AnimationType_Cast:
        {
            Guard = Pose.t;
            Lean = 2.f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Guard = 1.f;
            Lean = 2.f;
            Bob = 0.5f * Pose.Wave;
        } break;

        case AnimationType_Stop:
        {
            Guard = 1.f - Pose.t;
            Lean = 1.f;
        } break;

        default:
        {
            Bob = 0.5f * Pose.Wave;
        } break;
    }

    float Ground = 45.f;
    float HipY = 33.f + Bob;
    FillLimb(Canvas, V2(22.f, HipY), V2(21.f - Step, Ground - 2.f), 3.5f, 3.f, Glass, -0.3f);
    FillLimb(Canvas, V2(29.f, HipY), V2(30.f + Step, Ground - 2.f), 3.5f, 3.f, Glass, 0.f);
    FillBlob(Canvas, 21.f - Step, Ground - 1.f, 3.5f, 1.8f, Glass, -0.2f);
    FillBlob(Canvas, 31.f + Step, Ground - 1.f, 3.5f, 1.8f, Glass, 0.f);

    // NOTE(zoubir): the sword arm behind, then a breastplate of black glass
    FillLimb(Canvas, V2(20.f + Lean, HipY - 11.f), V2(15.f, HipY - 2.f), 2.5f, 2.f, Glass, -0.3f);
    FillLimb(Canvas, V2(15.f, HipY - 2.f), V2(8.f, HipY - 14.f), 1.f, 0.7f, Mirror, 0.2f);
    FillBlob(Canvas, 26.f + Lean, HipY - 9.f, 8.f, 10.f, Glass, 0.f);
    FillLimb(Canvas, V2(20.f + Lean, HipY - 14.f), V2(32.f + Lean, HipY - 14.f), 1.f, 1.f, Gold, 0.3f);
    FillLimb(Canvas, V2(26.f + Lean, HipY - 17.f), V2(26.f + Lean, HipY - 1.f), 0.7f, 0.7f, Gold, 0.2f);

    // NOTE(zoubir): a great helm with a slit of violet light
    float HeadX = 27.f + 1.2f * Lean;
    float HeadY = HipY - 22.f;
    FillBlob(Canvas, HeadX, HeadY, 5.5f, 6.f, Glass, 0.2f);
    FillLimb(Canvas, V2(HeadX + 1.f, HeadY), V2(HeadX + 5.f, HeadY), 0.8f, 0.8f, Gold, 0.4f);
    FillDot(Canvas, HeadX + 3.5f, HeadY, 0.7f, ART_RGB(220, 170, 255));
    FillTriangle(Canvas, V2(HeadX - 2.f, HeadY - 5.f), V2(HeadX + 1.f, HeadY - 5.f),
                 V2(HeadX - 3.f, HeadY - 11.f), Gold, 0.3f, 0.6f);

    // NOTE(zoubir): the mirror shield, swung round in front to guard
    float ShieldX = 33.f + 8.f * Guard + Lean;
    float ShieldY = HipY - 8.f - 2.f * Guard;
    FillBlob(Canvas, ShieldX, ShieldY, 4.f + 2.f * Guard, 10.f, Gold, 0.f);
    FillBlob(Canvas, ShieldX + 0.5f, ShieldY, 3.f + 2.f * Guard, 8.5f, Mirror, 0.4f + 0.3f * Guard);
    if (Guard > 0.5f)
    {
        FillDot(Canvas, ShieldX + 1.f, ShieldY - 4.f, 1.2f, ART_RGB(255, 255, 255));
    }

    OutlineFrame(Canvas, ART_RGB(4, 2, 10));
}

#endif
