/* Hero skin frames (hero_skins.cpp): which cell of a skin's sheet a
   player shows this frame. The animation comes from what the simulation
   plays (walk, stand, attack, cast, skid, jump), a fresh hit (hurt) or a
   downed body (death); the frame from how far through the simulation's
   own animation the body is, so the timing stays the simulation's. */

// NOTE(zoubir): in client/fx_bursts.cpp, included later
internal float GetFxClock(app_state *AppState);

#define HERO_DEATH_SECONDS_PER_FRAME 0.11f

// NOTE(zoubir): per player slot, when its body was first seen down
global_variable float HeroDownedAt[MAX_PLAYERS];
global_variable bool32 HeroWasDown[MAX_PLAYERS];

struct hero_skin_pick
{
    hero_anim Anim;
    u32 Frame;
};

// NOTE(zoubir): 0..1 through the simulation's current animation
internal float
HeroSimProgress(world_entity *Entity, animation_state *Shown)
{
    float Result = 0.f;
    animation_slot *Slot = GetAnimation(Entity->AnimationSet, Shown->CurrentType,
                                        Shown->LastAnimationDirection);
    if (Slot->IndicesCount)
    {
        u32 Count = Slot->IndicesCount;
        u32 Index = Shown->SlotIndex % Count;
        float Seconds = Slot->FramesTimeInSeconds[Index];
        float Within = Seconds > 0.f ? ArtClamp01(Shown->DeltaTime / Seconds) : 0.f;
        Result = ((float)Index + Within) / (float)Count;
    }
    return Result;
}

internal hero_skin_pick
HeroSkinPick(app_state *AppState, world_entity *Entity, animation_state *Shown,
             hero_skin_sheet *Sheet)
{
    hero_skin_pick Pick = {HeroAnim_Idle, 0};
    float Clock = GetFxClock(AppState);
    u32 SlotIndex = Entity->PlayerIndex;
    if (IsDeadPlayer(Entity))
    {
        if (!HeroWasDown[SlotIndex])
        {
            HeroWasDown[SlotIndex] = true;
            HeroDownedAt[SlotIndex] = Clock;
        }
        Pick.Anim = HeroAnim_Death;
        Pick.Frame = (u32)((Clock - HeroDownedAt[SlotIndex]) / HERO_DEATH_SECONDS_PER_FRAME);
    }
    else
    {
        HeroWasDown[SlotIndex] = false;
        float Progress = HeroSimProgress(Entity, Shown);
        switch(Shown->CurrentType)
        {
            case AnimationType_Move: { Pick.Anim = HeroAnim_Walk; } break;
            case AnimationType_Attack: { Pick.Anim = HeroAnim_Attack; } break;
            case AnimationType_Cast: { Pick.Anim = HeroAnim_Cast; } break;
            case AnimationType_Stop: { Pick.Anim = HeroAnim_Skid; } break;
            case AnimationType_JumpUp:
            case AnimationType_JumpDown:
            {
                // NOTE(zoubir): rising shows the take-off, falling the
                // frame reaching for the ground
                Pick.Anim = HeroAnim_Jump;
                u32 Last = Sheet->Frames[HeroAnim_Jump] - 1;
                Progress = Shown->CurrentType == AnimationType_JumpUp ?
                    (float)((2 * Last + 2) / 5) / (float)(Last + 1) : 1.f;
            } break;
            default:
            {
                // NOTE(zoubir): breathing on the clock, each player on
                // its own beat
                Pick.Anim = HeroAnim_Idle;
                Progress = (Clock + 0.37f * (float)SlotIndex) /
                    (Sheet->IdleSeconds * (float)Sheet->Frames[HeroAnim_Idle]);
                Progress -= (float)(u32)Progress;
            } break;
        }
        // NOTE(zoubir): a fresh hit flinches, unless a swing or cast is
        // under way
        body_pose_draw Pose = GetBodyPose(AppState, Entity);
        if (Pose.Flash > 0.f && Pick.Anim != HeroAnim_Attack && Pick.Anim != HeroAnim_Cast)
        {
            Pick.Anim = HeroAnim_Hurt;
            Progress = 1.f - Pose.Flash;
        }
        Pick.Frame = (u32)(Progress * (float)Sheet->Frames[Pick.Anim]);
    }
    u32 Frames = Sheet->Frames[Pick.Anim];
    Pick.Frame = Minimum(Pick.Frame, Frames ? Frames - 1 : 0);
    return Pick;
}

