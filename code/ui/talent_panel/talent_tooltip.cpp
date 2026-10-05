/* The talent panel's tooltip: a card beside the hovered medallion with
   the talent's name in its branch's colour, what kind it is, what it
   does, its numbers now and at its next rank (talent_effects.cpp), its
   cooldown, and whether a point can go in. */

#define TOOLTIP_LINES 8

struct tooltip_lines
{
    char Text[TOOLTIP_LINES][112];
    u32 Color[TOOLTIP_LINES];
    font *Font[TOOLTIP_LINES];
    u32 Count;
};

inline char *
AddTooltipLine(tooltip_lines *Lines, font *Font, u32 Color)
{
    char *Result = 0;
    if (Lines->Count < TOOLTIP_LINES)
    {
        Lines->Font[Lines->Count] = Font;
        Lines->Color[Lines->Count] = Color;
        Result = Lines->Text[Lines->Count++];
        Result[0] = 0;
    }
    return Result;
}

internal void
DrawTalentTooltip(render_context *RenderContext, app_state *AppState,
                  talent_panel_layout *L, u32 Talent, v2 Node)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    talent_def *Def = &TalentDefs[Talent];
    u32 Accent = TalentBranchAccents[Def->Branch];
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : Body;
    u32 Level = TalentLevel(Slot, Talent);
    talent_refusal Refusal = CanLearnTalent(Slot, Talent);
    tooltip_lines Lines = {};
    char *Line;

    if ((Line = AddTooltipLine(&Lines, Strong, Accent)) != 0)
    {
        snprintf(Line, 112, "%s", Def->Name);
    }
    if ((Line = AddTooltipLine(&Lines, Small, UI_COLOR_TEXT_MUTED)) != 0)
    {
        if (Def->Button)
        {
            snprintf(Line, 112, "%s ability  key %s   level %u of %u",
                     TalentBranchNames[Def->Branch], ActionKeyLabel(Def->Button), Level,
                     Def->MaxLevel);
        }
        else
        {
            snprintf(Line, 112, "%s passive   rank %u of %u", TalentBranchNames[Def->Branch],
                     Level, Def->MaxLevel);
        }
    }
    if ((Line = AddTooltipLine(&Lines, Body, UI_COLOR_TEXT)) != 0)
    {
        snprintf(Line, 112, "%s", Def->Summary);
    }

    // NOTE(zoubir): the numbers now, and what the next rank makes them
    char Now[96];
    char Next[96];
    bool32 HasNow = Level > 0 && TalentEffectText(Slot, Talent, Level, Now, sizeof(Now));
    bool32 HasNext = Level < Def->MaxLevel &&
        TalentEffectText(Slot, Talent, Level + 1, Next, sizeof(Next));
    float Cooldown = Def->Button ? TalentBaseCooldown(Slot->Entity, Def->Button) : 0.f;
    if (Level > 0 && (HasNow || Cooldown > 0.f) &&
        (Line = AddTooltipLine(&Lines, Body, UI_RGBA(230, 236, 245, 255))) != 0)
    {
        if (Cooldown > 0.f)
        {
            snprintf(Line, 112, "Now: %s%s%.1f s cooldown", HasNow ? Now : "",
                     HasNow ? ", " : "", Cooldown * CooldownScaleForLevel(Level));
        }
        else
        {
            snprintf(Line, 112, "Now: %s", Now);
        }
    }
    if (Level < Def->MaxLevel &&
        (Line = AddTooltipLine(&Lines, Body, UI_RGBA(140, 230, 160, 255))) != 0)
    {
        char *Label = (char *)((Def->Button && Level == 0) ? "Unlock" : "Next");
        if (Cooldown > 0.f)
        {
            snprintf(Line, 112, "%s: %s%s%.1f s cooldown", Label, HasNext ? Next : "",
                     HasNext ? ", " : "", Cooldown * CooldownScaleForLevel(Level + 1));
        }
        else if (HasNext)
        {
            snprintf(Line, 112, "%s: %s", Label, Next);
        }
        else
        {
            snprintf(Line, 112, "%s: %s", Label, Def->PerRank);
        }
    }
    u32 StatusColor = Refusal == TalentRefusal_None ? UI_COLOR_GOOD :
        (Refusal == TalentRefusal_MaxRank ? UI_COLOR_ACCENT : UI_RGBA(255, 120, 100, 255));
    if ((Line = AddTooltipLine(&Lines, Body, StatusColor)) != 0)
    {
        TalentRefusalText(Slot, Talent, Refusal, Line, 112);
    }

    float Width = TALENT_TOOLTIP_WIDTH;
    float Height = Lines.Count > 3 ? 24.f : 20.f;
    for(u32 Index = 0; Index < Lines.Count; Index++)
    {
        Width = Maximum(Width, UITextWidth(Lines.Font[Index], Lines.Text[Index]) + 30.f);
        Height += UILineHeight(Lines.Font[Index]) + 5.f;
    }
    float X = Node.X + 0.5f * L->NodeSize + 22.f;
    if (X + Width > L->X + L->Width - 8.f)
    {
        X = Node.X - 0.5f * L->NodeSize - 22.f - Width;
    }
    float Y = Minimum(Node.Y - 0.5f * L->NodeSize - 6.f, L->Y + L->Height - Height - 8.f);
    // NOTE(zoubir): a dark sheet under the glass, and the branch's colour
    // as a bar down the left edge
    DrawFilledRectangle(RenderContext, X + 4.f, Y + 4.f, Width - 8.f, Height - 8.f,
                        UI_RGBA(6, 7, 11, 240), 0.f);
    DrawUIPanel(RenderContext, X, Y, Width, Height, Accent);
    DrawFilledRectangle(RenderContext, X + 6.f, Y + 10.f, 3.f, Height - 20.f,
                        WithAlpha(Accent, 0.9f), 0.f);
    float LineY = Y + 11.f;
    for(u32 Index = 0; Index < Lines.Count; Index++)
    {
        // NOTE(zoubir): a thin rule before the numbers
        if (Index == 3)
        {
            DrawFilledRectangle(RenderContext, X + 16.f, LineY - 1.f, Width - 32.f, 1.f,
                                UI_RGBA(255, 255, 255, 30), 0.f);
            LineY += 3.f;
        }
        UIText(RenderContext, Lines.Font[Index], X + 16.f, LineY, Lines.Text[Index],
               Lines.Color[Index]);
        LineY += UILineHeight(Lines.Font[Index]) + 5.f;
    }
}
