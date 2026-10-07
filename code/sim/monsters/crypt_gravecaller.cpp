/* Gravecaller Ossian: the Sunken Crypt's first boss (sim/dungeon/,
   docs/dungeon-plan.md), in the Ossuary. A lich in rusted bone armour
   with a crown of finger bones and a reaper's hook. Never roams
   (SpawnWeight 0): the dungeon places it.

   Calm:    Bone Spikes marks four spots round its target and splits the
            ground there, leaving the struck bleeding; Raise the Honour
            Guard lifts two Skeletal Thralls, four at most, which the
            healer and the damage players clear while the tank holds it.
   Enraged (below half health): faster, glows a sickly green, and adds
            Grave Lunge: it vanishes and strikes from behind its target.
   The dungeon adds the rest of the fight (sim/dungeon/boss_scripts.cpp):
   Bone Shamans climbing out of the walls at two thirds and one third. */
#if defined(MONSTER_NAME_PASS)
MONSTER(Gravecaller)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Gravecaller(monster_def *Def)
{
    Def->Name = "Gravecaller Ossian";
    Def->MaxHp = 900.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 600.f;
    Def->StopRange = 45.f;
    Def->AttackRange = 58.f;
    Def->AttackDamage = 12.f;
    Def->AttackInterval = 1.3f;
    Def->SpawnWeight = 0;
    Def->MaxAlive = 1;
    Def->FrameSize = 64;
    Def->FrameCounts[MonsterRow_Windup] = 6;

    Def->EnrageHpShare = 0.5f;
    Def->EnrageSpeedScale = 1.25f;
    Def->EnrageCooldownScale = 0.7f;
    Def->EnrageTint = 0xFFA0FFA0;

    monster_ability *Lunge = AddMonsterAbility(Def, MonsterAbility_Blink,
                                               "Grave Lunge");
    Lunge->MinRange = 80.f;
    Lunge->MaxRange = 420.f;
    Lunge->Cooldown = 7.f;
    Lunge->Windup = 0.7f;
    Lunge->Active = 0.2f;
    Lunge->Recover = 0.7f;
    Lunge->Damage = 22.f;
    Lunge->Spread = 50.f;
    Lunge->Radius = 70.f;
    Lunge->Knockback = 300.f;
    Lunge->PhaseMask = PHASE_ENRAGED;

    monster_ability *Spikes = AddMonsterAbility(Def, MonsterAbility_Mortar,
                                                "Bone Spikes");
    Spikes->MaxRange = 420.f;
    Spikes->Cooldown = 5.f;
    Spikes->Windup = 1.f;
    Spikes->Active = 0.3f;
    Spikes->Recover = 0.6f;
    Spikes->Damage = 18.f;
    Spikes->Radius = 48.f;
    Spikes->Count = MAX_ABILITY_POINTS;
    Spikes->Spread = 110.f;
    Spikes->Knockback = 200.f;
    Spikes->Status = StatusEffect_Bleeding;
    Spikes->StatusSeconds = 3.f;

    monster_ability *Guard = AddMonsterAbility(Def, MonsterAbility_Summon,
                                               "Raise the Honour Guard");
    Guard->MaxRange = 500.f;
    Guard->Cooldown = 12.f;
    Guard->Windup = 1.2f;
    Guard->Active = 0.3f;
    Guard->Recover = 0.6f;
    Guard->SummonKind = MonsterKind_Thrall;
    Guard->Count = 2;
    Guard->MaxActive = 4;
    Guard->Spread = 60.f;
    Guard->Radius = 14.f;
}

#else

