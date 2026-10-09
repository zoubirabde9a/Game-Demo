/* Hero rig: one body drawn from a handful of joints, shared by every
   class. A class only says how it is dressed and what it holds
   (hero_look, hero_looks.cpp); an animation only says where the joints
   go on each frame (hero_pose, hero_anims.cpp). Three facings are drawn:
   down (toward the camera), up (away) and right; left is right mirrored
   when drawn. Frames are HERO_FRAME_SIZE pixels square with the feet on
   HERO_FEET_Y, the 7/8 line, like the monsters. */

#define HERO_FRAME_SIZE 48
#define HERO_FEET_Y 42.f
#define HERO_MID_X 24.f

enum hero_facing
{
    HeroFacing_Down,
    HeroFacing_Up,
    HeroFacing_Right,
    HeroFacing_Count,
};

enum hero_headgear
{
    HeroHeadgear_None,
    HeroHeadgear_WizardHat,
    HeroHeadgear_Helmet,
    HeroHeadgear_Circlet,
    HeroHeadgear_Hood,
    HeroHeadgear_Horns,
    HeroHeadgear_Cavalier,
};

enum hero_weapon
{
    HeroWeapon_Staff,
    HeroWeapon_Sword,
    // NOTE(zoubir): empty hands, for a class whose own look draws its
    // weapon over the sprite (client/dungeon/classes/)
    HeroWeapon_None,
};

enum hero_offhand
{
    HeroOffhand_None,
    HeroOffhand_Shield,
};

struct hero_look
{
    color_ramp Skin;
    color_ramp Hair;
    // NOTE(zoubir): the tunic or robe, its trim and the belt
    color_ramp Cloth;
    color_ramp Trim;
    color_ramp Belt;
    // NOTE(zoubir): sleeves and leggings; plate for armoured classes
    color_ramp Limb;
    // NOTE(zoubir): trousers when they differ from the sleeves; left zero
    // they are the sleeves' colour
    color_ramp Legs;
    color_ramp Boots;
    color_ramp Hat;
    color_ramp HatBand;
    // NOTE(zoubir): the staff's shaft or the sword's blade, and the orb or
    // the hilt
    color_ramp Shaft;
    color_ramp Head;
    color_ramp ShieldRim;
    color_ramp ShieldField;
    // NOTE(zoubir): casts, impacts and the orb's light
    u32 Glow;
    u32 GlowCore;
    bool32 Robe;
    bool32 LongHair;
    bool32 Beard;
    bool32 Mustache;
    bool32 SpikyHair;
    // NOTE(zoubir): a cloth over the mouth and nose, in the hat's colour
    bool32 Mask;
    bool32 Pauldrons;
    hero_headgear Headgear;
    hero_weapon Weapon;
    hero_offhand Offhand;
};

// NOTE(zoubir): where the joints are on one frame, as offsets from the
// standing pose; hero_anims.cpp fills it in
struct hero_pose
{
    hero_facing Facing;
    // NOTE(zoubir): the whole upper body (hips up), and how far it leans
    // forward along the facing
    v2 Body;
    float Lean;
    // NOTE(zoubir): 1 standing, below 1 crouched, above 1 stretched, about
    // the feet
    float Squash;
    // NOTE(zoubir): Y below 0 lifts a foot, X moves it along the facing
    // (side view) or sideways (front and back)
    v2 FootNear;
    v2 FootFar;
    // NOTE(zoubir): hands as offsets from where they hang at rest
    v2 HandMain;
    v2 HandOff;
    // NOTE(zoubir): the weapon's direction on screen, 0 along the facing,
    // -Pi/2 straight up; for the down and up facings 0 is screen right
    float WeaponAngle;
    v2 Head;
    // NOTE(zoubir): robe hem swing, pixels
    float Hem;
    float Glow;
    float Impact;
    // NOTE(zoubir): radians the whole body turns about the feet (falling)
    float Topple;
    bool32 WeaponDropped;
    // NOTE(zoubir): skid dust, 0 none, else 1..3 growing puffs
    u32 Dust;
};

// NOTE(zoubir): a rig point moved by the body's squash and topple
internal v2
HeroPoint(hero_pose *Pose, v2 P)
{
    v2 Feet = V2(HERO_MID_X, HERO_FEET_Y);
    v2 D = P - Feet;
    D.Y *= Pose->Squash;
    float C = Cos(Pose->Topple);
    float S = Sin(Pose->Topple);
    v2 Result = Feet + V2(C * D.X - S * D.Y, S * D.X + C * D.Y);
    // NOTE(zoubir): a body lying down is longer than half the frame, so
    // it slides back to keep its middle in the frame
    Result.X -= 17.f * S;
    return Result;
}

