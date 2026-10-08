/* Cast mode toggle: a checkbox at the top middle of the screen that
   switches between quick cast (abilities cast on the key) and standard
   cast (an area ability's key shows its range first, a left click casts
   it), with a short line saying what the current mode does. While an
   ability is aimed, that line names it instead and says how to cast or
   cancel it, in the ability's colour. It stays inside the plate, as the
   dungeon HUD's objective line sits just under it.
   The modes themselves are client/cast_targeting.cpp. */

#define CAST_TOGGLE_TOP 12.f
#define CAST_TOGGLE_BOX 16.f
#define CAST_TOGGLE_PAD 10.f

internal void
DoCastModeToggle(render_context *RenderContext, app_state *AppState,
                 app_input *Input, u32 WindowWidth)
{
    cast_targeting *Targeting = GetCastTargeting(AppState);
    world_entity *Player = GetLocalPlayer(AppState);
    if (!Player || ConnectScreenTakesInput(AppState))
    {
        Targeting->ToggleWidth = 0.f;
        return;
    }
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    bool32 Quick = Targeting->Mode == CastMode_Quick;
    char *Label = (char *)"Quick cast";
    char *Hint = Quick ? (char *)"keys cast at once" :
        (char *)"keys aim first, click to cast";
    u32 HintColor = UI_COLOR_TEXT_MUTED;
    // NOTE(zoubir): what is aimed takes the hint's place, in its slot's
    // colour
    char AimLine[96];
    if (Targeting->Aiming)
    {
        char *Name = (char *)"";
        HintColor = UI_COLOR_ACCENT;
        for(u32 Index = 0; Index < ABILITY_SLOT_DEF_COUNT; Index++)
        {
            if (AbilitySlotDefs[Index].Button == Targeting->Aiming)
            {
                Name = AbilitySlotDefs[Index].Name;
                HintColor = AbilitySlotDefs[Index].Accent;
            }
        }
        // NOTE(zoubir): a dungeon class's spell on the key goes by its own
        // name (sim/dungeon/role_abilities.cpp)
        role_spell *Spell = LocalRoleSpell(AppState, Targeting->Aiming);
        if (Spell)
        {
            Name = Spell->Name;
        }
        snprintf(AimLine, sizeof(AimLine), "%s: left click or %s to cast, right click to cancel",
                 Name, ActionKeyLabel(Targeting->Aiming));
        Hint = AimLine;
    }

    float LineHeight = UILineHeight(Body);
    float LabelWidth = UITextWidth(Body, Label);
    float HintWidth = UITextWidth(Small, Hint);
    float Width = 2.f * CAST_TOGGLE_PAD + CAST_TOGGLE_BOX + UI_GAP_SMALL +
        LabelWidth + UI_GAP + HintWidth;
    float Height = LineHeight + 2.f * UI_PADDING;
    float X = 0.5f * ((float)WindowWidth - Width);
    float Y = CAST_TOGGLE_TOP;
    Targeting->ToggleX = X;
    Targeting->ToggleY = Y;
    Targeting->ToggleWidth = Width;
    Targeting->ToggleHeight = Height;

    bool32 Hot = IsMouseOnCastModeToggle(Targeting, Input);
    if (Hot && Input->LeftButton.Pressed)
    {
        Targeting->Mode = Quick ? CastMode_Standard : CastMode_Quick;
        Targeting->Aiming = 0;
        Quick = !Quick;
    }

    DrawUIPanel(RenderContext, X, Y, Width, Height,
                Hot ? UI_COLOR_ACCENT : UI_RGBA(150, 160, 190, 255));
    float BoxX = X + CAST_TOGGLE_PAD;
    float BoxY = Y + 0.5f * (Height - CAST_TOGGLE_BOX);
    DrawRoundRect(RenderContext, BoxX, BoxY, CAST_TOGGLE_BOX, CAST_TOGGLE_BOX,
                  UI_COLOR_FIELD);
    DrawRoundOutline(RenderContext, BoxX, BoxY, CAST_TOGGLE_BOX, CAST_TOGGLE_BOX,
                     Hot ? UI_COLOR_ACCENT : UI_COLOR_BORDER);
    if (Quick)
    {
        DrawRoundRect(RenderContext, BoxX + 4.f, BoxY + 4.f,
                      CAST_TOGGLE_BOX - 8.f, CAST_TOGGLE_BOX - 8.f, UI_COLOR_ACCENT);
    }
    float TextX = BoxX + CAST_TOGGLE_BOX + UI_GAP_SMALL;
    UIText(RenderContext, Body, TextX, Y + 0.5f * (Height - LineHeight) + 1.f, Label,
           Quick ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);
    UIText(RenderContext, Small, TextX + LabelWidth + UI_GAP,
           Y + 0.5f * (Height - UILineHeight(Small)), Hint, HintColor);
}