internal void
DrawMonster_Gravecaller(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Bone = Ramp(ART_RGB(112, 102, 86), ART_RGB(176, 166, 140),
                           ART_RGB(216, 208, 184), ART_RGB(244, 240, 224));
    color_ramp Rust = Ramp(ART_RGB(40, 30, 28), ART_RGB(70, 50, 40),
                           ART_RGB(104, 72, 52), ART_RGB(140, 100, 70));
    color_ramp Robe = Ramp(ART_RGB(22, 26, 24), ART_RGB(38, 46, 40),
                           ART_RGB(56, 68, 58), ART_RGB(78, 94, 80));
    color_ramp Iron = Ramp(ART_RGB(40, 40, 44), ART_RGB(76, 76, 82),
                           ART_RGB(120, 120, 126), ART_RGB(190, 192, 196));
    color_ramp Soul = Ramp(ART_RGB(30, 120, 60), ART_RGB(60, 200, 100),
                           ART_RGB(130, 250, 150), ART_RGB(220, 255, 220));

    float Bob = 0.f;
    float Lean = 0.f;
    float Stride = 0.f;
    float Glow = 0.2f;
    // NOTE(zoubir): the hook is a haft from the hand toward its head; the
    // angle is from straight up, positive tipping it forward
    v2 Hand = V2(42.f, 34.f);
    float HookAngle = 0.35f;
    // NOTE(zoubir): the free hand, raised to call the dead
    v2 Palm = V2(18.f, 36.f);

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Stride = 3.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            HookAngle = 0.45f + 0.1f * Pose.Wave;
        } break;

        case AnimationType_Cast:
        {
            // NOTE(zoubir): both arms rise, the hook back over the crown
            Hand = Lerp2(Hand, Pose.t, V2(40.f, 27.f));
            HookAngle = Lerp(0.35f, Pose.t, -0.6f);
            Palm = Lerp2(Palm, Pose.t, V2(14.f, 18.f));
            Lean = -2.f * Pose.t;
            Glow = 0.2f + 0.8f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            // NOTE(zoubir): the hook reaps down and forward
            Hand = Lerp2(V2(40.f, 27.f), Pose.t, V2(46.f, 36.f));
            HookAngle = Lerp(-0.6f, Pose.t, 1.6f);
            Palm = V2(16.f, 30.f);
            Lean = 3.f;
            Glow = 1.f - 0.6f * Pose.t;
        } break;

        case AnimationType_Stop:
        {
            Lean = 2.f;
            Bob = 1.5f + 0.5f * Pose.Wave;
            HookAngle = 1.f;
        } break;

        default:
        {
            Bob = 0.8f * Pose.Wave;
            Glow = 0.25f + 0.15f * Pose.Wave;
        } break;
    }

    // NOTE(zoubir): bony feet under a tattered robe
    FillBlob(Canvas, 25.f - Stride, 56.f, 3.5f, 1.6f, Bone, -0.2f);
    FillBlob(Canvas, 36.f + Stride, 56.f, 3.5f, 1.6f, Bone);
    FillTriangle(Canvas, V2(30.f + Lean, 24.f + Bob), V2(14.f, 55.f),
                 V2(46.f, 55.f), Robe, 0.8f, 0.25f);
    // NOTE(zoubir): the robe's ragged hem
    for(u32 Tatter = 0; Tatter < 5; Tatter++)
    {
        float X = 16.f + 7.f * Tatter;
        FillTriangle(Canvas, V2(X - 2.f, 53.f), V2(X + 2.f, 53.f),
                     V2(X + 0.5f, 57.f + (float)(Tatter % 2)), Robe, 0.6f, 0.2f);
    }

    // NOTE(zoubir): rusted breastplate with ribs showing through the gaps
    float ChestX = 30.f + Lean;
    float ChestY = 32.f + Bob;
    // NOTE(zoubir): a cuirass tapering to the waist, split down the front
    // where the ribs show
    FillTriangle(Canvas, V2(ChestX - 9.f, ChestY - 7.f), V2(ChestX + 9.f, ChestY - 7.f),
                 V2(ChestX, ChestY + 11.f), Rust, 0.7f, 0.2f);
    FillBlob(Canvas, ChestX, ChestY - 3.f, 8.5f, 5.f, Rust, 0.1f);
    FillLimb(Canvas, V2(ChestX, ChestY - 7.f), V2(ChestX, ChestY + 8.f),
             1.6f, 0.8f, Robe);
    for(u32 Rib = 0; Rib < 3; Rib++)
    {
        float Y = ChestY - 4.f + 3.f * Rib;
        float Half = 5.f - 1.2f * Rib;
        FillLimb(Canvas, V2(ChestX - Half, Y + 1.f), V2(ChestX + Half, Y),
                 0.6f, 0.5f, Bone);
    }
    // NOTE(zoubir): pauldrons of stacked vertebrae
    FillBlob(Canvas, ChestX - 10.f, ChestY - 6.f, 4.5f, 3.5f, Bone, -0.1f);
    FillBlob(Canvas, ChestX + 10.f, ChestY - 6.f, 4.5f, 3.5f, Bone);
    FillDot(Canvas, ChestX - 10.f, ChestY - 9.f, 1.6f, Bone.C[3]);
    FillDot(Canvas, ChestX + 10.f, ChestY - 9.f, 1.6f, Bone.C[3]);

    // NOTE(zoubir): the hook behind its hand: an iron haft, a pale blade
    v2 Up = V2(Sin(HookAngle), -Cos(HookAngle));
    v2 Head = Hand + 22.f * Up;
    FillLimb(Canvas, Hand - 8.f * Up, Head, 1.2f, 1.f, Iron);
    // NOTE(zoubir): the blade curls forward and down from the head like a
    // reaper's, thinning to a point
    v2 Across = V2(-Up.Y, Up.X);
    v2 Last = Head;
    for(u32 Step = 1; Step <= 6; Step++)
    {
        float Bend = 0.35f * (float)Step;
        v2 Point = Head + (2.4f * Step) * (Cos(Bend) * Across) - (2.4f * Step) * (Sin(Bend) * Up);
        FillLimb(Canvas, Last, Point, 1.8f - 0.25f * Step, 1.6f - 0.25f * Step, Bone, 0.1f);
        Last = Point;
    }

    // NOTE(zoubir): arms: one on the haft, one open to the dead
    FillLimb(Canvas, V2(ChestX + 8.f, ChestY - 4.f), Hand, 1.8f, 1.4f, Bone);
    FillBlob(Canvas, Hand.X, Hand.Y, 2.2f, 2.2f, Bone, 0.1f);
    FillLimb(Canvas, V2(ChestX - 8.f, ChestY - 4.f), Palm, 1.8f, 1.4f, Bone, -0.1f);
    FillBlob(Canvas, Palm.X, Palm.Y, 2.2f, 2.f, Bone, -0.1f);
    if (Glow > 0.3f)
    {
        u32 Motes = 2 + (u32)(5.f * Glow);
        for(u32 Mote = 0; Mote < Motes; Mote++)
        {
            float Angle = 2.f * Pi32 * ((float)Mote / (float)Motes + 0.5f * Pose.t);
            float R = 4.f + 3.f * Glow;
            FillDot(Canvas, Palm.X + R * Cos(Angle), Palm.Y + 0.7f * R * Sin(Angle),
                    0.7f + 0.6f * Glow, Soul.C[1 + (Mote % 3)]);
        }
    }

    // NOTE(zoubir): a long skull under a crown of finger bones
    float SkullX = ChestX + 2.f;
    float SkullY = ChestY - 15.f;
    FillBlob(Canvas, SkullX, SkullY, 6.f, 6.5f, Bone, 0.05f);
    FillBlob(Canvas, SkullX + 1.5f, SkullY + 5.f, 3.5f, 2.f, Bone);
    u32 EyeColor = Soul.C[Glow > 0.6f ? 3 : 2];
    FillDot(Canvas, SkullX - 0.5f, SkullY, 1.6f, ART_RGB(20, 18, 16));
    FillDot(Canvas, SkullX + 3.5f, SkullY, 1.4f, ART_RGB(20, 18, 16));
    FillDot(Canvas, SkullX - 0.5f, SkullY, 0.8f, EyeColor);
    FillDot(Canvas, SkullX + 3.5f, SkullY, 0.7f, EyeColor);
    for(u32 Spike = 0; Spike < 5; Spike++)
    {
        float X = SkullX - 5.f + 2.5f * Spike;
        float Tall = (Spike == 2) ? 7.f : 4.5f;
        FillLimb(Canvas, V2(X, SkullY - 5.f), V2(X + 0.5f, SkullY - 5.f - Tall),
                 0.9f, 0.4f, Bone);
    }

    if (Pose.Anim == AnimationType_Attack)
    {
        // NOTE(zoubir): a pale arc behind the reaping blade
        for(u32 Trail = 1; Trail <= 4; Trail++)
        {
            float Back = HookAngle - 0.25f * Trail;
            v2 Point = Hand + 30.f * V2(Sin(Back), -Cos(Back));
            FillDot(Canvas, Point.X, Point.Y, 1.6f - 0.25f * Trail, Soul.C[2]);
        }
    }

    OutlineFrame(Canvas, ART_RGB(12, 10, 10));
}

#endif
