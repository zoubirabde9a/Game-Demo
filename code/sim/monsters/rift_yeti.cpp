/* Frostmaw Yeti: a shaggy white brute from the Aurora Rift
   (docs/dungeon-rift.md). Ground Pound sends a ring of frost rolling out
   across the floor (MonsterAbility_Wave): it goes through walls and
   outruns anyone, so the only answer is to jump it as it reaches you.
   From farther off it bounds at its target in a leap. Only the rift's
   encounters and its bosses bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Yeti)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Yeti(monster_def *Def)
{
    Def->Name = "Frostmaw Yeti";
    Def->MaxHp = 150.f;
    Def->Acceleration = 30000.f;
    Def->AggroRange = 400.f;
    Def->StopRange = 40.f;
    Def->AttackRange = 52.f;
    Def->AttackDamage = 12.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 56;

    // NOTE(zoubir): one ring out to 260, about a second to read it
    monster_ability *Pound = AddMonsterAbility(Def, MonsterAbility_Wave,
                                               "Ground Pound");
    Pound->MaxRange = 170.f;
    Pound->Cooldown = 6.f;
    Pound->Windup = 0.9f;
    Pound->Active = 0.9f;
    Pound->Recover = 0.7f;
    Pound->Damage = 18.f;
    Pound->Radius = 260.f;
    Pound->Speed = 300.f;
    Pound->Knockback = 320.f;
    Pound->Count = 1;
    Pound->Status = StatusEffect_Slowed;
    Pound->StatusSeconds = 1.5f;

    monster_ability *Bound = AddMonsterAbility(Def, MonsterAbility_Charge,
                                               "Bounding Leap");
    Bound->MinRange = 170.f;
    Bound->MaxRange = 420.f;
    Bound->Cooldown = 7.f;
    Bound->Windup = 0.7f;
    Bound->Active = 0.55f;
    Bound->Recover = 0.8f;
    Bound->Damage = 16.f;
    Bound->Radius = 36.f;
    Bound->Speed = 620.f;
    Bound->Knockback = 450.f;
}

#else

internal void
DrawMonster_Yeti(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Fur = Ramp(ART_RGB(84, 98, 128), ART_RGB(146, 162, 190),
                          ART_RGB(204, 216, 234), ART_RGB(244, 248, 255));
    color_ramp Skin = Ramp(ART_RGB(40, 52, 78), ART_RGB(66, 84, 120),
                           ART_RGB(98, 120, 160), ART_RGB(136, 160, 198));
    color_ramp Ice = Ramp(ART_RGB(50, 110, 160), ART_RGB(100, 180, 220),
                          ART_RGB(170, 230, 250), ART_RGB(236, 252, 255));

    float Bob = 0.f;
    float Step = 0.f;
    // NOTE(zoubir): both fists, 0 hanging, 1 high over its head
    float Raise = 0.f;
    float Hunch = 0.f;
    float Mouth = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 4.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            Hunch = 2.f;
        } break;

        case AnimationType_Cast:
        {
            Raise = Pose.t;
            Bob = -2.f * Pose.t;
            Mouth = Pose.t;
        } break;

        case AnimationType_Attack:
        {
            Raise = 1.f - 1.3f * Minimum(1.f, 2.5f * Pose.t);
            Bob = 3.f * Minimum(1.f, 2.5f * Pose.t);
            Hunch = 3.f;
            Mouth = 1.f;
        } break;

        case AnimationType_Stop:
        {
            Raise = -0.25f;
            Hunch = 3.f;
            Bob = 2.f + 0.5f * Pose.Wave;
        } break;

        default:
        {
            Bob = 0.7f * Pose.Wave;
            Mouth = 0.2f + 0.2f * Pose.Wave2;
        } break;
    }

    float Ground = 49.f;
    float HipY = 37.f + Bob;
    float BodyY = HipY - 9.f;

    // NOTE(zoubir): short legs under the shag
    FillLimb(Canvas, V2(23.f, HipY), V2(22.f - Step, Ground - 2.f), 4.5f, 4.f, Fur, -0.4f);
    FillBlob(Canvas, 22.f - Step, Ground - 1.f, 4.5f, 2.f, Skin, -0.2f);
    FillLimb(Canvas, V2(32.f, HipY), V2(33.f + Step, Ground - 2.f), 4.5f, 4.f, Fur, 0.1f);
    FillBlob(Canvas, 33.f + Step, Ground - 1.f, 4.5f, 2.f, Skin, 0.f);

    // NOTE(zoubir): the far arm, long like an ape's
    v2 FarShoulder = V2(20.f + Hunch, BodyY - 5.f);
    v2 FarFist = FarShoulder + V2(-4.f, 13.f - 26.f * Raise);
    FillLimb(Canvas, FarShoulder, FarFist, 4.f, 3.5f, Fur, -0.4f);
    FillBlob(Canvas, FarFist.X, FarFist.Y, 3.5f, 3.5f, Skin, -0.2f);

    // NOTE(zoubir): a shaggy barrel of a body, icicles in the fur
    FillBlob(Canvas, 28.f + 0.5f * Hunch, BodyY, 12.f, 11.f, Fur, 0.f);
    for(u32 Icicle = 0; Icicle < 4; Icicle++)
    {
        float X = 19.f + 5.f * Icicle + 0.5f * Hunch;
        FillTriangle(Canvas, V2(X - 1.5f, BodyY + 8.f), V2(X + 1.5f, BodyY + 8.f),
                     V2(X, BodyY + 13.f - (Icicle % 2)), Ice, 0.4f, 1.f);
    }
    FillTriangle(Canvas, V2(20.f, BodyY - 8.f), V2(24.f, BodyY - 10.f), V2(18.f, BodyY - 15.f), Ice, 0.3f, 1.f);

    // NOTE(zoubir): the near arm and its big fist
    v2 Shoulder = V2(35.f + Hunch, BodyY - 4.f);
    v2 Fist = Shoulder + V2(7.f, 13.f - 26.f * Raise);
    FillLimb(Canvas, Shoulder, Fist, 4.5f, 4.f, Fur, 0.2f);
    FillBlob(Canvas, Fist.X, Fist.Y, 4.5f, 4.5f, Skin, 0.2f);

    // NOTE(zoubir): a blue face under a white brow, the jaw drops to roar
    float HeadX = 37.f + Hunch;
    float HeadY = BodyY - 8.f + 0.5f * Hunch;
    FillBlob(Canvas, HeadX, HeadY, 6.f, 6.f, Fur, 0.25f);
    FillBlob(Canvas, HeadX + 2.f, HeadY + 1.f, 4.5f, 4.f, Skin, 0.1f);
    FillDot(Canvas, HeadX + 3.f, HeadY - 1.f, 1.f, ART_RGB(250, 250, 200));
    FillDot(Canvas, HeadX + 3.f, HeadY + 2.5f + 2.f * Mouth, 1.2f + Mouth, ART_RGB(30, 20, 40));
    FillTriangle(Canvas, V2(HeadX + 1.5f, HeadY + 2.f), V2(HeadX + 2.5f, HeadY + 2.f),
                 V2(HeadX + 2.f, HeadY + 4.f), Ice, 0.8f, 1.f);
    FillBlob(Canvas, HeadX - 1.f, HeadY - 4.f, 5.f, 2.5f, Fur, 0.4f);

    OutlineFrame(Canvas, ART_RGB(14, 18, 32));
}

#endif
