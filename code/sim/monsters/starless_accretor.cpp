/* Accretor: a knot of black rubble in the Starless Deep
   (docs/dungeon-starless.md) packed round a burning sliver of the dead
   star, with a disc of debris circling it like a ring round a planet.
   Accretion Disc swings the ring out to 230 (a slam with an inner
   radius): only the ground close round its core is safe, so step in to
   it. Core Flare then burns everyone close: step back out as its core
   brightens. The safe spot swaps between in and out, so watch which
   one is coming. Only the deep's encounters bring it (SpawnWeight 0). */
#if defined(MONSTER_NAME_PASS)
MONSTER(Accretor)
#elif !defined(MONSTER_ART_PASS)

internal void
DefineMonster_Accretor(monster_def *Def)
{
    Def->Name = "Accretor";
    Def->MaxHp = 210.f;
    Def->Acceleration = 22000.f;
    Def->AggroRange = 380.f;
    Def->StopRange = 44.f;
    Def->AttackRange = 58.f;
    Def->AttackDamage = 12.f;
    Def->AttackInterval = 1.1f;
    Def->SpawnWeight = 0;
    Def->FrameSize = 64;

    monster_ability *Flare = AddMonsterAbility(Def, MonsterAbility_Slam,
                                               "Core Flare");
    Flare->MaxRange = 90.f;
    Flare->Cooldown = 5.f;
    Flare->Windup = 0.8f;
    Flare->Active = 0.3f;
    Flare->Recover = 0.6f;
    Flare->Damage = 11.f;
    Flare->Radius = 95.f;
    Flare->Knockback = 420.f;
    Flare->Status = StatusEffect_Burning;
    Flare->StatusSeconds = 2.f;

    // NOTE(zoubir): a ring from 85 out to 230; standing at its feet is safe
    monster_ability *Disc = AddMonsterAbility(Def, MonsterAbility_Slam,
                                              "Accretion Disc");
    Disc->MaxRange = 220.f;
    Disc->Cooldown = 7.f;
    Disc->Windup = 1.1f;
    Disc->Active = 0.4f;
    Disc->Recover = 0.6f;
    Disc->Damage = 10.f;
    Disc->Radius = 230.f;
    Disc->InnerRadius = 85.f;
    Disc->Knockback = 300.f;
}

#else

// NOTE(zoubir): draws the half of the debris ring that is behind the body
// (Front false) or in front of it (Front true)
internal void
DrawAccretorRing(sprite_canvas *Canvas, v2 Center, float RX, float RY,
                 float Tilt, float Spin, bool32 Front,
                 color_ramp Stone, color_ramp Rubble, color_ramp Gold)
{
    u32 RockCount = 18;
    float TiltCos = Cos(Tilt);
    float TiltSin = Sin(Tilt);
    // NOTE(zoubir): a band of fine dust traces the whole ellipse
    for(u32 Mote = 0; Mote < 48; Mote++)
    {
        float Angle = 2.f * Pi32 * ((float)Mote / 48.f + 0.5f * Spin);
        float Depth = Sin(Angle);
        if ((Depth > 0.f) != (Front != 0))
        {
            continue;
        }
        float LX = RX * Cos(Angle);
        float LY = RY * Depth;
        u32 Color = Front ? ART_RGB(150, 110, 200) : ART_RGB(92, 64, 130);
        if (Mote % 6 == 0)
        {
            Color = ART_RGB(230, 186, 80);
        }
        FillDot(Canvas, Center.X + LX * TiltCos - LY * TiltSin,
                Center.Y + LX * TiltSin + LY * TiltCos, 0.8f, Color);
    }
    for(u32 Rock = 0; Rock < RockCount; Rock++)
    {
        float Angle = 2.f * Pi32 * (((float)Rock + 0.37f * (Rock % 3)) / (float)RockCount + Spin);
        float Depth = Sin(Angle);
        if ((Depth > 0.f) != (Front != 0))
        {
            continue;
        }
        float LX = RX * Cos(Angle);
        float LY = RY * Depth;
        float X = Center.X + LX * TiltCos - LY * TiltSin;
        float Y = Center.Y + LX * TiltSin + LY * TiltCos;
        // NOTE(zoubir): rocks at the front look a little bigger
        float Size = (Rock % 3 == 0 ? 2.6f : (Rock % 3 == 1 ? 1.9f : 1.4f)) + 0.4f * Depth;
        if (Rock % 4 == 2)
        {
            FillBlob(Canvas, X, Y, Size * 0.8f, Size * 0.8f, Gold, 0.2f + 0.2f * Depth);
        }
        else if (Rock % 2)
        {
            FillBlob(Canvas, X, Y, Size + 0.4f, Size, Rubble, 0.1f + 0.2f * Depth);
        }
        else
        {
            FillBlob(Canvas, X, Y, Size, Size * 0.85f, Stone, 0.1f + 0.2f * Depth);
        }
    }
}