inline void
HeroBlob(sprite_canvas *Canvas, hero_pose *Pose, v2 C, float RX, float RY,
         color_ramp Ramp, float Bias = 0.f)
{
    v2 P = HeroPoint(Pose, C);
    // NOTE(zoubir): lying down, a blob's width and height swap over
    float Lying = Absolute(Sin(Pose->Topple));
    float W = Lerp(RX, Lying, RY);
    float H = Lerp(RY, Lying, RX) * Lerp(Pose->Squash, Lying, 1.f);
    FillBlob(Canvas, P.X, P.Y, W, H, Ramp, Bias);
}

inline void
HeroLimb(sprite_canvas *Canvas, hero_pose *Pose, v2 A, v2 B, float RA, float RB,
         color_ramp Ramp, float Bias = 0.f)
{
    FillLimb(Canvas, HeroPoint(Pose, A), HeroPoint(Pose, B), RA, RB, Ramp, Bias);
}

inline void
HeroTriangle(sprite_canvas *Canvas, hero_pose *Pose, v2 A, v2 B, v2 C,
             color_ramp Ramp, float Light0, float Light1)
{
    FillTriangle(Canvas, HeroPoint(Pose, A), HeroPoint(Pose, B), HeroPoint(Pose, C),
                 Ramp, Light0, Light1);
}

inline void
HeroDot(sprite_canvas *Canvas, hero_pose *Pose, v2 C, float R, u32 Color)
{
    v2 P = HeroPoint(Pose, C);
    FillDot(Canvas, P.X, P.Y, R, Color);
}

inline void
HeroPixel(sprite_canvas *Canvas, hero_pose *Pose, v2 C, u32 Color)
{
    v2 P = HeroPoint(Pose, C);
    PutPixel(Canvas, (i32)P.X, (i32)P.Y, Color);
}

// NOTE(zoubir): where the body's parts sit on this frame. Side is +1 for
// the main hand's side of the screen, so front and back views mirror
struct hero_joints
{
    v2 Hips;
    v2 Torso;
    v2 Neck;
    v2 Head;
    v2 ShoulderMain;
    v2 ShoulderOff;
    v2 HandMain;
    v2 HandOff;
    v2 FootNear;
    v2 FootFar;
    v2 HipNear;
    v2 HipFar;
    float Width;
};

internal hero_joints
HeroJoints(hero_pose *Pose)
{
    hero_joints J = {};
    v2 Up = Pose->Body;
    bool32 Side = Pose->Facing == HeroFacing_Right;
    // NOTE(zoubir): facing down the main (right) hand is on screen left;
    // facing up it is on screen right
    float MainX = Pose->Facing == HeroFacing_Down ? -1.f : 1.f;
    float Lean = Side ? Pose->Lean : 0.f;
    J.Width = Side ? 4.5f : 6.f;
    J.Hips = V2(HERO_MID_X, 34.f) + Up;
    J.Torso = V2(HERO_MID_X + 0.5f * Lean, 29.5f) + Up;
    J.Neck = V2(HERO_MID_X + 0.8f * Lean, 24.5f) + Up;
    J.Head = V2(HERO_MID_X + (Side ? 0.5f : 0.f) + Lean, 17.5f) + Up + Pose->Head;
    if (Side)
    {
        J.ShoulderMain = J.Neck + V2(0.f, 1.5f);
        J.ShoulderOff = J.Neck + V2(-0.5f, 1.f);
        J.HandMain = J.ShoulderMain + V2(2.5f, 6.5f) + Pose->HandMain;
        J.HandOff = J.ShoulderOff + V2(0.f, 6.5f) + Pose->HandOff;
        J.HipNear = J.Hips + V2(0.5f, 0.f);
        J.HipFar = J.Hips + V2(-0.5f, -0.5f);
        J.FootNear = V2(HERO_MID_X + 0.5f, HERO_FEET_Y - 0.5f) + Pose->FootNear;
        J.FootFar = V2(HERO_MID_X - 0.5f, HERO_FEET_Y - 1.f) + Pose->FootFar;
    }
    else
    {
        J.ShoulderMain = J.Neck + V2(MainX * 5.5f, 1.5f);
        J.ShoulderOff = J.Neck + V2(-MainX * 5.5f, 1.5f);
        J.HandMain = J.ShoulderMain + V2(MainX * 1.f, 7.f) +
            V2(MainX * Pose->HandMain.X, Pose->HandMain.Y);
        J.HandOff = J.ShoulderOff + V2(-MainX * 1.f, 7.f) +
            V2(-MainX * Pose->HandOff.X, Pose->HandOff.Y);
        J.HipNear = J.Hips + V2(-2.5f, 0.f);
        J.HipFar = J.Hips + V2(2.5f, 0.f);
        J.FootNear = V2(HERO_MID_X - 3.5f, HERO_FEET_Y - 0.5f) + Pose->FootNear;
        J.FootFar = V2(HERO_MID_X + 3.5f, HERO_FEET_Y - 0.5f) +
            V2(-Pose->FootFar.X, Pose->FootFar.Y);
    }
    return J;
}

