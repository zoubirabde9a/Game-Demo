/* Wild roll card (sim/dungeon/class_tree.cpp): when a run starts, or
   the player picks another class, a card on the right, under the minimap,
   lists what the class tree's wild slots rolled this time, a column per
   branch, the keystones in gold. The roll changes the tree under the
   player's points, so it says so instead of leaving it for the talent
   panel.

   It watches the local slot's TreeSeed and class, which a snapshot
   carries, so it shows online too. Entry point: DrawWildRollCard, once a
   frame from screen_pass.inc. */

// NOTE(zoubir): it waits out the level's title card and the controls
#define WILD_CARD_DELAY 3.f
#define WILD_CARD_SECONDS 9.f
#define WILD_CARD_FADE 0.6f
#define WILD_CARD_WIDTH 420.f

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
    float Rows = 5.f;
    float Height = UILineHeight(Strong) + 2.f * UILineHeight(Small) + Rows * UILineHeight(Body) +
        3.f * UI_GAP_SMALL + 4.f;
    float X = (float)WindowWidth - WILD_CARD_WIDTH - 16.f + 30.f * (1.f - Clamp01(Age / WILD_CARD_FADE));
    float Y = Maximum(240.f, 0.3f * (float)WindowHeight);
    DrawUIPanel(RenderContext, X, Y, WILD_CARD_WIDTH, Height, Accent, Alpha);
    float TopY = Y + UI_GAP_SMALL + 4.f;
    UIText(RenderContext, Strong, X + 16.f, TopY, "Wild talents this run", WithAlpha(Accent, Alpha));
    TopY += UILineHeight(Strong);
    UIText(RenderContext, Small, X + 16.f, TopY, "rolled again each run",
           WithAlpha(UI_COLOR_TEXT_MUTED, Alpha));
    TopY += UILineHeight(Small) + UI_GAP_SMALL;
    float ColumnWidth = 0.5f * (WILD_CARD_WIDTH - 32.f);
    for(u32 Branch = 0; Branch < 2; Branch++)
    {
        float ColumnX = X + 16.f + (float)Branch * ColumnWidth;
        float LineY = TopY;
        UIText(RenderContext, Small, ColumnX, LineY, ClassBranchName(Role, Branch),
               WithAlpha(Accent, Alpha));
        LineY += UILineHeight(Small);
        for(u32 Index = 0; Index < ROLE_TALENTS; Index++)
        {
            u32 Talent = ClassTreeTalent(Branch * ROLE_TALENTS + Index);
            if (!ClassSlotIsWild(Talent))
            {
                continue;
            }
            u32 NameColor = ClassSlotIsKeystone(Talent) ? UI_RGBA(255, 205, 80, 255) : UI_COLOR_TEXT;
            // NOTE(zoubir): a small die before each, as the panel marks them
            DrawFilledRectangle(RenderContext, ColumnX + 2.f, LineY + 5.f, 7.f, 7.f,
                                WithAlpha(NameColor, 0.9f * Alpha), 0.f);
            UIText(RenderContext, Body, ColumnX + 16.f, LineY, ShownTalentDef(Slot, Talent)->Name,
                   WithAlpha(NameColor, Alpha));
            LineY += UILineHeight(Body);
        }
    }
}
