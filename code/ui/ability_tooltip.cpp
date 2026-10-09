/* Ability tooltip (ability_bar.cpp): the card over a hovered slot. The
   name in the slot's colour with its key on the right, a line saying
   what kind of ability it is, what it does wrapped to the card, its
   numbers, then under a rule its cooldown on the left and what a cast
   costs on the right. A dungeon class's spell reads its row in the class
   table (sim/dungeon/roles.cpp, role_kits/<class>_defs.cpp); the game's
   own abilities read their talent (sim/progression/talents.cpp) and its
   numbers at the level held (talent_panel/talent_effects.cpp). */

#define ABILITY_TIP_WIDTH 300.f
#define ABILITY_TIP_MOST_WIDTH 380.f
#define ABILITY_TIP_PAD 14.f
#define ABILITY_TIP_ROWS 4 // the description wraps to at most this many rows
// NOTE(zoubir): in a dungeon the class meters (ui/dungeon/classes/) sit
// just over the bar; the card stays above them
#define ABILITY_TIP_LIFT 10.f
#define ABILITY_TIP_DUNGEON_LIFT 64.f

// NOTE(zoubir): what the ability bar saw hovered this frame, drawn late
// (DrawAbilityTip in ability_bar.cpp)
struct ability_tip
{
    i32 Slot;     // index into AbilitySlotDefs, -1 for none
    float X;      // the slot's centre
    float Bottom; // the bar's plate top
    float Full;   // the whole cooldown as it stands, 0 for none
    float Left;   // seconds still to run
};

// NOTE(zoubir): where each row of Text starts and ends when it is wrapped
// at spaces to Width; returns the number of rows, at most
// ABILITY_TIP_ROWS (the last one may run long)
internal u32
WrapAbilityTipText(font *Font, char *Text, float Width, u32 *Starts, u32 *Ends)
{
    u32 Count = 0;
    u32 At = 0;
    while (Text[At] && Count < ABILITY_TIP_ROWS)
    {
        float Used = 0.f;
        u32 LastSpace = 0;
        u32 End = At;
        bool32 Last = Count + 1 == ABILITY_TIP_ROWS;
        while (Text[End])
        {
            float Next = Used + GetCharacterWidth(Font, Text[End]);
            if (Next > Width && End > At && !Last) break;
            if (Text[End] == ' ') LastSpace = End;
            Used = Next;
            ++End;
        }
        Starts[Count] = At;
        if (Text[End] && LastSpace > At)
        {
            End = LastSpace;
        }
        Ends[Count++] = End;
        At = Text[End] == ' ' ? End + 1 : End;
    }
    return Count;
}

// NOTE(zoubir): a cooldown as people say it: 0.6 s, 9 s, 12.5 s
inline void
FormatAbilityTipSeconds(char *Out, u32 OutSize, float Seconds, char *After)
{
    snprintf(Out, OutSize, "%.3g s%s", Seconds, After);
}

