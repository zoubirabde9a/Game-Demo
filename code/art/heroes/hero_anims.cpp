/* Hero animations (hero_rig.cpp): where the joints go on each frame of
   each animation, the same for every class; only the weapon's swing
   differs between a staff and a sword. A sheet is one row per animation
   and facing, HERO_SHEET_COLUMNS frames wide. */

enum hero_anim
{
    HeroAnim_Idle,
    HeroAnim_Walk,
    HeroAnim_Attack,
    HeroAnim_Cast,
    HeroAnim_Skid,
    HeroAnim_Jump,
    HeroAnim_Hurt,
    HeroAnim_Death,
    HeroAnim_Count,
};

#define HERO_SHEET_COLUMNS 8

struct hero_anim_def
{
    char *Name;
    u32 FrameCount;
    float SecondsPerFrame;
    bool32 Loops;
};

global_variable hero_anim_def HeroAnimDefs[HeroAnim_Count] =
{
    {"idle",   4, 0.22f,  true},
    {"walk",   8, 0.085f, true},
    {"attack", 6, 0.045f, false},
    {"cast",   6, 0.08f,  false},
    {"skid",   4, 0.05f,  false},
    {"jump",   4, 0.12f,  false},
    {"hurt",   3, 0.08f,  false},
    {"death",  6, 0.12f,  false},
};

// NOTE(zoubir): the hold every class rests its weapon in
internal float
HeroRestAngle(hero_look *Look, hero_facing Facing)
{
    float Result = 0.f;
    if (Look->Weapon == HeroWeapon_Staff)
    {
        Result = Facing == HeroFacing_Right ? -1.25f : -1.35f;
    }
    else
    {
        Result = Facing == HeroFacing_Right ? 1.15f : 1.8f;
    }
    return Result;
}

// NOTE(zoubir): six keys of a swing: anticipation, wound up, mid swing,
// the hit, follow-through, recovering. Side view, then front and back
struct hero_swing
{
    float Angle[6];
    v2 Hand[6];
};

global_variable hero_swing HeroSwordSide =
{
    {-1.6f, -2.3f, -0.7f, 0.5f, 0.8f, 1.f},
    {{-3.f, -6.f}, {-4.f, -8.f}, {2.f, -5.f}, {5.f, -2.f}, {5.f, -1.f}, {3.f, 0.f}},
};
global_variable hero_swing HeroSwordFront =
{
    {-1.4f, -1.9f, -0.3f, 1.3f, 1.7f, 1.7f},
    {{0.f, -6.f}, {-1.f, -9.f}, {2.f, -4.f}, {-3.f, 2.f}, {-4.f, 3.f}, {-2.f, 1.f}},
};
global_variable hero_swing HeroStaffSide =
{
    {-1.5f, -2.f, -0.9f, -0.1f, 0.f, -0.6f},
    {{-1.f, -3.f}, {-3.f, -5.f}, {1.f, -3.f}, {5.f, -3.f}, {5.f, -3.f}, {2.f, -1.f}},
};
global_variable hero_swing HeroStaffFront =
{
    {-1.4f, -1.8f, -0.6f, 0.6f, 0.7f, -0.4f},
    {{0.f, -3.f}, {1.f, -6.f}, {0.f, -4.f}, {-2.f, -1.f}, {-2.f, -1.f}, {0.f, -1.f}},
};

