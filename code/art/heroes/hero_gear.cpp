/* Hero gear (hero_rig.cpp): what a hero holds (a staff with a glowing
   orb, a sword, a kite shield) and the light their casts and blows give
   off, plus the dust a skid kicks up. */

// NOTE(zoubir): a disc that only covers an ordered-dither share of its
// pixels, Amount 0 none to 1 all: soft light and dust
internal void
HeroDitherDisc(sprite_canvas *Canvas, v2 C, float R, u32 Color, float Amount)
{
    for(i32 Y = (i32)(C.Y - R - 1.f); Y <= (i32)(C.Y + R + 1.f); Y++)
    {
        for(i32 X = (i32)(C.X - R - 1.f); X <= (i32)(C.X + R + 1.f); X++)
        {
            float DX = (float)X + 0.5f - C.X;
            float DY = (float)Y + 0.5f - C.Y;
            float D = SquareRoot(DX * DX + DY * DY) / Maximum(R, 0.01f);
            float Share = Amount * (1.f - D * D);
            if (D <= 1.f && BayerMatrix4[(Y & 3) * 4 + (X & 3)] < Share)
            {
                PutPixel(Canvas, X, Y, Color);
            }
        }
    }
}

// NOTE(zoubir): the weapon's direction on screen for this facing
inline v2
HeroWeaponDir(hero_pose *Pose)
{
    v2 Result = V2(Cos(Pose->WeaponAngle), Sin(Pose->WeaponAngle));
    if (Pose->Facing == HeroFacing_Down)
    {
        Result.X = -Result.X;
    }
    return Result;
}

// NOTE(zoubir): where the staff's orb or the sword's point is
internal v2
HeroWeaponTip(hero_pose *Pose, hero_look *Look, hero_joints *J)
{
    float Reach = Look->Weapon == HeroWeapon_Staff ? 15.f : 14.f;
    v2 Result = J->HandMain + Reach * HeroWeaponDir(Pose);
    return Result;
}

internal void
DrawHeroWeapon(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, hero_joints *J)
{
    if (Pose->WeaponDropped)
    {
        // NOTE(zoubir): lying on the ground beside the body, not turned
        // with it
        v2 A = V2(HERO_MID_X - 13.f, HERO_FEET_Y + 1.f);
        v2 B = V2(HERO_MID_X + 5.f, HERO_FEET_Y + 2.f);
        FillLimb(Canvas, A, B, 1.f, 1.f, Look->Shaft);
        if (Look->Weapon == HeroWeapon_Staff)
        {
            FillBlob(Canvas, A.X - 1.f, A.Y, 2.4f, 2.4f, Look->Head);
        }
        else
        {
            FillLimb(Canvas, B + V2(-2.f, -3.f), B + V2(-2.f, 3.f), 0.9f, 0.9f, Look->Head);
        }
        return;
    }
    v2 Dir = HeroWeaponDir(Pose);
    v2 Hand = J->HandMain;
    if (Look->Weapon == HeroWeapon_Staff)
    {
        v2 Top = Hand + 15.f * Dir;
        v2 Bottom = Hand - 7.f * Dir;
        HeroLimb(Canvas, Pose, Bottom, Top, 1.f, 1.1f, Look->Shaft);
        // NOTE(zoubir): a cradle of two prongs round the orb
        v2 Side = V2(-Dir.Y, Dir.X);
        HeroLimb(Canvas, Pose, Top - 1.f * Dir, Top + 2.f * Dir + 2.f * Side, 0.7f, 0.6f, Look->HatBand);
        HeroLimb(Canvas, Pose, Top - 1.f * Dir, Top + 2.f * Dir - 2.f * Side, 0.7f, 0.6f, Look->HatBand);
        v2 Orb = Top + 2.5f * Dir;
        if (Pose->Glow > 0.f)
        {
            HeroDitherDisc(Canvas, HeroPoint(Pose, Orb), 3.f + 4.f * Pose->Glow, Look->Glow,
                           0.5f + 0.5f * Pose->Glow);
        }
        HeroBlob(Canvas, Pose, Orb, 2.4f, 2.4f, Look->Head, 0.15f);
        HeroPixel(Canvas, Pose, Orb + V2(-0.8f, -0.8f), Look->GlowCore);
    }
    else
    {
        v2 Side = V2(-Dir.Y, Dir.X);
        v2 Guard = Hand + 1.8f * Dir;
        v2 Point = Hand + 14.f * Dir;
        HeroLimb(Canvas, Pose, Guard, Point, 1.4f, 0.6f, Look->Shaft, 0.1f);
        // NOTE(zoubir): a bright line down the blade's middle
        HeroLimb(Canvas, Pose, Guard + Dir, Point - 2.f * Dir, 0.4f, 0.3f,
                 Ramp(Look->Shaft.C[3], Look->Shaft.C[3], Look->Shaft.C[3], Look->Shaft.C[3]));
        HeroLimb(Canvas, Pose, Guard - 2.8f * Side, Guard + 2.8f * Side, 0.9f, 0.9f, Look->Head);
        HeroLimb(Canvas, Pose, Hand - 2.f * Dir, Guard, 0.8f, 0.8f, Look->Belt);
        HeroDot(Canvas, Pose, Hand - 2.5f * Dir, 1.f, Look->Head.C[2]);
    }
}