// NOTE(zoubir): the texture and UVs of one cell of a skin's sheet; Facing
// in the sheets' order: down, up, right, left
internal v4
HeroSkinCellUvs(assets *Assets, u32 Role, u32 Skin, hero_skin_sheet *Sheet, hero_anim Anim,
                u32 Facing, u32 Frame, asset_id *ID)
{
    *ID = HeroSkinAsset(Role, Skin);
    u32 Columns = HERO_SHEET_COLUMNS;
    u32 Row = 0;
    bool32 Mirror = false;
    if (Anim == HeroAnim_Attack && Sheet->WideAttack)
    {
        *ID = HeroWideAsset(Role);
        Columns = Sheet->Frames[HeroAnim_Attack];
        Row = Facing;
    }
    else if (Sheet->Lpc)
    {
        // NOTE(zoubir): hurt and death are drawn facing down only, one
        // row each after the four-facing rows
        Row = Anim < HeroAnim_Hurt ? (u32)Anim * 4 + Facing :
            HeroAnim_Hurt * 4 + (Anim - HeroAnim_Hurt);
    }
    else
    {
        // NOTE(zoubir): drawn in code facing right only; left is mirrored
        if (Facing == 3)
        {
            Facing = 2;
            Mirror = true;
        }
        Row = HeroSheetRow(Anim, (hero_facing)Facing);
    }
    zas_texture_info *Info = &GetAssetInfo(Assets, *ID)->Texture;
    v4 Result = GetTextureUvsFromIndex(Info->Width, Info->Height, Info->NumTilesX,
                                       Info->NumTilesY, Row * Columns + Frame);
    if (Mirror)
    {
        float Swap = Result.X;
        Result.X = Result.Z;
        Result.Z = Swap;
    }
    return Result;
}

// NOTE(zoubir): a player wearing a skin gets that skin's texture and the
// frame's UVs; anyone else goes back to the original hero. False when
// the original hero's table should pick the frame
internal bool32
UpdateHeroSkinUvs(app_state *AppState, assets *Assets, world_entity *Entity,
                  animation_state *Shown)
{
    u32 Role = 0;
    u32 Skin = 0;
    hero_skin_sheet *Sheet = HeroSkinSheetOf(AppState, Entity, &Role, &Skin);
    if (!Sheet)
    {
        if (Entity->Texture.Type == AssetType_HeroSkin)
        {
            Entity->Texture = {AssetType_Zoubir};
        }
        return false;
    }
    hero_skin_pick Pick = HeroSkinPick(AppState, Entity, Shown, Sheet);
    u32 Facing = 0;
    switch(Shown->LastAnimationDirection)
    {
        case AnimationDirection_Up: { Facing = 1; } break;
        case AnimationDirection_Right: { Facing = 2; } break;
        case AnimationDirection_Left: { Facing = 3; } break;
        default: break;
    }
    Entity->Uvs = HeroSkinCellUvs(Assets, Role, Skin, Sheet, Pick.Anim, Facing, Pick.Frame,
                                  &Entity->Texture);
    return true;
}

// NOTE(zoubir): the size a sprite is drawn at: a skin's cell in world
// units, else the entity's own
inline v2
SpriteDrawDimensions(world_entity *Entity)
{
    v2 Result = Entity->Dimensions;
    if (Entity->Texture.Type == AssetType_HeroSkin && Entity->Texture.Index < HERO_SKIN_ASSETS)
    {
        float Size = HeroSkinDrawSizes[Entity->Texture.Index];
        Result = V2(Size, Size);
    }
    return Result;
}

// NOTE(zoubir): a downed player wearing a skin lies where they fell, so
// the party can see who to revive
internal bool32
DrawsDownedHero(app_state *AppState, world_entity *Entity)
{
    u32 Role = 0;
    u32 Skin = 0;
    bool32 Result = IsDeadPlayer(Entity) && HeroSkinSheetOf(AppState, Entity, &Role, &Skin) != 0;
    return Result;
}