internal void
HeroWalkPose(hero_pose *Pose, hero_look *Look, float t)
{
    float P = 2.f * Pi32 * t;
    float S = Sin(P);
    float C = Cos(P);
    Pose->Body.Y = 0.9f * Absolute(S) - 0.3f;
    if (Pose->Facing == HeroFacing_Right)
    {
        Pose->FootNear = V2(4.5f * S, -2.2f * Maximum(0.f, C));
        Pose->FootFar = V2(-4.5f * S, -2.2f * Maximum(0.f, -C));
        Pose->HandMain.X = -2.5f * S;
        Pose->HandOff.X = 2.5f * S;
        Pose->Lean = 1.f;
        Pose->Hem = -1.f * S;
    }
    else
    {
        float Toward = Pose->Facing == HeroFacing_Down ? 1.f : -1.f;
        Pose->FootNear = V2(0.f, -2.2f * Maximum(0.f, C) + Toward * S);
        Pose->FootFar = V2(0.f, -2.2f * Maximum(0.f, -C) - Toward * S);
        Pose->Body.X = 0.5f * S;
        Pose->HandMain.Y = -1.5f * S;
        Pose->HandOff.Y = 1.5f * S;
        Pose->Hem = 0.8f * S;
    }
    Pose->WeaponAngle = HeroRestAngle(Look, Pose->Facing) + 0.08f * S;
}

internal void
HeroAttackPose(hero_pose *Pose, hero_look *Look, u32 Frame)
{
    bool32 Side = Pose->Facing == HeroFacing_Right;
    hero_swing *Swing = Look->Weapon == HeroWeapon_Staff ?
        (Side ? &HeroStaffSide : &HeroStaffFront) :
        (Side ? &HeroSwordSide : &HeroSwordFront);
    float Lean[6] = {-0.5f, -1.f, 1.5f, 2.5f, 2.f, 1.f};
    float Drop[6] = {0.f, 0.f, 0.5f, 1.f, 1.f, 0.5f};
    float Step[6] = {0.f, -1.f, 2.f, 3.f, 3.f, 2.f};
    float Hit[6] = {0.f, 0.f, 0.f, 1.f, 0.5f, 0.f};
    Pose->WeaponAngle = Swing->Angle[Frame];
    Pose->HandMain = Swing->Hand[Frame];
    Pose->Lean = Lean[Frame];
    Pose->Body.Y = Drop[Frame];
    Pose->Impact = Hit[Frame];
    // NOTE(zoubir): the off hand braces back as the main hand swings
    if (Side)
    {
        Pose->FootNear.X = Step[Frame];
        Pose->FootFar.X = -0.5f * Step[Frame];
        Pose->HandOff = V2(-0.5f * Step[Frame], -1.f);
    }
    else
    {
        float Toward = Pose->Facing == HeroFacing_Down ? 1.f : -1.f;
        Pose->FootNear.Y = 0.4f * Toward * Step[Frame];
        Pose->HandOff = V2(1.f, -0.4f * Step[Frame]);
        Pose->Head.Y = 0.3f * Drop[Frame];
    }
    Pose->Hem = (Side ? -0.5f : 0.3f) * Lean[Frame];
}

internal void
HeroCastPose(hero_pose *Pose, hero_look *Look, u32 Frame)
{
    float Glow[6] = {0.2f, 0.45f, 0.75f, 1.f, 0.7f, 0.25f};
    float Raise[6] = {1.f, 3.f, 5.f, 5.5f, 4.5f, 2.f};
    float Lift[6] = {0.f, -0.5f, -1.f, -1.f, -0.5f, 0.f};
    Pose->Glow = Glow[Frame];
    Pose->Body.Y = Lift[Frame];
    // NOTE(zoubir): the weapon goes up, the off hand reaches out with
    // the spell in it
    Pose->WeaponAngle = Look->Weapon == HeroWeapon_Staff ? -1.57f : -1.45f;
    Pose->HandMain = V2(0.f, -Raise[Frame]);
    if (Pose->Facing == HeroFacing_Right)
    {
        Pose->HandOff = V2(1.f + Raise[Frame], -0.8f * Raise[Frame]);
        Pose->Lean = 0.2f * Raise[Frame];
        Pose->FootNear.X = 1.5f;
        Pose->FootFar.X = -1.5f;
    }
    else
    {
        Pose->HandOff = V2(-0.4f * Raise[Frame], -0.9f * Raise[Frame]);
        Pose->Head.Y = Lift[Frame];
    }
    Pose->Hem = 0.6f * Sin(2.f * Pi32 * (float)Frame / 6.f);
}