internal void
DrawHeroLeg(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 Hip, v2 Foot,
            float Bias)
{
    v2 Ankle = Foot + V2(0.f, -1.5f);
    if (!Look->Robe)
    {
        color_ramp Legs = Look->Legs.C[3] ? Look->Legs : Look->Limb;
        HeroLimb(Canvas, Pose, Hip, Ankle, 2.2f, 1.8f, Legs, Bias);
    }
    float Forward = Pose->Facing == HeroFacing_Right ? 1.f : 0.f;
    HeroBlob(Canvas, Pose, Foot + V2(Forward, -0.5f), 2.4f + Forward, 1.8f, Look->Boots, Bias);
}

internal void
DrawHeroArm(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, v2 Shoulder, v2 Hand,
            float Bias)
{
    HeroLimb(Canvas, Pose, Shoulder, Hand, 2.1f, 1.6f, Look->Robe ? Look->Cloth : Look->Limb, Bias);
    if (Look->Robe)
    {
        // NOTE(zoubir): a wide sleeve cuff, then the hand
        v2 Cuff = Lerp2(Shoulder, 0.78f, Hand);
        HeroBlob(Canvas, Pose, Cuff, 2.2f, 2.f, Look->Trim, Bias);
    }
    HeroBlob(Canvas, Pose, Hand, 1.7f, 1.7f, Look->Robe ? Look->Skin : Look->Limb, Bias);
}

// NOTE(zoubir): the torso, the robe's skirt or the tunic's tabard, the
// belt; Back hides the front's trim
internal void
DrawHeroTorso(sprite_canvas *Canvas, hero_pose *Pose, hero_look *Look, hero_joints *J)
{
    bool32 Side = Pose->Facing == HeroFacing_Right;
    bool32 Back = Pose->Facing == HeroFacing_Up;
    float W = J->Width;
    if (Look->Robe)
    {
        // NOTE(zoubir): the skirt flares from the waist to the hem
        float HemW = Side ? 6.5f : 8.f;
        v2 Waist = J->Hips + V2(0.f, -1.f);
        v2 HemL = V2(J->Hips.X - HemW + Pose->Hem, HERO_FEET_Y - 2.f);
        v2 HemR = V2(J->Hips.X + HemW + Pose->Hem, HERO_FEET_Y - 2.f);
        HeroTriangle(Canvas, Pose, Waist + V2(-W, 0.f), HemL, HemR, Look->Cloth, 0.7f, 0.35f);
        HeroTriangle(Canvas, Pose, Waist + V2(-W, 0.f), Waist + V2(W, 0.f), HemR, Look->Cloth, 0.7f, 0.45f);
        // NOTE(zoubir): a trim band along the hem and down the front
        HeroLimb(Canvas, Pose, HemL + V2(0.5f, -0.5f), HemR + V2(-0.5f, -0.5f), 1.f, 1.f, Look->Trim);
        if (!Back)
        {
            float FrontX = Side ? 2.f : 0.f;
            HeroLimb(Canvas, Pose, Waist + V2(FrontX, 1.f),
                     V2(J->Hips.X + FrontX + Pose->Hem, HERO_FEET_Y - 2.5f), 0.8f, 0.8f, Look->Trim);
        }
    }
    HeroBlob(Canvas, Pose, J->Torso, W, 5.5f, Look->Cloth);
    if (!Look->Robe)
    {
        // NOTE(zoubir): a tabard over the chest, hanging past the belt
        float TW = Side ? 2.5f : 3.5f;
        v2 Top = J->Torso + V2(Side ? 1.5f : 0.f, -3.5f);
        v2 Low = J->Hips + V2(Side ? 1.5f : 0.f, 4.5f);
        HeroTriangle(Canvas, Pose, Top + V2(-TW, 0.f), Top + V2(TW, 0.f), Low, Look->Trim, 0.75f, 0.4f);
        HeroLimb(Canvas, Pose, Top, Low + V2(0.f, -2.f), 1.4f, 1.f, Look->Trim, 0.15f);
    }
    // NOTE(zoubir): the belt and its buckle
    HeroLimb(Canvas, Pose, J->Hips + V2(-W + 0.5f, -1.5f), J->Hips + V2(W - 0.5f, -1.5f),
             1.1f, 1.1f, Look->Belt);
    if (!Back)
    {
        HeroDot(Canvas, Pose, J->Hips + V2(Side ? 2.5f : 0.f, -1.5f), 1.f, Look->HatBand.C[3]);
    }
    if (Look->Pauldrons)
    {
        float Off = Side ? 0.f : 5.f;
        if (Side)
        {
            HeroBlob(Canvas, Pose, J->Neck + V2(0.f, 2.f), 3.5f, 2.6f, Look->Cloth, 0.1f);
        }
        else
        {
            HeroBlob(Canvas, Pose, J->Neck + V2(-Off, 2.f), 3.f, 2.6f, Look->Cloth, 0.1f);
            HeroBlob(Canvas, Pose, J->Neck + V2(Off, 2.f), 3.f, 2.6f, Look->Cloth, 0.1f);
        }
    }
}

