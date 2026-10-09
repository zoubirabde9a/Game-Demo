/* The talent panel's branches and medallions: each column's glowing
   backdrop, its name and points, the tier thresholds, the lines between
   tiers with light running along the open ones, and each talent's
   medallion, rank ring, pips, key and name. */

// NOTE(zoubir): whether a line runs from Talent to Next: the tier below,
// the same column, or a tier with one talent that every column feeds
inline bool32
TalentsJoined(u32 Talent, u32 Next)
{
    talent_def *Def = &TalentDefs[Talent];
    talent_def *NextDef = &TalentDefs[Next];
    bool32 Result = NextDef->Branch == Def->Branch && NextDef->Tier == Def->Tier + 1 &&
        (NextDef->Column == Def->Column || TalentsInTier(Def->Branch, NextDef->Tier) == 1);
    return Result;
}

internal void
DrawTalentColumn(render_context *RenderContext, app_state *AppState, talent_panel *Panel,
                 talent_panel_layout *L, u32 Branch, i32 Hovered)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    u32 Accent = TalentBranchAccent(Slot, Branch);
    float X = L->ColumnX[Branch];
    u32 Spent = TalentPointsSpent(Slot, Branch);
    DrawShaderQuad(RenderContext, Shader_TalentBackdrop, X, L->ColumnTop, L->ColumnWidth,
                   L->ColumnHeight, WithAlpha(Accent, 0.55f + 0.05f * (float)Minimum(Spent, 8u)));

    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Strong, X + 16.f, L->ColumnTop + 12.f, TalentBranchName(Slot, Branch),
           Accent);
    char Text[32];
    snprintf(Text, sizeof(Text), "%u spent", Spent);
    UIText(RenderContext, Small, X + L->ColumnWidth - 16.f, L->ColumnTop + 15.f, Text,
           UI_COLOR_TEXT_MUTED, UIAlign_Right);
    // NOTE(zoubir): how far toward the last tier, as a thin bar under the name
    float Toward = Minimum(1.f, (float)Spent / (float)TalentTierCost(TalentBranchTiers(Branch) - 1, Branch));
    float BarX = X + 16.f;
    float BarWidth = L->ColumnWidth - 32.f;
    DrawFilledRectangle(RenderContext, BarX, L->ColumnTop + 38.f, BarWidth, 2.f,
                        UI_RGBA(255, 255, 255, 20), 0.f);
    DrawFilledRectangle(RenderContext, BarX, L->ColumnTop + 38.f, Toward * BarWidth, 2.f,
                        WithAlpha(Accent, 0.9f), 0.f);

    // NOTE(zoubir): each tier's threshold on the left edge: a faint line
    // once open, a lock and the points it needs while shut
    for(u32 Tier = 1; Tier < TalentBranchTiers(Branch); Tier++)
    {
        float Y = L->TierTop + (float)Tier * L->TierHeight - 8.f;
        bool32 Open = IsTalentTierOpen(Slot, Branch, Tier);
        u32 Line = Open ? WithAlpha(Accent, 0.25f) : UI_RGBA(255, 255, 255, 14);
        DrawFilledRectangle(RenderContext, X + 12.f, Y, L->ColumnWidth - 24.f, 1.f, Line, 0.f);
        if (!Open)
        {
            snprintf(Text, sizeof(Text), "%u", TalentTierCost(Tier, Branch));
            float ChipX = X + 12.f;
            float ChipY = Y - 0.5f * UILineHeight(Small) - 2.f;
            float ChipWidth = UITextWidth(Small, Text) + 26.f;
            DrawFilledRectangle(RenderContext, ChipX, ChipY, ChipWidth,
                                UILineHeight(Small) + 4.f, UI_RGBA(10, 11, 16, 230), 0.f);
            DrawRectangle(RenderContext, ChipX, ChipY, ChipWidth, UILineHeight(Small) + 4.f,
                          UI_RGBA(110, 110, 125, 200), 0.f);
            // NOTE(zoubir): a padlock: a body and a shackle
            DrawFilledRectangle(RenderContext, ChipX + 5.f, ChipY + 8.f, 9.f, 7.f,
                                UI_COLOR_TEXT_MUTED, 0.f);
            DrawRectangle(RenderContext, ChipX + 6.5f, ChipY + 3.f, 6.f, 6.f,
                          UI_COLOR_TEXT_MUTED, 0.f);
            UIText(RenderContext, Small, ChipX + 18.f, ChipY + 2.f, Text, UI_COLOR_TEXT_MUTED);
        }
    }

    // NOTE(zoubir): lines from each tier to the next. Open ones carry
    // motes of light downward; the ones into the hovered talent are bright
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        if (TalentDefs[Talent].Branch != Branch)
        {
            continue;
        }
        for(u32 Next = 0; Next < Talent_Count; Next++)
        {
            if (!TalentsJoined(Talent, Next))
            {
                continue;
            }
            // NOTE(zoubir): from under the name to the next ring's top
            float Ring = 0.58f * L->NodeSize;
            v2 From = TalentNodeCentre(L, Talent) + V2(0.f, Ring + 34.f);
            v2 To = TalentNodeCentre(L, Next) - V2(0.f, Ring + 2.f);
            bool32 Lit = IsTalentTierOpen(Slot, Branch, TalentDefs[Next].Tier);
            bool32 Path = Hovered == (i32)Next || Hovered == (i32)Talent;
            u32 Color = Lit ? WithAlpha(Accent, Path ? 0.95f : 0.55f) :
                (Path ? UI_RGBA(255, 255, 255, 70) : UI_RGBA(255, 255, 255, 22));
            DrawUIBand(RenderContext, From, To, Path ? 4.f : (Lit ? 3.f : 2.f), Color);
            if (Lit)
            {
                for(u32 Mote = 0; Mote < 3; Mote++)
                {
                    float At = fmodf(0.45f * Panel->Clock + (float)Mote / 3.f +
                                     0.13f * (float)Talent, 1.f);
                    v2 P = From + At * (To - From);
                    float Glow = Sin(Pi32 * At);
                    float Size = 10.f + 6.f * Glow;
                    DrawShaderQuad(RenderContext, Shader_Glow, P.X - 0.5f * Size,
                                   P.Y - 0.5f * Size, Size, Size,
                                   WithAlpha(Accent, 0.9f * Glow), RenderBlend_Additive);
                }
            }
        }
    }
}

