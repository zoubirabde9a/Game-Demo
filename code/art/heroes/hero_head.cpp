/* Hero heads (hero_rig.cpp): the face, the hair round it and what is worn
   on it (a wizard's hat, a plumed helmet, a circlet), for each facing. */

#define HERO_EYE_RGB ART_RGB(28, 20, 34)
#define HERO_EYE_LIGHT_RGB ART_RGB(250, 250, 255)

// NOTE(zoubir): the hair that hangs behind the head, drawn first
internal void
DrawHeroBackHair(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    if (!Look->LongHair)
    {
        return;
    }
    switch(Pose->Facing)
    {
        case HeroFacing_Down:
        {
            HeroBlob(Canvas, Pose, H + V2(-5.5f, 4.f), 2.6f, 4.5f, Look->Hair, -0.15f);
            HeroBlob(Canvas, Pose, H + V2(5.5f, 4.f), 2.6f, 4.5f, Look->Hair, -0.15f);
        } break;
        case HeroFacing_Up:
        {
            HeroBlob(Canvas, Pose, H + V2(0.f, 5.5f), 6.f, 5.f, Look->Hair);
        } break;
        default:
        {
            HeroBlob(Canvas, Pose, H + V2(-4.f, 5.f), 3.2f, 5.f, Look->Hair);
        } break;
    }
}

internal void
DrawHeroFace(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    bool32 Helmet = Look->Headgear == HeroHeadgear_Helmet;
    if (Helmet)
    {
        // NOTE(zoubir): a dark visor slit, eyes glinting inside
        u32 Slit = ART_RGB(18, 16, 24);
        if (Pose->Facing == HeroFacing_Down)
        {
            HeroLimb(Canvas, Pose, H + V2(-4.f, 0.5f), H + V2(4.f, 0.5f), 1.f, 1.f,
                     Ramp(Slit, Slit, Slit, Slit));
            HeroLimb(Canvas, Pose, H + V2(0.f, 0.5f), H + V2(0.f, 4.f), 0.7f, 0.7f,
                     Ramp(Slit, Slit, Slit, Slit));
            HeroPixel(Canvas, Pose, H + V2(-2.5f, 0.5f), Look->Glow);
            HeroPixel(Canvas, Pose, H + V2(2.5f, 0.5f), Look->Glow);
        }
        else if (Pose->Facing == HeroFacing_Right)
        {
            HeroLimb(Canvas, Pose, H + V2(1.5f, 0.5f), H + V2(6.5f, 0.5f), 1.f, 1.f,
                     Ramp(Slit, Slit, Slit, Slit));
            HeroPixel(Canvas, Pose, H + V2(4.f, 0.5f), Look->Glow);
        }
        return;
    }
    if (Pose->Facing == HeroFacing_Down)
    {
        for(float Side = -1.f; Side <= 1.f; Side += 2.f)
        {
            HeroPixel(Canvas, Pose, H + V2(Side * 2.5f, 2.f), HERO_EYE_RGB);
            HeroPixel(Canvas, Pose, H + V2(Side * 2.5f, 3.f), HERO_EYE_RGB);
            HeroPixel(Canvas, Pose, H + V2(Side * 2.5f + 1.f, 2.f), HERO_EYE_RGB);
            HeroPixel(Canvas, Pose, H + V2(Side * 2.5f + 1.f, 3.f), HERO_EYE_RGB);
            HeroPixel(Canvas, Pose, H + V2(Side * 2.5f, 2.f), HERO_EYE_LIGHT_RGB);
        }
    }
    else if (Pose->Facing == HeroFacing_Right)
    {
        HeroPixel(Canvas, Pose, H + V2(4.f, 2.f), HERO_EYE_RGB);
        HeroPixel(Canvas, Pose, H + V2(4.f, 3.f), HERO_EYE_RGB);
        HeroPixel(Canvas, Pose, H + V2(4.f, 2.f), HERO_EYE_LIGHT_RGB);
    }
}

