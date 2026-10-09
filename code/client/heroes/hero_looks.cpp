/* Hero looks: which drawing a class hero wears on this machine, the
   classic one (heroes/<class>.png, the original hero recoloured, played by
   the original's animation table) or the new one (heroes_v2/<class>.png,
   drawn by misc/hero_looks/). The pick is the player's, in the Esc options
   (ui/options_menu.cpp), saved on the seventh line of server.txt
   (online_config.cpp). It changes only the picture: the simulation plays
   the same animations with the same timing either way.

   The new sheet has more frames than the original, so it does not use the
   animation table's cells. HeroLookCell picks one from what the body is
   doing (its animation, facing and the frame the simulation is on) and
   what it is doing it with: the kind of its last swing (player_swing),
   the spell it winds up, a dash, a fresh hit. Facing left is the right
   cells mirrored, as in the original.

   The new sheet: 16 by 16 cells of 32 pixels, rows counted from the top
   of the PNG. Three facings, each a block of five rows: down (rows 0-4),
   right (5-9), up (10-14). In a block, animation slot S sits in row S / 2,
   columns (S % 2) * 8 onward, one column a frame (hero_look_slot). Row 15
   holds single frames, five columns a facing: jump up, fall, dash, hurt. */

// NOTE(zoubir): the new look first, so a zeroed AppState draws it
enum hero_look
{
    HeroLook_New,
    HeroLook_Classic,
    HeroLook_Count
};

// NOTE(zoubir): the look every hero is drawn in on this machine; a global
// like GlobalControlScheme, mirrored in AppState->HeroLook so it outlives
// a reload of the game code
global_variable hero_look GlobalHeroLook = HeroLook_New;

// NOTE(zoubir): the words server.txt stores
internal char *
HeroLookWord(hero_look Look)
{
    char *Result = (Look == HeroLook_Classic) ? (char *)"classic" : (char *)"new";
    return Result;
}

#define HERO_LOOK_CELLS 16
#define HERO_LOOK_FACING_ROWS 5
#define HERO_LOOK_SLOT_COLUMNS 8
#define HERO_LOOK_SINGLES_ROW 15
#define HERO_LOOK_SINGLES_PER_FACING 5

// NOTE(zoubir): the animations of a facing's block, in sheet order
enum hero_look_slot
{
    HeroLookSlot_Idle,
    HeroLookSlot_Walk,
    HeroLookSlot_Stop,
    HeroLookSlot_SwingCut,
    HeroLookSlot_SwingBackhand,
    HeroLookSlot_SwingHeavy,
    HeroLookSlot_Thrust,
    HeroLookSlot_CastThrust,
    HeroLookSlot_CastRaise,
    HeroLookSlot_CastGround,
    HeroLookSlot_Count
};

global_variable u32 HeroLookSlotFrames[HeroLookSlot_Count] = {4, 4, 2, 6, 6, 6, 6, 4, 4, 4};

// NOTE(zoubir): the single frames of row 15
enum hero_look_single
{
    HeroLookSingle_JumpUp,
    HeroLookSingle_Fall,
    HeroLookSingle_Dash,
    HeroLookSingle_Hurt,
    HeroLookSingle_Count
};

// NOTE(zoubir): how each spell's wind-up is drawn: a hand pushed out at
// the aim, both arms raised over the head, or a crouch striking the floor
enum hero_cast_style
{
    HeroCast_Thrust,
    HeroCast_Raise,
    HeroCast_Ground,
};

