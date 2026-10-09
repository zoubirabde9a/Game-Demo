/* Wild roll card (sim/dungeon/run_tree/): when a run starts, or the
   player picks another class, a card on the right, under the minimap,
   lists what the second tree's wild slots rolled this time, each with
   what a rank gives, the keystone in gold. The roll changes the tree under the player's points,
   so it says so instead of leaving it for the talent panel.

   It watches the local slot's TreeSeed and class, which a snapshot
   carries, so it shows online too. Entry point: DrawWildRollCard, once a
   frame from screen_pass.inc. */

// NOTE(zoubir): it waits out the level's title card and the controls
#define WILD_CARD_DELAY 3.f
#define WILD_CARD_SECONDS 9.f
#define WILD_CARD_FADE 0.6f
#define WILD_CARD_WIDTH 380.f

internal void
DrawWildRollCard(render_context *RenderContext, app_state *AppState, u32 WindowWidth,
                 u32 WindowHeight)
{
    local_persist u32 SeenSeed;
    local_persist u32 SeenRole;
    local_persist bool32 SeenAny;
    local_persist float Born = -100.f;
    float Now = GetFxClock(AppState);
    if (!IsDungeon(AppState) || AppState->LocalPlayerIndex >= MAX_PLAYERS)
    {
        SeenAny = false;
        return;
    }
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    if (!Slot->Active)
    {
        return;
    }
    if (!SeenAny || SeenSeed != Slot->TreeSeed || SeenRole != Slot->Role)
    {
        SeenAny = true;
        SeenSeed = Slot->TreeSeed;
        SeenRole = Slot->Role;
        Born = Now;
    }
    float Age = Now - Born - WILD_CARD_DELAY;
    if (Age < 0.f || Age >= WILD_CARD_SECONDS)
    {
        return;
    }
    float Alpha = Minimum(Clamp01(Age / WILD_CARD_FADE),
                          Clamp01((WILD_CARD_SECONDS - Age) / WILD_CARD_FADE));
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    u32 Role = Slot->Role < PlayerRole_Count ? Slot->Role : PlayerRole_Damage;
    u32 Accent = RunBranchAccent(Role);
    float Row = UILineHeight(Body) + UILineHeight(Small) + 4.f;
    float Height = UILineHeight(Strong) + UILineHeight(Small) + 6.f * Row + 3.f * UI_GAP_SMALL + 4.f;
    float X = (float)WindowWidth - WILD_CARD_WIDTH - 16.f + 30.f * (1.f - Clamp01(Age / WILD_CARD_FADE));
    float Y = Maximum(240.f, 0.3f * (float)WindowHeight);
    DrawUIPanel(RenderContext, X, Y, WILD_CARD_WIDTH, Height, Accent, Alpha);
    float LineY = Y + UI_GAP_SMALL + 4.f;
    UIText(RenderContext, Strong, X + 16.f, LineY, "Wild talents this run", WithAlpha(Accent, Alpha));
    LineY += UILineHeight(Strong);
    char Line[112];
    snprintf(Line, sizeof(Line), "%s, rolled again each run", RunTrees[Role].Name);
    UIText(RenderContext, Small, X + 16.f, LineY, Line, WithAlpha(UI_COLOR_TEXT_MUTED, Alpha));
    LineY += UILineHeight(Small) + UI_GAP_SMALL;
    for(u32 Index = 0; Index < RUN_TALENTS; Index++)
    {
        if (!RunSlotWild[Index])
        {
            continue;
        }
        bool32 Keystone = Index == RUN_KEYSTONE_SLOT;
        run_mod_def *Mod = &RunModDefs[RunModAtSlot(Slot, Index)];
        u32 NameColor = Keystone ? UI_RGBA(255, 205, 80, 255) : UI_COLOR_TEXT;
        // NOTE(zoubir): a small die before each, as the panel marks them
        DrawFilledRectangle(RenderContext, X + 18.f, LineY + 5.f, 8.f, 8.f,
                            WithAlpha(NameColor, 0.9f * Alpha), 0.f);
        UIText(RenderContext, Body, X + 34.f, LineY, Mod->Name, WithAlpha(NameColor, Alpha));
        LineY += UILineHeight(Body);
        RunTalentEffectText(Slot, Talent_RunFirst + Index, 1, Line, sizeof(Line));
        UIText(RenderContext, Small, X + 34.f, LineY, Line, WithAlpha(UI_COLOR_TEXT_MUTED, Alpha));
        LineY += UILineHeight(Small) + 4.f;
    }
}