// NOTE(zoubir): a short beard round the jaw, in the hair's colour
internal void
DrawHeroBeard(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    if (!Look->Beard || Pose->Facing == HeroFacing_Up)
    {
        return;
    }
    if (Pose->Facing == HeroFacing_Down)
    {
        HeroBlob(Canvas, Pose, H + V2(0.f, 5.f), 4.6f, 2.6f, Look->Hair, 0.1f);
        HeroLimb(Canvas, Pose, H + V2(-2.f, 3.8f), H + V2(2.f, 3.8f), 0.6f, 0.6f, Look->Skin, 0.2f);
    }
    else
    {
        HeroBlob(Canvas, Pose, H + V2(3.f, 4.8f), 3.4f, 2.5f, Look->Hair, 0.1f);
    }
}

// NOTE(zoubir): the hair's top over the skin, with a fringe for the front
internal void
DrawHeroHairCap(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    switch(Pose->Facing)
    {
        case HeroFacing_Down:
        {
            HeroBlob(Canvas, Pose, H + V2(0.f, -3.f), 7.6f, 4.6f, Look->Hair);
            // NOTE(zoubir): fringe locks falling over the brow
            HeroTriangle(Canvas, Pose, H + V2(-6.5f, -2.f), H + V2(-2.f, -2.f),
                         H + V2(-5.5f, 0.f), Look->Hair, 0.6f, 0.3f);
            HeroTriangle(Canvas, Pose, H + V2(-2.5f, -2.f), H + V2(2.f, -2.f),
                         H + V2(-1.f, -0.5f), Look->Hair, 0.7f, 0.4f);
            HeroTriangle(Canvas, Pose, H + V2(1.5f, -2.f), H + V2(6.5f, -2.f),
                         H + V2(5.5f, 0.f), Look->Hair, 0.6f, 0.3f);
        } break;
        case HeroFacing_Up:
        {
            HeroBlob(Canvas, Pose, H + V2(0.f, -0.5f), 7.4f, 6.8f, Look->Hair);
        } break;
        default:
        {
            HeroBlob(Canvas, Pose, H + V2(-2.5f, -2.5f), 6.2f, 4.6f, Look->Hair);
            HeroBlob(Canvas, Pose, H + V2(-4.f, 0.5f), 3.4f, 4.4f, Look->Hair);
            HeroTriangle(Canvas, Pose, H + V2(1.f, -4.f), H + V2(6.5f, -3.f),
                         H + V2(5.f, -1.f), Look->Hair, 0.6f, 0.35f);
        } break;
    }
}

internal void
DrawHeroWizardHat(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    bool32 Side = Pose->Facing == HeroFacing_Right;
    v2 Brim = H + V2(Side ? -0.5f : 0.f, -4.f);
    float BrimW = Side ? 9.5f : 10.5f;
    // NOTE(zoubir): the cone leans back and its tip flops over
    float Back = Side ? -2.f : (Pose->Facing == HeroFacing_Down ? 1.5f : -1.5f);
    v2 Bend = Brim + V2(Back, -9.5f);
    v2 Tip = Bend + V2(Back * 2.5f + (Side ? -2.f : 0.f), -2.5f);
    HeroBlob(Canvas, Pose, Brim, BrimW, 2.6f, Look->Hat, -0.1f);
    HeroTriangle(Canvas, Pose, Brim + V2(-5.5f, 0.f), Brim + V2(5.5f, 0.f), Bend,
                 Look->Hat, 0.35f, 0.75f);
    HeroTriangle(Canvas, Pose, Bend + V2(-2.f, 1.f), Bend + V2(2.f, 1.f), Tip,
                 Look->Hat, 0.6f, 0.4f);
    HeroLimb(Canvas, Pose, Brim + V2(-5.f, -1.5f), Brim + V2(5.f, -1.5f), 1.2f, 1.2f,
             Look->HatBand);
    if (Pose->Facing != HeroFacing_Up)
    {
        HeroDot(Canvas, Pose, Brim + V2(Side ? 3.f : 0.f, -1.5f), 1.2f, Look->GlowCore);
    }
}