// NOTE(zoubir): a kite shield round Centre, Height tall, Width wide;
// Back draws its wooden back and straps instead of its face
internal void
DrawHeroKite(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 Centre,
             float Width, float Height, bool32 Back)
{
    float W = 0.5f * Width;
    float Top = -0.42f * Height;
    float Mid = 0.05f * Height;
    float Low = 0.58f * Height;
    v2 TL = Centre + V2(-W, Top);
    v2 TR = Centre + V2(W, Top);
    v2 ML = Centre + V2(-W, Mid);
    v2 MR = Centre + V2(W, Mid);
    v2 Tip = Centre + V2(0.f, Low);
    color_ramp Face = Back ? Look->Belt : Look->ShieldRim;
    HeroBlob(Canvas, Pose, Centre + V2(0.f, Top), W, 1.6f, Face);
    HeroTriangle(Canvas, Pose, TL, TR, MR, Face, 0.8f, 0.5f);
    HeroTriangle(Canvas, Pose, TL, MR, ML, Face, 0.8f, 0.4f);
    HeroTriangle(Canvas, Pose, ML, MR, Tip, Face, 0.6f, 0.3f);
    if (Back)
    {
        HeroLimb(Canvas, Pose, Centre + V2(-W + 1.f, -1.f), Centre + V2(W - 1.f, -1.f),
                 0.8f, 0.8f, Look->Boots);
        HeroLimb(Canvas, Pose, Centre + V2(-W + 1.f, 3.f), Centre + V2(W - 1.f, 3.f),
                 0.8f, 0.8f, Look->Boots);
        return;
    }
    float F = W - 1.5f;
    v2 FL = Centre + V2(-F, Top + 1.5f);
    v2 FR = Centre + V2(F, Top + 1.5f);
    v2 FML = Centre + V2(-F, Mid);
    v2 FMR = Centre + V2(F, Mid);
    v2 FTip = Centre + V2(0.f, Low - 2.2f);
    HeroTriangle(Canvas, Pose, FL, FR, FMR, Look->ShieldField, 0.85f, 0.55f);
    HeroTriangle(Canvas, Pose, FL, FMR, FML, Look->ShieldField, 0.85f, 0.45f);
    HeroTriangle(Canvas, Pose, FML, FMR, FTip, Look->ShieldField, 0.65f, 0.3f);
    // NOTE(zoubir): a gold cross and boss
    HeroLimb(Canvas, Pose, Centre + V2(0.f, Top + 2.f), Centre + V2(0.f, Low - 3.f),
             0.6f, 0.6f, Look->HatBand);
    HeroLimb(Canvas, Pose, Centre + V2(-F + 0.5f, -1.f), Centre + V2(F - 0.5f, -1.f),
             0.6f, 0.6f, Look->HatBand);
    HeroBlob(Canvas, Pose, Centre + V2(0.f, -1.f), 1.6f, 1.6f, Look->HatBand, 0.2f);
}

