/* Hero art: the player skins drawn in code, one sheet per class. A sheet is
   HERO_SHEET_COLUMNS frames wide and has one row per animation and facing
   (hero_anims.cpp), HERO_FRAME_SIZE pixels a cell. The client turns them
   into textures and picks the frame (client/heroes/hero_skins.cpp). */

#include "hero_rig.cpp"
#include "hero_anims.cpp"
#include "hero_looks.cpp"
#include "hero_looks_classes.cpp"

#define HERO_SHEET_ROWS (HeroAnim_Count * HeroFacing_Count)

inline u32
HeroSheetRow(hero_anim Anim, hero_facing Facing)
{
    u32 Result = (u32)Anim * HeroFacing_Count + (u32)Facing;
    return Result;
}

// NOTE(zoubir): a class's look; the Fire Mage's for an unknown one
internal hero_look
HeroLookFor(u32 Role)
{
    hero_look Result = FireMageLook();
    switch(Role)
    {
        case PlayerRole_Tank:        { Result = BulwarkLook(); } break;
        case PlayerRole_Healer:      { Result = MenderLook(); } break;
        case PlayerRole_Ranger:      { Result = RangerLook(); } break;
        case PlayerRole_Berserker:   { Result = BerserkerLook(); } break;
        case PlayerRole_Shadowblade: { Result = ShadowbladeLook(); } break;
        case PlayerRole_Stormcaller: { Result = StormcallerLook(); } break;
        case PlayerRole_Duelist:     { Result = DuelistLook(); } break;
        case PlayerRole_FrostMage:   { Result = FrostMageLook(); } break;
        case PlayerRole_Druid:       { Result = DruidLook(); } break;
        default: break;
    }
    return Result;
}

// NOTE(zoubir): Pixels holds (HERO_SHEET_COLUMNS * HERO_FRAME_SIZE) *
// (HERO_SHEET_ROWS * HERO_FRAME_SIZE) u32s
internal void
BuildHeroSheet(u32 Role, u32 *Pixels)
{
    u32 Width = HERO_SHEET_COLUMNS * HERO_FRAME_SIZE;
    ZeroSize(Pixels, Width * HERO_SHEET_ROWS * HERO_FRAME_SIZE * sizeof(u32));
    hero_look Look = HeroLookFor(Role);
    for(u32 Anim = 0; Anim < HeroAnim_Count; Anim++)
    {
        for(u32 Facing = 0; Facing < HeroFacing_Count; Facing++)
        {
            for(u32 Frame = 0; Frame < HeroAnimDefs[Anim].FrameCount; Frame++)
            {
                sprite_canvas Canvas = CanvasFrame(Pixels, Width, HERO_FRAME_SIZE, Frame,
                                                   HeroSheetRow((hero_anim)Anim, (hero_facing)Facing));
                hero_pose Pose = HeroPoseFor(&Look, (hero_anim)Anim, (hero_facing)Facing, Frame);
                DrawHero(&Canvas, &Look, &Pose);
            }
        }
    }
}