internal void
DrawHeroHelmet(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    bool32 Side = Pose->Facing == HeroFacing_Right;
    HeroBlob(Canvas, Pose, H + V2(0.f, -0.5f), 7.4f, 7.f, Look->Hat);
    // NOTE(zoubir): a ridge over the crown and a cheek guard's rim
    if (Side)
    {
        HeroLimb(Canvas, Pose, H + V2(-5.f, -4.f), H + V2(4.f, -6.f), 0.9f, 0.9f, Look->HatBand);
    }
    else
    {
        HeroLimb(Canvas, Pose, H + V2(0.f, -7.f), H + V2(0.f, -1.f), 0.9f, 0.9f, Look->HatBand);
    }
    // NOTE(zoubir): the plume sweeps back off the crown
    float Back = Side ? -1.f : 0.f;
    v2 Root = H + V2(Side ? -1.f : 0.f, -6.5f);
    v2 Mid = Root + V2(Back * 4.f, -3.f);
    v2 End = Mid + V2(Back * 4.f, Side ? 2.5f : 0.f);
    color_ramp Plume = Look->ShieldField;
    HeroLimb(Canvas, Pose, Root, Mid, 1.8f, 2.2f, Plume, 0.1f);
    HeroLimb(Canvas, Pose, Mid, End, 2.2f, 1.2f, Plume);
}

internal void
DrawHeroCirclet(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    switch(Pose->Facing)
    {
        case HeroFacing_Down:
        {
            HeroLimb(Canvas, Pose, H + V2(-6.5f, -2.f), H + V2(6.5f, -2.f), 0.8f, 0.8f, Look->HatBand);
            HeroDot(Canvas, Pose, H + V2(0.f, -2.f), 1.4f, Look->GlowCore);
            HeroPixel(Canvas, Pose, H + V2(-0.5f, -2.5f), HERO_EYE_LIGHT_RGB);
        } break;
        case HeroFacing_Up:
        {
            HeroLimb(Canvas, Pose, H + V2(-7.f, -1.f), H + V2(7.f, -1.f), 0.8f, 0.8f, Look->HatBand);
        } break;
        default:
        {
            HeroLimb(Canvas, Pose, H + V2(-6.f, -1.f), H + V2(6.f, -2.5f), 0.8f, 0.8f, Look->HatBand);
            HeroDot(Canvas, Pose, H + V2(5.5f, -2.5f), 1.2f, Look->GlowCore);
        } break;
    }
}

internal void
DrawHeroHead(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, hero_joints *J)
{
    v2 H = J->Head;
    bool32 Helmet = Look->Headgear == HeroHeadgear_Helmet;
    if (!Helmet)
    {
        DrawHeroBackHair(Canvas, Pose, Look, H);
    }
    HeroBlob(Canvas, Pose, H, 7.f, 6.6f, Look->Skin);
    switch(Look->Headgear)
    {
        case HeroHeadgear_Helmet:
        {
            DrawHeroHelmet(Canvas, Pose, Look, H);
        } break;
        case HeroHeadgear_WizardHat:
        {
            DrawHeroHairCap(Canvas, Pose, Look, H);
            DrawHeroWizardHat(Canvas, Pose, Look, H);
        } break;
        case HeroHeadgear_Circlet:
        {
            DrawHeroHairCap(Canvas, Pose, Look, H);
            DrawHeroCirclet(Canvas, Pose, Look, H);
        } break;
        case HeroHeadgear_None:
        {
            DrawHeroHairCap(Canvas, Pose, Look, H);
        } break;
    }
    DrawHeroFace(Canvas, Pose, Look, H);
    DrawHeroBeard(Canvas, Pose, Look, H);
}