// NOTE(zoubir): Full is the button's whole cooldown as it stands (after
// talents), 0 for none; Left is what is still to run
internal void
DrawAbilityTooltip(render_context *RenderContext, app_state *AppState, world_entity *Player,
                   ability_slot_def *Def, float SlotCentreX, float PlateTop,
                   u32 WindowWidth, float Full, float Left)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : Body;
    if (!Body)
    {
        return;
    }
    char *Key = ActionKeyLabel(Def->Button);
    char *Name = Def->Name;
    char Kind[64] = "";
    char *Description = 0;
    char Numbers[112] = "";
    char *Cost = 0;

    // NOTE(zoubir): a dungeon role's spell on the key (sim/dungeon/
    // role_abilities.cpp) goes by its own name and its class's table
    role_spell *RoleSpell = RoleSpellOnButton(AppState, Player, Def->Button);
    u32 Talent = TalentForButton(Def->Button);
    if (RoleSpell)
    {
        Name = RoleSpell->Name;
        Cost = RoleSpell->Cost;
        snprintf(Kind, sizeof(Kind), "%s spell", RoleTable[Slot->Role].Name);
        // NOTE(zoubir): Help reads "Name: what it does"; the card has the
        // name already
        Description = RoleSpell->Help;
        size_t NameLength = strlen(Name);
        if (Description && strncmp(Description, Name, NameLength) == 0 &&
            Description[NameLength] == ':')
        {
            Description += NameLength + 1;
            while (*Description == ' ') ++Description;
        }
        // NOTE(zoubir): a Help that opens on the cost ("30 Rage, spin...")
        // leaves it to the card's foot
        size_t CostLength = Cost ? strlen(Cost) : 0;
        if (Description && CostLength && strncmp(Description, Cost, CostLength) == 0 &&
            Description[CostLength] == ',')
        {
            Description += CostLength + 1;
            while (*Description == ' ') ++Description;
        }
        float Reach = RoleSpell->Reach;
        switch (RoleSpell->Aim)
        {
            case RoleAim_Ground: snprintf(Numbers, sizeof(Numbers), "Ground target, %.0f radius", Reach); break;
            case RoleAim_Foe:    snprintf(Numbers, sizeof(Numbers), "Foe target, %.0f range", Reach); break;
            case RoleAim_Ally:   snprintf(Numbers, sizeof(Numbers), "Ally target, %.0f range", Reach); break;
            case RoleAim_Line:   snprintf(Numbers, sizeof(Numbers), "Aimed in a line, %.0f range", Reach); break;
            default:
            {
                if (Reach > 0.f)
                {
                    snprintf(Numbers, sizeof(Numbers), "%.0f reach", Reach);
                }
            } break;
        }
        if (!Full)
        {
            Full = RoleSpell->Cooldown;
        }
    }
    else if (Talent < Talent_Count)
    {
        talent_def *TalentDef = &TalentDefs[Talent];
        u32 Level = TalentLevel(Slot, Talent);
        snprintf(Kind, sizeof(Kind), "Level %u of %u", Level, TalentDef->MaxLevel);
        Description = TalentDef->Summary;
        if (Level > 0)
        {
            TalentEffectText(Slot, Talent, Level, Numbers, sizeof(Numbers));
        }
    }

    char CooldownText[48];
    if (Full > 0.f)
    {
        FormatAbilityTipSeconds(CooldownText, sizeof(CooldownText), Full, " cooldown");
    }
    else
    {
        snprintf(CooldownText, sizeof(CooldownText), "No cooldown");
    }
    char LeftText[32] = "";
    if (Left > 0.f)
    {
        FormatAbilityTipSeconds(LeftText, sizeof(LeftText), Left, " left");
    }
    char *CostText = Cost ? Cost : (char *)"No cost";

    // NOTE(zoubir): wide enough for the title and the footer, the
    // description wraps to what is left
    float KeyWidth = UITextWidth(Small, Key) + 10.f;
    float FooterWidth = UITextWidth(Small, CooldownText) + UITextWidth(Small, LeftText) +
        UITextWidth(Small, CostText) + 40.f;
    float Width = Maximum(ABILITY_TIP_WIDTH, UITextWidth(Strong, Name) + KeyWidth + 20.f +
                          2.f * ABILITY_TIP_PAD);
    Width = Maximum(Width, FooterWidth + 2.f * ABILITY_TIP_PAD);
    Width = Maximum(Width, UITextWidth(Body, Numbers) + 2.f * ABILITY_TIP_PAD);
    Width = Minimum(Width, ABILITY_TIP_MOST_WIDTH);
    float Inner = Width - 2.f * ABILITY_TIP_PAD;

    u32 Starts[ABILITY_TIP_ROWS];
    u32 Ends[ABILITY_TIP_ROWS];
    u32 Rows = (Description && Description[0]) ?
        WrapAbilityTipText(Body, Description, Inner, Starts, Ends) : 0;

    float Gap = 4.f;
    float Height = 2.f * ABILITY_TIP_PAD - 4.f + UILineHeight(Strong);
    if (Kind[0]) Height += UILineHeight(Small);
    if (Rows) Height += 2.f * Gap + (float)Rows * UILineHeight(Body);
    if (Numbers[0]) Height += Gap + UILineHeight(Body);
    Height += 2.f * Gap + 5.f + UILineHeight(Small);

    float X = Maximum(8.f, Minimum(SlotCentreX - 0.5f * Width, (float)WindowWidth - Width - 8.f));
    float Y = PlateTop - Height -
        (IsDungeon(AppState) ? ABILITY_TIP_DUNGEON_LIFT : ABILITY_TIP_LIFT);
    u32 Accent = Def->Accent;
    // NOTE(zoubir): a dark sheet under the glass, the slot's colour down
    // the left edge, as the talent panel's cards
    DrawFilledRectangle(RenderContext, X + 4.f, Y + 4.f, Width - 8.f, Height - 8.f,
                        UI_RGBA(6, 7, 11, 240), 0.f);
    DrawUIPanel(RenderContext, X, Y, Width, Height, Accent);
    DrawFilledRectangle(RenderContext, X + 6.f, Y + 10.f, 3.f, Height - 20.f,
                        WithAlpha(Accent, 0.9f), 0.f);

    float TextX = X + ABILITY_TIP_PAD;
    float Right = X + Width - ABILITY_TIP_PAD;
    float LineY = Y + ABILITY_TIP_PAD - 4.f;
    UIText(RenderContext, Strong, TextX, LineY, Name, Accent);
    // NOTE(zoubir): the key on a dark tab, as on the slot
    float KeyHeight = UILineHeight(Small);
    float KeyX = Right - KeyWidth;
    float KeyY = LineY + 0.5f * (UILineHeight(Strong) - KeyHeight);
    DrawRoundRect(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight, UI_RGBA(12, 13, 20, 235));
    DrawRoundOutline(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight, WithAlpha(Accent, 0.7f));
    UIText(RenderContext, Small, KeyX + 5.f, KeyY, Key, UI_COLOR_TEXT);
    LineY += UILineHeight(Strong);
    if (Kind[0])
    {
        UIText(RenderContext, Small, TextX, LineY, Kind, UI_COLOR_TEXT_MUTED);
        LineY += UILineHeight(Small);
    }
    if (Rows)
    {
        LineY += 2.f * Gap;
        for(u32 Row = 0; Row < Rows; Row++)
        {
            char Text[160];
            u32 Length = Minimum(Ends[Row] - Starts[Row], (u32)sizeof(Text) - 1);
            memcpy(Text, Description + Starts[Row], Length);
            Text[Length] = 0;
            UIText(RenderContext, Body, TextX, LineY, Text, UI_COLOR_TEXT);
            LineY += UILineHeight(Body);
        }
    }
    if (Numbers[0])
    {
        LineY += Gap;
        UIText(RenderContext, Body, TextX, LineY, Numbers, UI_RGBA(150, 205, 255, 255));
        LineY += UILineHeight(Body);
    }

    // NOTE(zoubir): a thin rule, then cooldown and cost along the foot
    LineY += 2.f * Gap;
    DrawFilledRectangle(RenderContext, TextX, LineY, Inner, 1.f, UI_RGBA(255, 255, 255, 30), 0.f);
    LineY += 5.f;
    UIText(RenderContext, Small, TextX, LineY, CooldownText, UI_COLOR_TEXT);
    if (LeftText[0])
    {
        UIText(RenderContext, Small, TextX + UITextWidth(Small, CooldownText) + 10.f, LineY,
               LeftText, UI_RGBA(255, 150, 120, 255));
    }
    UIText(RenderContext, Small, Right, LineY, CostText,
           Cost ? UI_COLOR_ACCENT : UI_COLOR_TEXT_MUTED, UIAlign_Right);
}