#include "hero_head.cpp"
#include "hero_gear.cpp"

// NOTE(zoubir): one whole frame: parts drawn back to front for the facing
internal void
DrawHero(sprite_canvas *Canvas, hero_look *Look, hero_pose *Pose)
{
    hero_joints J = HeroJoints(Pose);
    float Far = -0.18f;
    switch(Pose->Facing)
    {
        case HeroFacing_Down:
        {
            DrawHeroLeg(Canvas, Pose, Look, J.HipNear, J.FootNear, 0.f);
            DrawHeroLeg(Canvas, Pose, Look, J.HipFar, J.FootFar, 0.f);
            DrawHeroTorso(Canvas, Pose, Look, &J);
            DrawHeroArm(Canvas, Pose, Look, J.ShoulderOff, J.HandOff, 0.f);
            DrawHeroHead(Canvas, Pose, Look, &J);
            DrawHeroWeapon(Canvas, Pose, Look, &J);
            DrawHeroArm(Canvas, Pose, Look, J.ShoulderMain, J.HandMain, 0.f);
            DrawHeroShield(Canvas, Pose, Look, &J);
        } break;
        case HeroFacing_Up:
        {
            // NOTE(zoubir): what is held in front is behind the body here
            DrawHeroShield(Canvas, Pose, Look, &J);
            DrawHeroWeapon(Canvas, Pose, Look, &J);
            DrawHeroArm(Canvas, Pose, Look, J.ShoulderMain, J.HandMain, Far);
            DrawHeroArm(Canvas, Pose, Look, J.ShoulderOff, J.HandOff, Far);
            DrawHeroLeg(Canvas, Pose, Look, J.HipNear, J.FootNear, 0.f);
            DrawHeroLeg(Canvas, Pose, Look, J.HipFar, J.FootFar, 0.f);
            DrawHeroTorso(Canvas, Pose, Look, &J);
            DrawHeroHead(Canvas, Pose, Look, &J);
        } break;
        case HeroFacing_Right:
        case HeroFacing_Count:
        {
            DrawHeroArm(Canvas, Pose, Look, J.ShoulderOff, J.HandOff, Far);
            DrawHeroLeg(Canvas, Pose, Look, J.HipFar, J.FootFar, Far);
            DrawHeroLeg(Canvas, Pose, Look, J.HipNear, J.FootNear, 0.f);
            DrawHeroTorso(Canvas, Pose, Look, &J);
            DrawHeroShield(Canvas, Pose, Look, &J);
            DrawHeroHead(Canvas, Pose, Look, &J);
            DrawHeroWeapon(Canvas, Pose, Look, &J);
            DrawHeroArm(Canvas, Pose, Look, J.ShoulderMain, J.HandMain, 0.f);
        } break;
    }
    DrawHeroFx(Canvas, Pose, Look, &J);
    OutlineFrame(Canvas, ART_RGB(22, 16, 26));
    DrawHeroDust(Canvas, Pose);
}