// NOTE(zoubir): one talent's medallion, ring, icon, key, pips and name; a
// click on it asks for a point, or shakes it and says why not
internal void
DrawTalentNode(render_context *RenderContext, app_state *AppState, app_input *Input,
               talent_panel *Panel, talent_panel_layout *L, u32 Talent, bool32 Hot)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    talent_def *Def = ShownTalentDef(Slot, Talent);
    u32 Accent = TalentBranchAccent(Slot, Def->Branch);
    v2 Centre = TalentNodeCentre(L, Talent);
    float Denied = Panel->Denied[Talent];
    Centre.X += 5.f * Denied * Sin(Panel->Clock * 70.f);
    float Size = L->NodeSize;
    float State = TalentNodeState(AppState, Slot, Talent);
    u32 Level = TalentLevel(Slot, Talent);
    float Flash = Panel->Flash[Talent];
    float Lift = Hot ? 1.06f : 1.f;
    float Quad = Lift * Size / 0.74f;

    // NOTE(zoubir): sent here from the ability bar: a slow ring pulses out
    if (Panel->Focus == (i32)Talent)
    {
        float Beat = fmodf(Panel->FocusAge * 1.6f, 1.f);
        float Ring = Size * (1.1f + 0.9f * Beat);
        DrawShaderQuad(RenderContext, Shader_Ring, Centre.X - 0.5f * Ring,
                       Centre.Y - 0.5f * Ring, Ring, Ring,
                       WithAlpha(UI_RGBA(255, 230, 150, 255), 1.f - Beat), RenderBlend_Additive);
    }

    DrawShaderQuad(RenderContext, Shader_TalentNode, Centre.X - 0.5f * Quad,
                   Centre.Y - 0.5f * Quad, Quad, Quad, WithAlpha(Accent, State));
    // NOTE(zoubir): the rank ring just outside the disc
    float Arc = Lift * Size * 1.16f;
    u32 ArcColor = Level >= Def->MaxLevel ? UI_RGBA(255, 214, 110, 255) : Accent;
    DrawShaderQuad(RenderContext, Shader_TalentArc, Centre.X - 0.5f * Arc,
                   Centre.Y - 0.5f * Arc, Arc, Arc,
                   WithAlpha(ArcColor, (float)Level / (float)Def->MaxLevel));
    float Icon = Lift * 0.78f * Size;
    u32 Tint = 0xFFFFFFFF;
    // NOTE(zoubir): in a dungeon run, an ability whose key the role has
    // taken is greyed out whatever its level: its points would do nothing
    bool32 Replaced = RoleReplacesTalent(AppState, Slot, Talent);
    if (Replaced)
    {
        Tint = UI_RGBA(70, 70, 82, 255);
    }
    else if (Level == 0)
    {
        Tint = State >= 0.5f ? UI_RGBA(205, 205, 215, 255) : UI_RGBA(90, 90, 104, 255);
    }
    DrawTexturedQuad(RenderContext, Panel->Atlas, Centre.X - 0.5f * Icon,
                     Centre.Y - 0.5f * Icon, Icon, Icon, TalentIconUvs(TalentIconCell(Slot, Talent)), Tint);
    if (Hot)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Quad,
                       Centre.Y - 0.5f * Quad, Quad, Quad, WithAlpha(Accent, 0.25f),
                       RenderBlend_Additive);
    }
    if (Denied > 0.f)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Quad,
                       Centre.Y - 0.5f * Quad, Quad, Quad,
                       WithAlpha(UI_RGBA(255, 60, 50, 255), 0.6f * Denied), RenderBlend_Additive);
    }
    if (Flash > 0.f)
    {
        float Ring = Size * (1.f + 1.1f * (1.f - Flash));
        DrawShaderQuad(RenderContext, Shader_Ring, Centre.X - 0.5f * Ring,
                       Centre.Y - 0.5f * Ring, Ring, Ring,
                       WithAlpha(UI_RGBA(255, 230, 150, 255), Flash), RenderBlend_Additive);
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Quad,
                       Centre.Y - 0.5f * Quad, Quad, Quad,
                       WithAlpha(UI_RGBA(255, 230, 150, 255), 0.6f * Flash),
                       RenderBlend_Additive);
    }

    // NOTE(zoubir): one pip per rank under the medallion
    float Pip = 9.f;
    float PipGap = 5.f;
    float PipsWidth = (float)Def->MaxLevel * Pip + (float)(Def->MaxLevel - 1) * PipGap;
    float PipX = Centre.X - 0.5f * PipsWidth;
    float PipY = Centre.Y + 0.5f * Arc + 3.f;
    u32 Base = TalentBaseLevel(Talent);
    for(u32 Rank = 0; Rank < Def->MaxLevel; Rank++)
    {
        u32 Color = UI_RGBA(14, 15, 22, 255);
        if (Rank < Base)
        {
            Color = UI_RGBA(200, 206, 220, 255);
        }
        else if (Rank < Level)
        {
            Color = UI_RGBA(255, 206, 90, 255);
        }
        v2 P = V2(PipX + (float)Rank * (Pip + PipGap) + 0.5f * Pip, PipY + 0.5f * Pip);
        // NOTE(zoubir): a diamond, a square turned on its corner, on a
        // lighter rim so an empty one still shows
        u32 Rim = WithAlpha(Accent, 0.7f);
        float H = 0.5f * Pip + 1.5f;
        DrawFilledQuad(RenderContext, P - V2(0.f, H), P + V2(H, 0.f), P + V2(0.f, H),
                       P - V2(H, 0.f), Rim, Rim, Rim, Rim, RenderBlend_Alpha);
        H = 0.5f * Pip;
        DrawFilledQuad(RenderContext, P - V2(0.f, H), P + V2(H, 0.f), P + V2(0.f, H),
                       P - V2(H, 0.f), Color, Color, Color, Color, RenderBlend_Alpha);
    }

    // NOTE(zoubir): an ability's key on its top-left
    font *Small = AppState->Fonts.Small;
    if (Def->Button)
    {
        char *Key = ActionKeyLabel(Def->Button);
        float KeyWidth = UITextWidth(Small, Key) + 8.f;
        float KeyHeight = UILineHeight(Small);
        float KeyX = Centre.X - 0.5f * Arc - 2.f;
        float KeyY = Centre.Y - 0.5f * Arc - 2.f;
        DrawRoundRect(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                      UI_RGBA(12, 13, 20, 235));
        DrawRoundOutline(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                         WithAlpha(Accent, Level ? 0.8f : 0.3f));
        UIText(RenderContext, Small, KeyX + 4.f, KeyY, Key,
               Level ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);
    }
    // NOTE(zoubir): a wild slot of the class tree (sim/dungeon/class_tree.cpp)
    // wears a die on its top-right, gold for the keystone: it rolls again
    // each run
    if (ClassSlotIsWild(Talent))
    {
        bool32 Keystone = ClassSlotIsKeystone(Talent);
        u32 DieColor = Keystone ? UI_RGBA(255, 205, 80, 255) : UI_RGBA(235, 238, 248, 255);
        float Die = 15.f;
        float DieX = Centre.X + 0.5f * Arc - Die + 2.f;
        float DieY = Centre.Y - 0.5f * Arc - 2.f;
        DrawRoundRect(RenderContext, DieX, DieY, Die, Die, UI_RGBA(12, 13, 20, 235));
        DrawRoundOutline(RenderContext, DieX, DieY, Die, Die, WithAlpha(DieColor, 0.9f));
        v2 Dots[3] = {V2(0.28f, 0.28f), V2(0.5f, 0.5f), V2(0.72f, 0.72f)};
        for(u32 Dot = 0; Dot < 3; Dot++)
        {
            DrawFilledRectangle(RenderContext, DieX + Dots[Dot].X * Die - 1.5f,
                                DieY + Dots[Dot].Y * Die - 1.5f, 3.f, 3.f, DieColor, 0.f);
        }
    }
    UIText(RenderContext, Small, Centre.X, PipY + Pip + 6.f, Def->Name,
           (Level && !Replaced) ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED, UIAlign_Center);

    if (Hot && Input->LeftButton.Pressed)
    {
        talent_refusal Refusal = CanLearnTalent(Slot, Talent);
        if (RoleReplacesTalent(AppState, Slot, Talent))
        {
            Panel->Denied[Talent] = 1.f;
            RoleReplacedText(AppState, Slot, Talent, Panel->Message, sizeof(Panel->Message));
            Panel->MessageAge = 0.f;
        }
        else if (Refusal == TalentRefusal_None)
        {
            RequestTalent(AppState, Talent);
            Panel->Message[0] = 0;
        }
        else
        {
            Panel->Denied[Talent] = 1.f;
            TalentRefusalText(Slot, Talent, Refusal, Panel->Message, sizeof(Panel->Message));
            Panel->MessageAge = 0.f;
        }
    }
}