internal void
DrawMonster_Accretor(sprite_canvas *Canvas, monster_pose Pose)
{
    color_ramp Stone = Ramp(ART_RGB(18, 16, 24), ART_RGB(40, 36, 52),
                            ART_RGB(68, 62, 84), ART_RGB(106, 98, 124));
    color_ramp Ash = Ramp(ART_RGB(50, 38, 72), ART_RGB(96, 76, 128),
                          ART_RGB(146, 122, 180), ART_RGB(200, 180, 226));
    color_ramp Rubble = Ramp(ART_RGB(10, 8, 14), ART_RGB(26, 22, 32),
                             ART_RGB(46, 40, 54), ART_RGB(74, 66, 84));
    color_ramp Halo = Ramp(ART_RGB(60, 20, 96), ART_RGB(120, 56, 190),
                           ART_RGB(196, 140, 250), ART_RGB(246, 226, 255));
    color_ramp Gold = Ramp(ART_RGB(100, 60, 10), ART_RGB(176, 124, 30),
                           ART_RGB(236, 196, 76), ART_RGB(255, 244, 186));

    float Bob = 0.f;
    float Step = 0.f;
    float Lean = 0.f;
    float Tilt = -0.34f;
    float Spread = 0.f;
    float Spin = Pose.t / 14.f;
    float Glow = 0.2f + 0.1f * Pose.Wave2;
    float Flare = 0.f;

    switch(Pose.Anim)
    {
        case AnimationType_Move:
        {
            Step = 4.f * Pose.Wave;
            Bob = -1.5f * Absolute(Pose.Wave);
            Lean = 1.5f * Pose.Wave;
            Tilt = -0.34f + 0.14f * Pose.Wave;
            Spin = 2.f * Pose.t / 14.f;
        } break;

        case AnimationType_Cast:
        {
            Spread = Pose.t;
            Spin = 0.4f * Pose.t * Pose.t;
            Glow = 0.3f + 0.7f * Pose.t;
            Bob = -2.f * Pose.t;
            Tilt = -0.34f + 0.12f * Pose.t;
        } break;

        case AnimationType_Attack:
        {
            float Snap = Minimum(1.f, 2.5f * Pose.t);
            Spread = 1.f + 0.35f * Snap;
            Spin = 0.4f + 0.3f * Pose.t;
            Glow = 1.f;
            Flare = 1.f - 0.6f * Pose.t;
            Bob = -2.f + 4.f * Snap;
            Tilt = -0.22f;
        } break;

        case AnimationType_Stop:
        {
            Spread = 0.9f * (1.f - Pose.t) * (1.f - Pose.t);
            Spin = 0.1f * Pose.t;
            Glow = 0.5f - 0.3f * Pose.t;
            Bob = 2.f - 1.5f * Pose.t;
            Tilt = -0.22f - 0.12f * Pose.t;
        } break;

        default:
        {
            Bob = 0.7f * Pose.Wave;
        } break;
    }

    float Ground = 56.f;
    float CX = 32.f + Lean;
    float CY = 36.f + Bob;
    v2 Center = V2(CX, CY - 1.f);
    float RingX = 23.5f + 3.3f * Spread;
    float RingY = 7.f + 2.f * Spread;

    // NOTE(zoubir): the far half of the disc, behind everything
    DrawAccretorRing(Canvas, Center, RingX, RingY, Tilt, Spin, false, Rubble, Stone, Gold);

    // NOTE(zoubir): two stubby legs, each a boulder on a flat foot stone
    for(u32 Leg = 0; Leg < 2; Leg++)
    {
        float Side = Leg ? 1.f : -1.f;
        float FootX = CX - Lean + 7.f * Side + Step * Side;
        float Light = Leg ? 0.f : -0.3f;
        FillBlob(Canvas, 0.5f * (CX + 6.f * Side + FootX), CY + 12.f, 5.f, 4.5f, Rubble, Light);
        FillBlob(Canvas, FootX, Ground - 3.f, 5.5f, 3.5f, Stone, Light + 0.1f);
    }

    // NOTE(zoubir): the knot of basalt chunks round the core
    FillBlob(Canvas, CX, CY, 13.f, 12.f, Stone, -0.1f);
    FillBlob(Canvas, CX - 8.f, CY - 7.f, 6.5f, 6.f, Rubble, 0.1f);
    FillBlob(Canvas, CX + 1.f, CY - 12.f, 6.f, 5.f, Stone, 0.3f);
    FillBlob(Canvas, CX + 9.f, CY - 7.f, 5.5f, 5.f, Stone, 0.2f);
    FillBlob(Canvas, CX - 10.f, CY + 4.f, 5.5f, 6.f, Rubble, -0.1f);
    FillBlob(Canvas, CX + 10.f, CY + 5.f, 5.f, 5.f, Stone, 0.f);
    FillBlob(Canvas, CX - 1.f, CY + 9.f, 6.f, 4.f, Rubble, -0.2f);
    FillTriangle(Canvas, V2(CX - 4.f, CY - 15.f), V2(CX + 1.f, CY - 15.f),
                 V2(CX - 3.f, CY - 21.f), Stone, 0.6f, 0.2f);
    FillTriangle(Canvas, V2(CX - 14.f, CY - 6.f), V2(CX - 12.f, CY - 2.f),
                 V2(CX - 19.f, CY - 7.f), Rubble, 0.5f, 0.1f);

    // NOTE(zoubir): gold cracks where the star shows through
    FillLimb(Canvas, V2(CX - 9.f, CY - 9.f), V2(CX - 5.f, CY - 3.f), 0.6f + 0.4f * Glow, 0.6f, Gold, 0.3f);
    FillLimb(Canvas, V2(CX + 10.f, CY + 2.f), V2(CX + 6.f, CY + 7.f), 0.6f + 0.4f * Glow, 0.6f, Gold, 0.3f);
    FillLimb(Canvas, V2(CX - 7.f, CY + 8.f), V2(CX - 3.f, CY + 5.f), 0.6f, 0.6f, Gold, 0.2f);

    // NOTE(zoubir): the sliver of star, a violet halo round a white-gold
    // core; it flares into spikes when it strikes
    float CoreX = CX + 2.f;
    float CoreY = CY - 1.f;
    if (Flare > 0.f)
    {
        for(u32 Ray = 0; Ray < 8; Ray++)
        {
            float Angle = 2.f * Pi32 * ((float)Ray / 8.f + 0.0625f);
            float Length = (Ray % 2 ? 11.f : 16.f) * Flare;
            v2 Dir = V2(Cos(Angle), Sin(Angle));
            v2 Side = V2(-Dir.Y, Dir.X);
            v2 Base = V2(CoreX, CoreY) + 4.f * Dir;
            FillTriangle(Canvas, Base + 4.f * Dir + Length * Dir,
                         Base + 2.2f * Side, Base - 2.2f * Side, Gold, 1.f, 0.6f);
        }
    }
    float HaloR = 5.f + 3.f * Glow + 2.f * Flare;
    FillBlob(Canvas, CoreX, CoreY, HaloR, HaloR, Halo, 0.3f + 0.3f * Glow);
    FillBlob(Canvas, CoreX, CoreY, 2.5f + 2.f * Glow, 3.5f + 2.f * Glow, Gold, 0.5f + 0.4f * Glow);
    FillDot(Canvas, CoreX, CoreY, 1.2f + 1.5f * Glow, ART_RGB(255, 252, 236));

    // NOTE(zoubir): the near half of the disc, over the body
    DrawAccretorRing(Canvas, Center, RingX, RingY, Tilt, Spin, true, Ash, Stone, Gold);

    OutlineFrame(Canvas, ART_RGB(8, 4, 14));
}

#endif
