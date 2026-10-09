/* Hero headgear (hero_head.cpp): the later classes' hats and faces: a
   hood with the face showing through its opening, a band with two horns,
   a cavalier's wide hat with a feather; spiky hair, a cloth mask and a
   moustache. */

internal void
DrawHeroHood(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    switch(Pose->Facing)
    {
        case HeroFacing_Down:
        {
            HeroBlob(Canvas, Pose, H + V2(0.f, -0.5f), 8.f, 7.6f, Look->Hat);
            HeroTriangle(Canvas, Pose, H + V2(-3.f, -6.5f), H + V2(3.f, -6.5f), H + V2(0.f, -10.f),
                         Look->Hat, 0.7f, 0.5f);
            // NOTE(zoubir): the face in the hood's opening, the rim round it
            HeroBlob(Canvas, Pose, H + V2(0.f, 1.8f), 5.4f, 4.8f, Look->HatBand, -0.2f);
            HeroBlob(Canvas, Pose, H + V2(0.f, 2.2f), 4.6f, 4.2f, Look->Skin);
        } break;
        case HeroFacing_Up:
        {
            HeroBlob(Canvas, Pose, H + V2(0.f, -0.5f), 8.f, 7.6f, Look->Hat);
            // NOTE(zoubir): the hood's point falls down the back
            HeroTriangle(Canvas, Pose, H + V2(-4.f, 3.f), H + V2(4.f, 3.f), H + V2(0.f, 10.f),
                         Look->Hat, 0.5f, 0.3f);
        } break;
        default:
        {
            HeroBlob(Canvas, Pose, H + V2(-1.f, -0.5f), 7.8f, 7.6f, Look->Hat);
            HeroTriangle(Canvas, Pose, H + V2(-6.f, -3.f), H + V2(-4.f, 3.f), H + V2(-10.f, 5.f),
                         Look->Hat, 0.5f, 0.3f);
            HeroBlob(Canvas, Pose, H + V2(3.2f, 1.6f), 3.8f, 4.6f, Look->HatBand, -0.2f);
            HeroBlob(Canvas, Pose, H + V2(3.8f, 2.f), 3.f, 4.f, Look->Skin);
        } break;
    }
}

// NOTE(zoubir): one horn from Root curling out by Side and up
internal void
DrawHeroHorn(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 Root, float Side)
{
    v2 Mid = Root + V2(Side * 4.f, -2.5f);
    v2 Tip = Mid + V2(Side * 1.5f, -4.5f);
    HeroLimb(Canvas, Pose, Root, Mid, 1.9f, 1.4f, Look->Shaft, 0.1f);
    HeroLimb(Canvas, Pose, Mid, Tip, 1.4f, 0.5f, Look->Shaft, 0.2f);
}

internal void
DrawHeroHorns(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    switch(Pose->Facing)
    {
        case HeroFacing_Right:
        {
            DrawHeroHorn(Canvas, Pose, Look, H + V2(-1.f, -5.f), -1.f);
            HeroLimb(Canvas, Pose, H + V2(-6.f, -2.f), H + V2(6.f, -3.f), 1.1f, 1.1f, Look->HatBand);
            DrawHeroHorn(Canvas, Pose, Look, H + V2(2.f, -5.f), 1.f);
        } break;
        default:
        {
            HeroLimb(Canvas, Pose, H + V2(-7.f, -2.5f), H + V2(7.f, -2.5f), 1.1f, 1.1f, Look->HatBand);
            DrawHeroHorn(Canvas, Pose, Look, H + V2(-5.5f, -4.f), -1.f);
            DrawHeroHorn(Canvas, Pose, Look, H + V2(5.5f, -4.f), 1.f);
        } break;
    }
}

internal void
DrawHeroCavalier(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    bool32 Side = Pose->Facing == HeroFacing_Right;
    v2 Brim = H + V2(Side ? -0.5f : 0.f, -4.f);
    HeroBlob(Canvas, Pose, Brim, Side ? 10.f : 11.5f, 2.8f, Look->Hat, -0.1f);
    HeroBlob(Canvas, Pose, Brim + V2(0.f, -2.8f), 5.6f, 3.6f, Look->Hat);
    HeroLimb(Canvas, Pose, Brim + V2(-5.f, -1.2f), Brim + V2(5.f, -1.2f), 1.f, 1.f, Look->HatBand);
    // NOTE(zoubir): the feather sweeps back off the band
    float Back = Side ? -1.f : (Pose->Facing == HeroFacing_Down ? 1.f : -1.f);
    v2 Root = Brim + V2(Back * 4.f, -2.f);
    v2 Mid = Root + V2(Back * 4.f, -4.f);
    v2 End = Mid + V2(Back * 3.f, 1.5f);
    HeroLimb(Canvas, Pose, Root, Mid, 1.2f, 2.f, Look->ShieldField, 0.15f);
    HeroLimb(Canvas, Pose, Mid, End, 2.f, 0.6f, Look->ShieldField, 0.1f);
}

// NOTE(zoubir): tufts standing up off the crown
internal void
DrawHeroSpikes(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    float Lean = Pose->Facing == HeroFacing_Right ? -1.5f : 0.f;
    for(i32 Spike = -2; Spike <= 2; Spike++)
    {
        float X = 2.8f * (float)Spike;
        v2 Base = H + V2(X, -5.f + 0.25f * (float)(Spike * Spike));
        v2 Tip = Base + V2(0.8f * (float)Spike + Lean, -4.f + 0.5f * (float)(Spike * Spike));
        HeroTriangle(Canvas, Pose, Base + V2(-2.f, 1.f), Base + V2(2.f, 1.f), Tip, Look->Hair,
                     0.75f, 0.45f);
    }
}

// NOTE(zoubir): a cloth mask over mouth and nose, and a moustache
internal void
DrawHeroFaceCover(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 H)
{
    if (Pose->Facing == HeroFacing_Up)
    {
        return;
    }
    bool32 Side = Pose->Facing == HeroFacing_Right;
    if (Look->Mask)
    {
        if (Side)
        {
            HeroBlob(Canvas, Pose, H + V2(4.2f, 4.4f), 3.f, 2.2f, Look->Hat, 0.1f);
        }
        else
        {
            HeroBlob(Canvas, Pose, H + V2(0.f, 4.6f), 4.6f, 2.2f, Look->Hat, 0.1f);
        }
    }
    if (Look->Mustache)
    {
        if (Side)
        {
            HeroLimb(Canvas, Pose, H + V2(4.f, 4.f), H + V2(6.f, 3.6f), 0.6f, 0.5f, Look->Hair);
        }
        else
        {
            HeroLimb(Canvas, Pose, H + V2(-2.5f, 4.4f), H + V2(-0.4f, 3.9f), 0.6f, 0.5f, Look->Hair);
            HeroLimb(Canvas, Pose, H + V2(0.4f, 3.9f), H + V2(2.5f, 4.4f), 0.6f, 0.5f, Look->Hair);
        }
    }
}
