/* Vaelith's ice tombs (sim/dungeon/frost_tombs.cpp, docs/dungeon-rift.md),
   two kinds in one file because only that script makes them.

   Frost Mark: a sigil of frost that hangs over one player's head and
   follows them. Its Frostbind is a Slam that never lands (the script ends
   it a moment before): the windup only draws the closing ring, online as
   offline, and when it closes every player inside it is frozen. It flies
   (FlyHeight) above everyone's bodies, so it never shoves anybody, and
   nothing can hurt it (IsFrostMark, frost_tombs.h).

   Ice Tomb: a block of ice round one frozen player. The party breaks it to
   free them. Its Shatter is the countdown: a Slam that never lands either,
   its windup the time the party has, shown by the cast bar over the
   block; when it runs out the script shatters the block on the player
   inside. Never moves, never bites. Both only come from the script
   (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(FrostMark)
MONSTER(IceTomb)
#elif !defined(MONSTER_ART_PASS)

// NOTE(zoubir): how high the mark hangs: over a player's body (19 tall)
// and any ground monster's, which share the player's box
#define FROST_MARK_HEIGHT 40.f

internal void
DefineMonster_FrostMark(monster_def *Def)
{
    Def->Name = "Frost Mark";
    Def->MaxHp = 1.f;
    Def->Acceleration = 0.f;
    Def->AggroRange = 2000.f;
    Def->StopRange = 1.f;
    Def->AttackRange = 0.f;
    Def->AttackDamage = 0.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FlyHeight = FROST_MARK_HEIGHT;
    Def->FrameSize = 32;

    // NOTE(zoubir): Damage only to pass the table's checks; the script
    // freezes whoever the ring holds before the slam can land
    monster_ability *Bind = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Frostbind");
    Bind->MaxRange = 2000.f;
    Bind->Cooldown = 30.f;
    Bind->Windup = 3.5f;
    Bind->Active = 0.3f;
    Bind->Recover = 0.3f;
    Bind->Damage = 1.f;
    Bind->Radius = 70.f;
}

internal void
DefineMonster_IceTomb(monster_def *Def)
{
    Def->Name = "Ice Tomb";
    Def->MaxHp = 12.f;
    Def->Acceleration = 0.f;
    Def->AggroRange = 2000.f;
    Def->StopRange = 1.f;
    Def->AttackRange = 0.f;
    Def->AttackDamage = 0.f;
    Def->AttackInterval = 1.f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 48;

    // NOTE(zoubir): the countdown; the script shatters the tomb when it
    // ends, so the slam itself never lands
    monster_ability *Shatter = AddMonsterAbility(Def, MonsterAbility_Slam,
                                                 "Shatter");
    Shatter->MaxRange = 2000.f;
    Shatter->Cooldown = 30.f;
    Shatter->Windup = 10.f;
    Shatter->Active = 0.3f;
    Shatter->Recover = 0.3f;
    Shatter->Damage = 1.f;
    Shatter->Radius = 26.f;
}

#else

internal void
DrawMonster_FrostMark(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Ice = Ramp(ART_RGB(40, 90, 150), ART_RGB(96, 168, 222),
                          ART_RGB(170, 226, 250), ART_RGB(240, 252, 255));
    color_ramp Green = Ramp(ART_RGB(20, 100, 80), ART_RGB(40, 190, 140),
                            ART_RGB(130, 250, 190), ART_RGB(225, 255, 240));

    // NOTE(zoubir): the sigil turns slowly, and spins up and brightens as
    // Frostbind winds up
    float Turn = 0.5f * Pi32 * Pose.t;
    float Glow = 0.3f + 0.15f * Pose.Wave;
    if (Pose.Anim == AnimationType_Cast)
    {
        Turn = 2.f * Pi32 * Pose.t * Pose.t;
        Glow = 0.3f + 0.7f * Pose.t;
    }
    else if (Pose.Anim == AnimationType_Attack)
    {
        Glow = 1.f;
    }
    v2 Centre = V2(16.f, 16.f);
    float Arm = 9.f + 2.f * Glow;
    for(u32 Spoke = 0; Spoke < 6; Spoke++)
    {
        float Angle = Turn + (float)Spoke * (Pi32 / 3.f);
        v2 Out = V2(Cos(Angle), Sin(Angle));
        v2 Side = V2(-Out.Y, Out.X);
        v2 Tip = Centre + Arm * Out;
        FillLimb(Canvas, Centre, Tip, 1.6f, 1.f, Ice, 0.2f);
        // NOTE(zoubir): two barbs on each arm, a snowflake's
        v2 Barb = Centre + (0.6f * Arm) * Out;
        FillLimb(Canvas, Barb, Barb + 3.f * (Out + Side), 0.9f, 0.6f, Ice, 0.3f);
        FillLimb(Canvas, Barb, Barb + 3.f * (Out - Side), 0.9f, 0.6f, Ice, 0.3f);
    }
    FillBlob(Canvas, Centre.X, Centre.Y, 3.f + 1.5f * Glow, 3.f + 1.5f * Glow, Green, 0.3f);
    if (Glow > 0.7f)
    {
        FillDot(Canvas, Centre.X, Centre.Y, 1.5f, Green.C[3]);
    }
    OutlineFrame(Canvas, ART_RGB(10, 20, 40));
}

// NOTE(zoubir): the front face of the block, every other pixel, so the
// frozen player shows through it as through ice
internal void
FillIcePane(sprite_canvas *Canvas, i32 MinX, i32 MinY, i32 MaxX, i32 MaxY, u32 Color)
{
    for(i32 Y = MinY; Y < MaxY; Y++)
    {
        for(i32 X = MinX; X < MaxX; X++)
        {
            if (((X + Y) & 1) == 0 && X >= 0 && Y >= 0 &&
                X < (i32)Canvas->Width && Y < (i32)Canvas->Height)
            {
                Canvas->Pixels[Y * Canvas->Stride + X] = Color;
            }
        }
    }
}

internal void
DrawMonster_IceTomb(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Ice = Ramp(ART_RGB(44, 96, 150), ART_RGB(100, 170, 220),
                          ART_RGB(176, 228, 250), ART_RGB(240, 252, 255));
    color_ramp Frost = Ramp(ART_RGB(150, 190, 220), ART_RGB(196, 226, 244),
                            ART_RGB(226, 244, 252), ART_RGB(250, 254, 255));

    // NOTE(zoubir): the block shudders harder as Shatter winds up, and
    // cracks open along the front as it nears the end
    float Strain = 0.f;
    if (Pose.Anim == AnimationType_Cast)
    {
        Strain = Pose.t;
    }
    else if (Pose.Anim == AnimationType_Attack || Pose.Anim == AnimationType_Stop)
    {
        Strain = 1.f;
    }
    float Shake = (Pose.Frame & 1) ? Strain * 0.8f : -Strain * 0.8f;
    float Ground = 42.f;

    // NOTE(zoubir): a frozen puddle round the base
    FillFlatEllipse(Canvas, 24.f, Ground, 17.f, 4.f, Frost.C[1]);

    // NOTE(zoubir): the sides of the block are solid ice, the front a
    // pane the player inside shows through
    float Left = 12.f + Shake;
    float Right = 36.f + Shake;
    float Top = 9.f;
    FillTriangle(Canvas, V2(Left, Ground), V2(Left + 5.f, Ground), V2(Left + 1.f, Top + 4.f), Ice, 0.1f, 0.7f);
    FillTriangle(Canvas, V2(Right - 5.f, Ground), V2(Right, Ground), V2(Right - 1.f, Top + 2.f), Ice, 0.2f, 0.9f);
    FillLimb(Canvas, V2(Left + 2.f, Top + 3.f), V2(Right - 2.f, Top + 1.f), 2.5f, 2.5f, Frost, 0.3f);
    FillIcePane(Canvas, (i32)(Left + 4.f), (i32)(Top + 4.f), (i32)(Right - 4.f), (i32)Ground,
                Ice.C[2]);

    // NOTE(zoubir): shards on top, taller on one side
    FillTriangle(Canvas, V2(Left + 2.f, Top + 2.f), V2(Left + 9.f, Top + 2.f), V2(Left + 4.f, Top - 6.f), Ice, 0.2f, 1.f);
    FillTriangle(Canvas, V2(Right - 10.f, Top + 1.f), V2(Right - 2.f, Top + 1.f), V2(Right - 5.f, Top - 4.f), Ice, 0.1f, 0.9f);

    // NOTE(zoubir): cracks, one more each quarter of the countdown
    u32 Cracks = (u32)(4.f * Strain);
    v2 Crack[4][2] =
    {
        {V2(18.f, 14.f), V2(23.f, 24.f)},
        {V2(30.f, 16.f), V2(26.f, 30.f)},
        {V2(20.f, 38.f), V2(25.f, 28.f)},
        {V2(32.f, 36.f), V2(28.f, 26.f)},
    };
    for(u32 Index = 0; Index < Cracks && Index < 4; Index++)
    {
        FillLimb(Canvas, Crack[Index][0] + V2(Shake, 0.f), Crack[Index][1] + V2(Shake, 0.f),
                 0.6f, 0.4f, Frost, 0.6f);
    }
    if (Pose.Anim == AnimationType_Attack)
    {
        DissolveFrame(Canvas, 0.3f + 0.2f * Pose.t);
    }
    OutlineFrame(Canvas, ART_RGB(14, 30, 56));
}

#endif