internal void
DrawHeroShield(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, hero_joints *J)
{
    if (Look->Offhand != HeroOffhand_Shield)
    {
        return;
    }
    switch(Pose->Facing)
    {
        case HeroFacing_Down:
        {
            DrawHeroKite(Canvas, Pose, Look, J->HandOff + V2(1.f, -2.f), 10.f, 14.f, false);
        } break;
        case HeroFacing_Up:
        {
            DrawHeroKite(Canvas, Pose, Look, J->HandOff + V2(-1.f, -2.f), 10.f, 14.f, true);
        } break;
        default:
        {
            // NOTE(zoubir): held out in front, seen half edge-on
            DrawHeroKite(Canvas, Pose, Look, J->HandOff + V2(5.f, -3.f), 6.5f, 14.f, false);
        } break;
    }
}

// NOTE(zoubir): a cast's light in the off hand (or the shield) and the
// flash where a blow lands
internal void
DrawHeroFx(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, hero_joints *J)
{
    if (Pose->Glow > 0.f && Pose->Facing != HeroFacing_Up)
    {
        v2 Source = J->HandOff;
        if (Look->Offhand == HeroOffhand_Shield)
        {
            Source = J->HandOff + (Pose->Facing == HeroFacing_Right ? V2(5.f, -3.f) : V2(1.f, -2.f));
        }
        v2 P = HeroPoint(Pose, Source);
        HeroDitherDisc(Canvas, P, 2.f + 5.f * Pose->Glow, Look->Glow, 0.35f + 0.5f * Pose->Glow);
        FillDot(Canvas, P.X, P.Y, 0.8f + 1.6f * Pose->Glow, Look->Glow);
        FillDot(Canvas, P.X, P.Y, 0.4f + 0.9f * Pose->Glow, Look->GlowCore);
        // NOTE(zoubir): sparks circling the light
        for(u32 Spark = 0; Spark < 3; Spark++)
        {
            float Angle = 7.f * Pose->Glow + 2.1f * (float)Spark;
            float R = 3.f + 4.f * Pose->Glow;
            PutPixel(Canvas, (i32)(P.X + R * Cos(Angle)), (i32)(P.Y + 0.7f * R * Sin(Angle)),
                     Look->GlowCore);
        }
    }
    if (Pose->Impact > 0.f)
    {
        v2 P = HeroPoint(Pose, HeroWeaponTip(Pose, Look, J));
        float R = 2.f + 5.f * Pose->Impact;
        HeroDitherDisc(Canvas, P, R, Look->Glow, 0.8f * Pose->Impact);
        for(u32 Ray = 0; Ray < 4; Ray++)
        {
            float Angle = 0.4f + 0.5f * Pi32 * (float)Ray;
            v2 Out = V2(Cos(Angle), Sin(Angle));
            FillLimb(Canvas, P + 0.4f * R * Out, P + R * Out, 0.6f, 0.3f,
                     Ramp(Look->Glow, Look->Glow, Look->GlowCore, Look->GlowCore));
        }
        FillDot(Canvas, P.X, P.Y, 1.f + Pose->Impact, Look->GlowCore);
    }
}

// NOTE(zoubir): puffs of dust kicked up behind sliding feet
internal void
DrawHeroDust(sprite_canvas *Canvas, hero_pose *Pose)
{
    if (!Pose->Dust)
    {
        return;
    }
    float Grow = (float)Pose->Dust;
    u32 Dust = ART_RGB(196, 180, 150);
    u32 Shade = ART_RGB(150, 134, 108);
    for(u32 Puff = 0; Puff < 3; Puff++)
    {
        v2 P = V2(HERO_MID_X, HERO_FEET_Y);
        if (Pose->Facing == HeroFacing_Right)
        {
            P += V2(4.f + 2.5f * Grow * (float)Puff, -1.f - 0.6f * (float)Puff * Grow);
        }
        else
        {
            float Side = Puff == 1 ? 0.f : (Puff == 0 ? -1.f : 1.f);
            P += V2(Side * (4.f + 2.f * Grow), (Pose->Facing == HeroFacing_Down ? 1.f : -2.f) - 0.5f * Grow);
        }
        float R = 1.2f + 0.9f * Grow;
        HeroDitherDisc(Canvas, P + V2(0.5f, 0.5f), R, Shade, 0.9f - 0.2f * Grow);
        HeroDitherDisc(Canvas, P, R * 0.8f, Dust, 1.f - 0.2f * Grow);
    }
}