internal void
HeroSkidPose(hero_pose *Pose, hero_look *Look, u32 Frame)
{
    float Back[4] = {-2.5f, -2.f, -1.f, 0.f};
    float Plant[4] = {4.f, 4.f, 3.f, 1.f};
    float Drop[4] = {1.f, 1.f, 0.5f, 0.f};
    u32 Dust[4] = {1, 2, 3, 0};
    Pose->Body.Y = Drop[Frame];
    Pose->Dust = Dust[Frame];
    Pose->WeaponAngle = HeroRestAngle(Look, Pose->Facing) - 0.15f * Back[Frame];
    if (Pose->Facing == HeroFacing_Right)
    {
        Pose->Lean = Back[Frame];
        Pose->FootNear.X = Plant[Frame];
        Pose->FootFar.X = -0.5f * Plant[Frame];
        Pose->HandMain = V2(-0.5f * Back[Frame], 0.5f * Back[Frame]);
        Pose->HandOff = V2(-Back[Frame], Back[Frame]);
        Pose->Hem = 0.6f * Plant[Frame];
    }
    else
    {
        float Toward = Pose->Facing == HeroFacing_Down ? 1.f : -1.f;
        Pose->FootNear = V2(-0.4f * Plant[Frame], 0.4f * Toward * Plant[Frame]);
        Pose->FootFar = V2(-0.4f * Plant[Frame], 0.f);
        Pose->HandMain = V2(-0.5f * Back[Frame], 0.6f * Back[Frame]);
        Pose->HandOff = V2(-0.5f * Back[Frame], 0.6f * Back[Frame]);
        Pose->Head.Y = 0.3f * Back[Frame];
    }
}

internal void
HeroJumpPose(hero_pose *Pose, hero_look *Look, u32 Frame)
{
    Pose->WeaponAngle = HeroRestAngle(Look, Pose->Facing);
    bool32 Side = Pose->Facing == HeroFacing_Right;
    switch(Frame)
    {
        case 0:
        {
            // NOTE(zoubir): crouched to spring
            Pose->Squash = 0.84f;
            Pose->Body.Y = 1.f;
            Pose->HandMain = V2(Side ? -2.f : 1.f, 1.f);
            Pose->HandOff = V2(Side ? -2.f : 1.f, 1.f);
            Pose->Lean = 1.f;
        } break;
        case 1:
        {
            // NOTE(zoubir): leaving the ground, stretched, arms up
            Pose->Squash = 1.08f;
            Pose->FootNear = V2(Side ? -1.f : 0.f, -1.f);
            Pose->FootFar = V2(Side ? -2.f : 0.f, -2.f);
            Pose->HandMain = V2(Side ? 1.f : 1.5f, -5.f);
            Pose->HandOff = V2(Side ? 1.f : 1.5f, -5.f);
            Pose->Hem = -1.f;
            Pose->WeaponAngle -= 0.2f;
        } break;
        case 2:
        {
            // NOTE(zoubir): the top of the jump, knees tucked
            Pose->FootNear = V2(Side ? 2.f : 0.f, -4.5f);
            Pose->FootFar = V2(Side ? 0.f : 0.f, -3.5f);
            Pose->HandMain = V2(2.f, -3.f);
            Pose->HandOff = V2(2.f, -3.f);
            Pose->Body.Y = -1.f;
        } break;
        default:
        {
            // NOTE(zoubir): falling, legs reaching for the ground
            Pose->FootNear = V2(Side ? 1.5f : 0.f, -0.5f);
            Pose->FootFar = V2(Side ? -1.5f : 0.f, -1.5f);
            Pose->HandMain = V2(3.f, -6.f);
            Pose->HandOff = V2(3.f, -6.f);
            Pose->Hem = 1.f;
            Pose->Squash = 1.04f;
            Pose->WeaponAngle += 0.25f;
        } break;
    }
}