internal hero_cast_style
HeroCastStyle(u32 Spell)
{
    hero_cast_style Result = HeroCast_Thrust;
    switch(Spell)
    {
        case PlayerSpell_Shockwave:     { Result = HeroCast_Ground; } break;
        case PlayerSpell_Push:          { Result = HeroCast_Thrust; } break;
        case PlayerSpell_Launch:        { Result = HeroCast_Ground; } break;
        case PlayerSpell_Slam:          { Result = HeroCast_Ground; } break;
        case PlayerSpell_Blink:         { Result = HeroCast_Thrust; } break;
        case PlayerSpell_FrostNova:     { Result = HeroCast_Ground; } break;
        case PlayerSpell_GravityWell:   { Result = HeroCast_Raise; } break;
        case PlayerSpell_Meteor:        { Result = HeroCast_Raise; } break;
        case PlayerSpell_GiantFireball: { Result = HeroCast_Raise; } break;
        // NOTE(zoubir): Piercing Shot draws the bow; Rapid Fire looses it
        case PlayerSpell_RangerA:       { Result = HeroCast_Thrust; } break;
        case PlayerSpell_RangerB:       { Result = HeroCast_Thrust; } break;
        // NOTE(zoubir): Whirlwind spins low; Execute comes down from overhead
        case PlayerSpell_BerserkerA:    { Result = HeroCast_Ground; } break;
        case PlayerSpell_BerserkerB:    { Result = HeroCast_Raise; } break;
        // NOTE(zoubir): Fan of Knives is a crouch; Eviscerate raises the daggers
        case PlayerSpell_ShadowbladeA:  { Result = HeroCast_Ground; } break;
        case PlayerSpell_ShadowbladeB:  { Result = HeroCast_Raise; } break;
        // NOTE(zoubir): Chain Lightning is thrown; Thunderclap hits the floor
        case PlayerSpell_StormcallerA:  { Result = HeroCast_Thrust; } break;
        case PlayerSpell_StormcallerB:  { Result = HeroCast_Ground; } break;
        // NOTE(zoubir): Heartseeker draws the point back to strike
        case PlayerSpell_DuelistA:      { Result = HeroCast_Thrust; } break;
        case PlayerSpell_DuelistB:      { Result = HeroCast_Thrust; } break;
        case PlayerSpell_FrostMageA:    { Result = HeroCast_Thrust; } break;
        case PlayerSpell_FrostMageB:    { Result = HeroCast_Raise; } break;
        // NOTE(zoubir): Starfire calls a star down; Tranquility is a channel
        case PlayerSpell_DruidA:        { Result = HeroCast_Raise; } break;
        case PlayerSpell_DruidB:        { Result = HeroCast_Raise; } break;
        case PlayerSpell_RewindSelf:    { Result = HeroCast_Raise; } break;
        case PlayerSpell_RewindBubble:  { Result = HeroCast_Raise; } break;
        case PlayerSpell_RewindWorld:   { Result = HeroCast_Raise; } break;
        // NOTE(zoubir): no spell: the plain fireball, thrown
        default: break;
    }
    return Result;
}

// NOTE(zoubir): what the body is doing, as HeroLookCell reads it
struct hero_look_input
{
    animation_type Type;
    animation_direction Facing;
    u32 SlotIndex;
    u32 Swing;   // player_swing
    u32 Spell;   // player_spell being wound up, PlayerSpell_None for none
    bool32 Dashing;
    bool32 Hurt;
};

// NOTE(zoubir): a cell of the new sheet, PNG column and row from the top,
// and whether it is drawn mirrored (facing left)
struct hero_look_cell
{
    u32 Column;
    u32 Row;
    bool32 Mirrored;
};

// NOTE(zoubir): the block a facing's frames are in; left is right mirrored
inline u32
HeroLookFacingBlock(animation_direction Facing)
{
    u32 Result = Facing == AnimationDirection_Down ? 0 :
        Facing == AnimationDirection_Up ? 2 : 1;
    return Result;
}

inline hero_look_cell
HeroLookSlotCell(u32 Block, hero_look_slot Slot, u32 Frame)
{
    Assert(Frame < HeroLookSlotFrames[Slot]);
    hero_look_cell Result = {};
    Result.Column = (Slot % 2) * HERO_LOOK_SLOT_COLUMNS + Frame;
    Result.Row = Block * HERO_LOOK_FACING_ROWS + Slot / 2;
    return Result;
}

inline hero_look_cell
HeroLookSingleCell(u32 Block, hero_look_single Single)
{
    hero_look_cell Result = {};
    Result.Column = Block * HERO_LOOK_SINGLES_PER_FACING + Single;
    Result.Row = HERO_LOOK_SINGLES_ROW;
    return Result;
}

