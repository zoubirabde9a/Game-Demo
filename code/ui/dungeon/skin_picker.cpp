/* Skin picker (dungeon_hud.cpp): under the class picker, a card per
   skin of the picked class (client/heroes/hero_skins.cpp) with the
   hero walking toward the camera in it and its name; the worn one is
   outlined in the class's colour. Offline the pick takes at once; online
   it rides with the class in the role request (role_requests.cpp). */

#define SKIN_CARD_WIDTH 104.f
#define SKIN_CARD_HEIGHT 112.f
#define SKIN_CARD_SPRITE 76.f

global_variable char *HeroSkinNames[HeroSkin_Count] = {"Chibi", "Heroic"};

// NOTE(zoubir): how tall the picker is for the class the player has, 0
// when it has no skins to pick from
internal float
SkinPickerHeight(app_state *AppState, u32 Role)
{
    float Result = 0.f;
    if (Role < HERO_SKIN_CLASSES)
    {
        Result = UILineHeight(AppState->Fonts.Body) + UI_GAP_SMALL + SKIN_CARD_HEIGHT + UI_GAP;
    }
    return Result;
}

// NOTE(zoubir): the skin's walk toward the camera, Size square at X, Y
internal void
DrawSkinPreview(render_context *RenderContext, app_state *AppState, u32 Role, u32 Skin,
                float X, float Y, float Size)
{
    hero_skin_sheet *Sheet = &HeroSkinSheets[Role][Skin];
    assets *Assets = &AppState->Assets;
    u32 Frames = Sheet->Frames[HeroAnim_Walk];
    u32 Frame = (u32)(GetFxClock(AppState) / 0.11f) % Maximum(1u, Frames);
    asset_id ID = {};
    v4 Cell = HeroSkinCellUvs(Assets, Role, Skin, Sheet, HeroAnim_Walk, 0, Frame, &ID);
    // NOTE(zoubir): the screen pass counts V from the texture's bottom
    // (ability_icons.cpp), the world pass from its top
    v4 Uvs = V4(Cell.X, 1.f - Cell.W, Cell.Z, 1.f - Cell.Y);
    loaded_texture *Texture = GetTexture(Assets, AppState->OpenGL, AppState, ID);
    if (Texture)
    {
        BeginBatch(RenderContext, Texture->ID, 0.f, RenderContext->TextureProgram);
        RenderQuadTexture(RenderContext, X, Y, Size, Size, Uvs, RGBA8_WHITE, 0.f);
        EndBatch(RenderContext);
    }
}

// NOTE(zoubir): the picker at Y, centred on CenterX
internal void
DoSkinPicker(render_context *RenderContext, app_state *AppState, app_input *Input,
             player_slot *Slot, float CenterX, float Y)
{
    u32 Role = Slot->Role;
    if (!SkinPickerHeight(AppState, Role))
    {
        return;
    }
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    u32 Color = RoleUIColor(Role);
    UIText(RenderContext, Body, CenterX, Y, "Pick your look", UI_COLOR_TEXT, UIAlign_Center);
    float CardY = Y + UILineHeight(Body) + UI_GAP_SMALL;
    float Width = HeroSkin_Count * SKIN_CARD_WIDTH + (HeroSkin_Count - 1) * UI_GAP;
    float X = CenterX - 0.5f * Width;
    for(u32 Skin = 0; Skin < HeroSkin_Count; Skin++)
    {
        bool32 Worn = Slot->Skin == Skin;
        bool32 Ready = HeroSkinSheets[Role][Skin].Loaded;
        if (OptionsButton(RenderContext, Input, X, CardY, SKIN_CARD_WIDTH, SKIN_CARD_HEIGHT, Worn) &&
            !Worn && Ready)
        {
            if (IsOnline(AppState->Online))
            {
                RequestDungeonRole(AppState, Role, Skin);
            }
            else
            {
                SetPlayerSkin(Slot, Skin);
            }
        }
        if (Worn)
        {
            DrawRoundOutline(RenderContext, X, CardY, SKIN_CARD_WIDTH, SKIN_CARD_HEIGHT, Color);
        }
        if (Ready)
        {
            DrawSkinPreview(RenderContext, AppState, Role, Skin,
                            X + 0.5f * (SKIN_CARD_WIDTH - SKIN_CARD_SPRITE), CardY + 4.f,
                            SKIN_CARD_SPRITE);
        }
        UIText(RenderContext, Small, X + 0.5f * SKIN_CARD_WIDTH,
               CardY + SKIN_CARD_HEIGHT - UILineHeight(Small) - 6.f,
               Ready ? HeroSkinNames[Skin] : (char *)"Missing files",
               Worn ? Color : (Ready ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED), UIAlign_Center);
        X += SKIN_CARD_WIDTH + UI_GAP;
    }
}