internal void
HeroHurtPose(hero_pose *Pose, hero_look *Look, u32 Frame)
{
    float Recoil[3] = {1.f, 0.6f, 0.2f};
    float R = Recoil[Frame];
    Pose->WeaponAngle = HeroRestAngle(Look, Pose->Facing) - 0.4f * R;
    Pose->Squash = 1.f - 0.06f * R;
    Pose->HandMain = V2(-2.f * R, -3.f * R);
    Pose->HandOff = V2(-2.f * R, -3.f * R);
    if (Pose->Facing == HeroFacing_Right)
    {
        Pose->Lean = -3.f * R;
        Pose->Head = V2(-1.f * R, -0.5f * R);
        Pose->FootNear.X = -1.f * R;
    }
    else
    {
        Pose->Head.Y = (Pose->Facing == HeroFacing_Down ? -1.f : 1.f) * R;
        Pose->Body.Y = 0.5f * R;
    }
}

internal void
HeroDeathPose(hero_pose *Pose, hero_look *Look, u32 Frame)
{
    // NOTE(zoubir): knees give, then the body tips over and lies still;
    // side on it falls backward, facing the camera or away to the side
    float Sink[6] = {0.f, 0.86f, 0.76f, 0.8f, 0.9f, 1.f};
    float Tip[6] = {0.f, 0.f, 0.f, 0.6f, 1.25f, 1.57f};
    if (Frame == 0)
    {
        HeroHurtPose(Pose, Look, 0);
        return;
    }
    float Way = Pose->Facing == HeroFacing_Right ? -1.f : 1.f;
    Pose->Squash = Sink[Frame];
    Pose->Body.Y = Frame < 3 ? 1.5f : 0.f;
    Pose->Topple = Way * Tip[Frame];
    Pose->Head.Y = Frame < 3 ? 1.f : 0.f;
    Pose->HandMain = V2(-1.f, 2.f);
    Pose->HandOff = V2(-1.f, 2.f);
    Pose->WeaponDropped = Frame >= 4;
    Pose->WeaponAngle = HeroRestAngle(Look, Pose->Facing) + 0.5f;
}

internal hero_pose
HeroPoseFor(hero_look *Look, hero_anim Anim, hero_facing Facing, u32 Frame)
{
    hero_pose Pose = {};
    Pose.Facing = Facing;
    Pose.Squash = 1.f;
    Pose.WeaponAngle = HeroRestAngle(Look, Facing);
    float t = (float)Frame / (float)HeroAnimDefs[Anim].FrameCount;
    switch(Anim)
    {
        case HeroAnim_Idle:
        {
            // NOTE(zoubir): breathing: down a pixel and back, the orb or
            // blade lifting with the chest
            float Breath = (Frame == 1 || Frame == 2) ? 1.f : 0.f;
            Pose.Body.Y = Breath;
            Pose.HandMain.Y = 0.5f * Breath;
            Pose.HandOff.Y = 0.5f * Breath;
            Pose.Hem = 0.4f * Sin(2.f * Pi32 * t);
        } break;
        case HeroAnim_Walk: { HeroWalkPose(&Pose, Look, t); } break;
        case HeroAnim_Attack: { HeroAttackPose(&Pose, Look, Frame); } break;
        case HeroAnim_Cast: { HeroCastPose(&Pose, Look, Frame); } break;
        case HeroAnim_Skid: { HeroSkidPose(&Pose, Look, Frame); } break;
        case HeroAnim_Jump: { HeroJumpPose(&Pose, Look, Frame); } break;
        case HeroAnim_Hurt: { HeroHurtPose(&Pose, Look, Frame); } break;
        case HeroAnim_Death: { HeroDeathPose(&Pose, Look, Frame); } break;
        case HeroAnim_Count: break;
    }
    return Pose;
}