internal hero_look_cell
HeroLookCell(hero_look_input *In)
{
    u32 Block = HeroLookFacingBlock(In->Facing);
    u32 Slot = In->SlotIndex;
    hero_look_cell Result = {};
    bool32 Acting = In->Type == AnimationType_Attack || In->Type == AnimationType_Cast;
    if (In->Hurt && !Acting)
    {
        Result = HeroLookSingleCell(Block, HeroLookSingle_Hurt);
    }
    else if (In->Dashing && !Acting)
    {
        Result = HeroLookSingleCell(Block, HeroLookSingle_Dash);
    }
    else
    {
        switch(In->Type)
        {
            case AnimationType_Move:
            {
                Result = HeroLookSlotCell(Block, HeroLookSlot_Walk, Slot % 4);
            } break;
            case AnimationType_Stop:
            {
                Result = HeroLookSlotCell(Block, HeroLookSlot_Stop, Minimum(Slot, 3u) / 2);
            } break;
            case AnimationType_Attack:
            {
                hero_look_slot Swing = In->Swing == PlayerSwing_Backhand ? HeroLookSlot_SwingBackhand :
                    In->Swing == PlayerSwing_Heavy ? HeroLookSlot_SwingHeavy :
                    In->Swing == PlayerSwing_Thrust ? HeroLookSlot_Thrust : HeroLookSlot_SwingCut;
                Result = HeroLookSlotCell(Block, Swing, Slot % 6);
            } break;
            case AnimationType_Cast:
            {
                hero_cast_style Style = HeroCastStyle(In->Spell);
                hero_look_slot Cast = Style == HeroCast_Raise ? HeroLookSlot_CastRaise :
                    Style == HeroCast_Ground ? HeroLookSlot_CastGround : HeroLookSlot_CastThrust;
                // NOTE(zoubir): a long wind-up holds its last frame; the
                // raised arms keep the two glowing frames going
                u32 Frame = Slot < 4 ? Slot : (Style == HeroCast_Raise ? 2 + Slot % 2 : 3);
                Result = HeroLookSlotCell(Block, Cast, Frame);
            } break;
            case AnimationType_JumpUp:
            {
                Result = HeroLookSingleCell(Block, HeroLookSingle_JumpUp);
            } break;
            case AnimationType_JumpDown:
            {
                Result = HeroLookSingleCell(Block, HeroLookSingle_Fall);
            } break;
            default:
            {
                Result = HeroLookSlotCell(Block, HeroLookSlot_Idle, Slot % 4);
            } break;
        }
    }
    Result.Mirrored = In->Facing == AnimationDirection_Left;
    return Result;
}

// NOTE(zoubir): the texture coordinates of Cell, like GetAnimationUvs
// gives them: index row 0 is the PNG's bottom row (the sheet reaches the
// GPU bottom up), a mirrored cell has its U swapped
internal v4
HeroLookCellUvs(hero_look_cell Cell, u32 TextureWidth, u32 TextureHeight)
{
    u32 Index = (HERO_LOOK_CELLS - 1 - Cell.Row) * HERO_LOOK_CELLS + Cell.Column;
    v4 Result = GetTextureUvsFromIndex(TextureWidth, TextureHeight,
                                       HERO_LOOK_CELLS, HERO_LOOK_CELLS, Index);
    if (Cell.Mirrored)
    {
        float Tmp = Result.X;
        Result.X = Result.Z;
        Result.Z = Tmp;
    }
    return Result;
}

// NOTE(zoubir): a hit makes the body flinch (body_pose.cpp); over this
// much of it the new look shows its hurt frame
#define HERO_LOOK_HURT_FLINCH 0.45f

internal bool32
HeroLookHurt(app_state *AppState, world_entity *Entity)
{
    u32 Index = (u32)(Entity - AppState->World.Entities);
    body_pose *Pose = (AppState->BodyPoses && Index < BODY_POSE_SLOTS) ?
        &AppState->BodyPoses->Poses[Index] : 0;
    bool32 Result = Pose && Pose->Tracking && Pose->EntityId == Entity->ID &&
        Pose->Flinch > HERO_LOOK_HURT_FLINCH;
    return Result;
}
