/* Talent panel (N, or the button right of the ability bar): the talent
   tree of sim/progression/talents.cpp as three glowing columns, Fire,
   Motion and Guard, four tiers each, top to bottom.

   Each talent is a medallion (build/shaders/fx/talent_node.frag) with its
   icon (talent_icons.cpp): dark while its tier is shut, breathing in its
   branch's colour while a point can go in, gold once every rank is
   bought. Pips under it count its ranks, silver for the free first level
   of an ability the match starts with and gold for bought ones; an
   ability shows its key. Lines join each tier to the next and light up as
   tiers open. Hovering one shows what it does, what the next rank adds
   and why it cannot take a point yet; clicking it spends one
   (RequestTalent, client/talent_requests.cpp), and the medallion flashes
   when the rank arrives.

   The game goes on behind it: keys still move and fight, only clicks on
   the panel stay with the panel. */

#define TALENT_PANEL_MAX_WIDTH 1040.f
#define TALENT_PANEL_MAX_HEIGHT 640.f
#define TALENT_PANEL_HEADER 70.f
#define TALENT_PANEL_FOOTER 34.f
#define TALENT_COLUMN_GAP 16.f
#define TALENT_COLUMN_HEADER 46.f
#define TALENT_NODE_MAX 68.f
#define TALENT_FLASH_SECONDS 0.6f
#define TALENT_TOOLTIP_WIDTH 320.f

global_variable u32 TalentBranchAccents[TalentBranch_Count] =
{
    UI_RGBA(255, 122, 52, 255),
    UI_RGBA(80, 215, 240, 255),
    UI_RGBA(176, 136, 255, 255),
};

struct talent_panel
{
    bool32 Open;
    // NOTE(zoubir): 0..1, eases toward Open for the slide in
    float Shown;
    u32 Atlas;
    float Flash[Talent_Count];
    u8 LastRanks[Talent_Count];
    bool32 RanksSeen;
};

internal talent_panel *
GetTalentPanel(app_state *AppState)
{
    if (!AppState->TalentPanel)
    {
        AppState->TalentPanel = AllocateStruct(&AppState->MemoryArena, talent_panel);
        *AppState->TalentPanel = {};
    }
    return AppState->TalentPanel;
}

internal void
ToggleTalentPanel(app_state *AppState)
{
    talent_panel *Panel = GetTalentPanel(AppState);
    Panel->Open = !Panel->Open;
}

struct talent_panel_layout
{
    float X, Y, Width, Height;
    float ColumnX[TalentBranch_Count];
    float ColumnWidth;
    float ColumnTop, ColumnHeight;
    float TierTop, TierHeight;
    float NodeSize;
};

internal talent_panel_layout
LayTalentPanel(u32 WindowWidth, u32 WindowHeight, float Shown)
{
    talent_panel_layout L = {};
    // NOTE(zoubir): above the ability bar, which keeps showing cooldowns
    float Room = (float)WindowHeight - 150.f;
    L.Width = Minimum(TALENT_PANEL_MAX_WIDTH, (float)WindowWidth - 40.f);
    L.Height = Minimum(TALENT_PANEL_MAX_HEIGHT, Room - 20.f);
    L.X = 0.5f * ((float)WindowWidth - L.Width);
    L.Y = Maximum(10.f, 0.5f * (Room - L.Height)) - 24.f * (1.f - Shown);
    L.ColumnTop = L.Y + TALENT_PANEL_HEADER;
    L.ColumnHeight = L.Height - TALENT_PANEL_HEADER - TALENT_PANEL_FOOTER;
    float Inner = L.Width - 2.f * UI_GAP_LARGE;
    L.ColumnWidth = (Inner - 2.f * TALENT_COLUMN_GAP) / (float)TalentBranch_Count;
    for(u32 Branch = 0; Branch < TalentBranch_Count; Branch++)
    {
        L.ColumnX[Branch] = L.X + UI_GAP_LARGE +
            (float)Branch * (L.ColumnWidth + TALENT_COLUMN_GAP);
    }
    L.TierTop = L.ColumnTop + TALENT_COLUMN_HEADER;
    L.TierHeight = (L.ColumnHeight - TALENT_COLUMN_HEADER - 8.f) / (float)TALENT_TIERS;
    L.NodeSize = Minimum(TALENT_NODE_MAX, 0.56f * L.TierHeight);
    return L;
}

// NOTE(zoubir): how many talents share Tier in Branch (1 or 2)
internal u32
TalentsInTier(u32 Branch, u32 Tier)
{
    u32 Result = 0;
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        Result += (TalentDefs[Talent].Branch == Branch && TalentDefs[Talent].Tier == Tier);
    }
    return Result;
}

internal v2
TalentNodeCentre(talent_panel_layout *L, u32 Talent)
{
    talent_def *Def = &TalentDefs[Talent];
    float ColumnCentre = L->ColumnX[Def->Branch] + 0.5f * L->ColumnWidth;
    float Spread = 0.24f * L->ColumnWidth;
    float X = ColumnCentre;
    if (TalentsInTier(Def->Branch, Def->Tier) > 1)
    {
        X += Def->Column ? Spread : -Spread;
    }
    float Y = L->TierTop + ((float)Def->Tier + 0.42f) * L->TierHeight;
    v2 Result = V2(X, Y);
    return Result;
}

// NOTE(zoubir): a straight band from A to B, Width wide
internal void
DrawUIBand(render_context *RenderContext, v2 A, v2 B, float Width, u32 Color)
{
    v2 Along = B - A;
    float Length = SquareRoot(LengthSq(Along));
    if (Length < 0.01f)
    {
        return;
    }
    v2 Side = (0.5f * Width / Length) * V2(-Along.Y, Along.X);
    DrawFilledQuad(RenderContext, A - Side, B - Side, B + Side, A + Side,
                   Color, Color, Color, Color, RenderBlend_Alpha);
}

// NOTE(zoubir): the medallion state the node shader reads, 0..1
internal float
TalentNodeState(player_slot *Slot, u32 Talent)
{
    talent_def *Def = &TalentDefs[Talent];
    u32 Level = TalentLevel(Slot, Talent);
    talent_refusal Refusal = CanLearnTalent(Slot, Talent);
    float Result = 0.f;
    if (Level >= Def->MaxLevel)
    {
        Result = 1.f;
    }
    else if (Refusal == TalentRefusal_None)
    {
        Result = 0.5f;
    }
    else if (Level > 0)
    {
        Result = 0.7f;
    }
    else if (Refusal == TalentRefusal_NoPoints)
    {
        Result = 0.3f;
    }
    return Result;
}

// NOTE(zoubir): the cooldown behind Button at its base length, 0 for none
internal float
TalentBaseCooldown(world_entity *Player, u32 Button)
{
    float Result = 0.f;
    for(u32 Index = 0; Index < PLAYER_COOLDOWN_COUNT && Player; Index++)
    {
        float Full = 0.f;
        if (PlayerCooldownButton(Index) == Button &&
            PlayerCooldownAtBase(Player, Index, &Full) && Full > Result)
        {
            Result = Full;
        }
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

    char Lines[6][96];
    u32 Colors[6];
    font *Fonts[6];
    u32 Count = 0;

    snprintf(Lines[Count], 96, "%s", Def->Name);
    Colors[Count] = Accent;
    Fonts[Count++] = Strong;
    if (Def->Button)
    {
        snprintf(Lines[Count], 96, "Ability  %s   Level %u of %u",
                 ActionKeyLabel(Def->Button), Level, Def->MaxLevel);
    }
    else
    {
        snprintf(Lines[Count], 96, "Passive   Rank %u of %u", Level, Def->MaxLevel);
    }
    Colors[Count] = UI_COLOR_TEXT_MUTED;
    Fonts[Count++] = Small;
    snprintf(Lines[Count], 96, "%s", Def->Summary);
    Colors[Count] = UI_COLOR_TEXT;
    Fonts[Count++] = Body;

    float Cooldown = Def->Button ? TalentBaseCooldown(Slot->Entity, Def->Button) : 0.f;
    if (Cooldown > 0.f && Level > 0)
    {
        float Now = Cooldown * CooldownScaleForLevel(Level);
        if (Level < Def->MaxLevel)
        {
            snprintf(Lines[Count], 96, "Cooldown %.1f s, next level %.1f s", Now,
                     Cooldown * CooldownScaleForLevel(Level + 1));
        }
        else
        {
            snprintf(Lines[Count], 96, "Cooldown %.1f s", Now);
        }
        Colors[Count] = UI_COLOR_TEXT;
        Fonts[Count++] = Body;
    }
    if (Level < Def->MaxLevel)
    {
        if (Def->Button && Level == 0)
        {
            snprintf(Lines[Count], 96, "Next: unlocks the ability on %s",
                     ActionKeyLabel(Def->Button));
        }
        else
        {
            snprintf(Lines[Count], 96, "Next: %s", Def->PerRank);
        }
        Colors[Count] = UI_RGBA(200, 230, 255, 255);
        Fonts[Count++] = Body;
    }
    switch (Refusal)
    {
        case TalentRefusal_None:
        {
            snprintf(Lines[Count], 96, "Click to spend a point");
            Colors[Count] = UI_COLOR_GOOD;
        } break;
        case TalentRefusal_MaxRank:
        {
            snprintf(Lines[Count], 96, "Fully trained");
            Colors[Count] = UI_COLOR_ACCENT;
        } break;
        case TalentRefusal_TierLocked:
        {
            u32 Spent = TalentPointsSpent(Slot, Def->Branch);
            u32 Need = TalentTierCost(Def->Tier);
            snprintf(Lines[Count], 96, "Spend %u more in %s to open this tier",
                     Need - Spent, TalentBranchNames[Def->Branch]);
            Colors[Count] = UI_RGBA(255, 120, 100, 255);
        } break;
        case TalentRefusal_NoPoints:
        {
            snprintf(Lines[Count], 96, "No points left: level up to earn one");
            Colors[Count] = UI_RGBA(255, 120, 100, 255);
        } break;
    }
    Fonts[Count++] = Body;

    float Width = TALENT_TOOLTIP_WIDTH;
    for(u32 Line = 0; Line < Count; Line++)
    {
        Width = Maximum(Width, UITextWidth(Fonts[Line], Lines[Line]) + 28.f);
    }
    float Height = 18.f;
    for(u32 Line = 0; Line < Count; Line++)
    {
        Height += UILineHeight(Fonts[Line]) + 4.f;
    }
    float X = Node.X + 0.5f * L->NodeSize + 16.f;
    if (X + Width > L->X + L->Width)
    {
        X = Node.X - 0.5f * L->NodeSize - 16.f - Width;
    }
    float Y = Minimum(Node.Y - 0.5f * L->NodeSize, L->Y + L->Height - Height);
    DrawUIPanel(RenderContext, X, Y, Width, Height, Accent);
    float LineY = Y + 10.f;
    for(u32 Line = 0; Line < Count; Line++)
    {
        UIText(RenderContext, Fonts[Line], X + 14.f, LineY, Lines[Line], Colors[Line]);
        LineY += UILineHeight(Fonts[Line]) + 4.f;
    }
}

// NOTE(zoubir): an X of two crossed bands, for the close button
internal void
DrawCloseCross(render_context *RenderContext, float CentreX, float CentreY, float Size,
               u32 Color)
{
    float H = 0.5f * Size;
    DrawUIBand(RenderContext, V2(CentreX - H, CentreY - H), V2(CentreX + H, CentreY + H),
               2.5f, Color);
    DrawUIBand(RenderContext, V2(CentreX - H, CentreY + H), V2(CentreX + H, CentreY - H),
               2.5f, Color);
}

internal void
DrawTalentPanelHeader(render_context *RenderContext, app_state *AppState,
                      app_input *Input, talent_panel *Panel, talent_panel_layout *L)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float CentreY = L->Y + 0.5f * TALENT_PANEL_HEADER;
    UIText(RenderContext, Title, L->X + UI_GAP_LARGE, CentreY - 0.5f * UILineHeight(Title),
           "Talents", UI_COLOR_TEXT);

    // NOTE(zoubir): level and experience in the middle
    u32 Level = ShownLevel(Slot);
    char Text[64];
    float BarWidth = Minimum(300.f, 0.32f * L->Width);
    float BarX = L->X + 0.5f * L->Width - 0.5f * BarWidth;
    snprintf(Text, sizeof(Text), "Level %u", Level);
    UIText(RenderContext, Body, BarX, CentreY - UILineHeight(Body) - 2.f, Text,
           UI_RGBA(255, 226, 150, 255));
    if (Level < PLAYER_MAX_LEVEL)
    {
        snprintf(Text, sizeof(Text), "%u / %u XP", Slot->Xp, XpToReach(Level + 1));
    }
    else
    {
        snprintf(Text, sizeof(Text), "%u XP, top level", Slot->Xp);
    }
    UIText(RenderContext, Small, BarX + BarWidth, CentreY - UILineHeight(Small) - 3.f, Text,
           UI_COLOR_TEXT_MUTED, UIAlign_Right);
    DrawShaderQuad(RenderContext, Shader_XpBar, BarX, CentreY + 3.f, BarWidth, 10.f,
                   WithAlpha(XP_COLOR, LevelProgress(Slot->Xp)));

    // NOTE(zoubir): points to spend, then the close button, on the right
    float CloseSize = 30.f;
    float CloseX = L->X + L->Width - UI_GAP_LARGE - CloseSize;
    float CloseY = CentreY - 0.5f * CloseSize;
    bool32 CloseHot = IsMouseOnRectangle(Input->MouseX, Input->MouseY, CloseX, CloseY,
                                         CloseSize, CloseSize);
    if (CloseHot)
    {
        DrawFilledRectangle(RenderContext, CloseX, CloseY, CloseSize, CloseSize,
                            UI_RGBA(255, 255, 255, 30), 0.f);
    }
    DrawCloseCross(RenderContext, CloseX + 0.5f * CloseSize, CentreY, 12.f,
                   CloseHot ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);
    if (CloseHot && Input->LeftButton.Pressed)
    {
        Panel->Open = false;
    }

    u32 Points = TalentPointsLeft(Slot);
    if (Points)
    {
        snprintf(Text, sizeof(Text), "%u point%s to spend", Points, Points == 1 ? "" : "s");
    }
    else
    {
        snprintf(Text, sizeof(Text), "No points to spend");
    }
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : Body;
    float PillWidth = UITextWidth(Strong, Text) + 28.f;
    float PillHeight = UILineHeight(Strong) + 10.f;
    float PillX = CloseX - 14.f - PillWidth;
    float PillY = CentreY - 0.5f * PillHeight;
    if (Points)
    {
        float Pulse = 0.5f + 0.5f * Sin(4.f * RenderContext->Time);
        DrawShaderQuad(RenderContext, Shader_Glow, PillX - 20.f, PillY - 16.f,
                       PillWidth + 40.f, PillHeight + 32.f,
                       WithAlpha(XP_COLOR, 0.25f + 0.2f * Pulse), RenderBlend_Additive);
    }
    DrawUIPanel(RenderContext, PillX, PillY, PillWidth, PillHeight,
                Points ? XP_COLOR : UI_RGBA(110, 116, 130, 255));
    UIText(RenderContext, Strong, PillX + 0.5f * PillWidth, PillY + 5.f, Text,
           Points ? UI_RGBA(255, 232, 160, 255) : UI_COLOR_TEXT_MUTED, UIAlign_Center);
}

internal void
DrawTalentColumn(render_context *RenderContext, app_state *AppState,
                 talent_panel_layout *L, u32 Branch)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    u32 Accent = TalentBranchAccents[Branch];
    float X = L->ColumnX[Branch];
    u32 Spent = TalentPointsSpent(Slot, Branch);
    DrawShaderQuad(RenderContext, Shader_TalentBackdrop, X, L->ColumnTop, L->ColumnWidth,
                   L->ColumnHeight, WithAlpha(Accent, 0.55f + 0.05f * (float)Minimum(Spent, 8u)));

    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Strong, X + 16.f, L->ColumnTop + 12.f, TalentBranchNames[Branch],
           Accent);
    char Text[32];
    snprintf(Text, sizeof(Text), "%u spent", Spent);
    UIText(RenderContext, Small, X + L->ColumnWidth - 16.f, L->ColumnTop + 15.f, Text,
           UI_COLOR_TEXT_MUTED, UIAlign_Right);

    // NOTE(zoubir): each tier's threshold on the left edge: a lit notch
    // once open, a lock and the points it needs while shut
    for(u32 Tier = 1; Tier < TALENT_TIERS; Tier++)
    {
        float Y = L->TierTop + (float)Tier * L->TierHeight - 6.f;
        bool32 Open = IsTalentTierOpen(Slot, Branch, Tier);
        u32 Line = Open ? WithAlpha(Accent, 0.35f) : UI_RGBA(255, 255, 255, 18);
        DrawFilledRectangle(RenderContext, X + 12.f, Y, L->ColumnWidth - 24.f, 1.f, Line, 0.f);
        if (!Open)
        {
            snprintf(Text, sizeof(Text), "%u", TalentTierCost(Tier));
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

    // NOTE(zoubir): lines from each tier to the next, lit once the lower
    // one is open
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        talent_def *Def = &TalentDefs[Talent];
        if (Def->Branch != Branch)
        {
            continue;
        }
        for(u32 Next = 0; Next < Talent_Count; Next++)
        {
            talent_def *NextDef = &TalentDefs[Next];
            bool32 Joined = NextDef->Branch == Branch && NextDef->Tier == Def->Tier + 1 &&
                (NextDef->Column == Def->Column || TalentsInTier(Branch, NextDef->Tier) == 1);
            if (!Joined)
            {
                continue;
            }
            v2 From = TalentNodeCentre(L, Talent) + V2(0.f, 0.5f * L->NodeSize + 10.f);
            v2 To = TalentNodeCentre(L, Next) - V2(0.f, 0.5f * L->NodeSize + 4.f);
            bool32 Lit = IsTalentTierOpen(Slot, Branch, NextDef->Tier);
            DrawUIBand(RenderContext, From, To, Lit ? 3.f : 2.f,
                       Lit ? WithAlpha(Accent, 0.7f) : UI_RGBA(255, 255, 255, 24));
        }
    }
}

// NOTE(zoubir): one talent's medallion, icon, key and pips; returns
// whether the mouse is on it
internal bool32
DrawTalentNode(render_context *RenderContext, app_state *AppState, app_input *Input,
               talent_panel *Panel, talent_panel_layout *L, u32 Talent)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    talent_def *Def = &TalentDefs[Talent];
    u32 Accent = TalentBranchAccents[Def->Branch];
    v2 Centre = TalentNodeCentre(L, Talent);
    float Size = L->NodeSize;
    bool32 Hot = IsMouseOnRectangle(Input->MouseX, Input->MouseY, Centre.X - 0.5f * Size,
                                    Centre.Y - 0.5f * Size, Size, Size);
    float State = TalentNodeState(Slot, Talent);
    u32 Level = TalentLevel(Slot, Talent);
    float Flash = Panel->Flash[Talent];
    float Lift = Hot ? 1.06f : 1.f;
    float Quad = Lift * Size / 0.74f;
    DrawShaderQuad(RenderContext, Shader_TalentNode, Centre.X - 0.5f * Quad,
                   Centre.Y - 0.5f * Quad, Quad, Quad, WithAlpha(Accent, State));
    float Icon = Lift * 0.78f * Size;
    u32 Tint = 0xFFFFFFFF;
    if (Level == 0)
    {
        Tint = State >= 0.5f ? UI_RGBA(205, 205, 215, 255) : UI_RGBA(95, 95, 108, 255);
    }
    DrawTexturedQuad(RenderContext, Panel->Atlas, Centre.X - 0.5f * Icon,
                     Centre.Y - 0.5f * Icon, Icon, Icon, TalentIconUvs(Talent), Tint);
    if (Hot)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, Centre.X - 0.5f * Quad,
                       Centre.Y - 0.5f * Quad, Quad, Quad, WithAlpha(Accent, 0.25f),
                       RenderBlend_Additive);
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
    float PipY = Centre.Y + 0.5f * Size + 6.f;
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
    if (Def->Button)
    {
        font *Small = AppState->Fonts.Small;
        char *Key = ActionKeyLabel(Def->Button);
        float KeyWidth = UITextWidth(Small, Key) + 8.f;
        float KeyHeight = UILineHeight(Small);
        float KeyX = Centre.X - 0.5f * Size - 4.f;
        float KeyY = Centre.Y - 0.5f * Size - 2.f;
        DrawFilledRectangle(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                            UI_RGBA(8, 9, 14, 230), 0.f);
        DrawRectangle(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                      WithAlpha(Accent, Level ? 0.8f : 0.3f), 0.f);
        UIText(RenderContext, Small, KeyX + 4.f, KeyY, Key,
               Level ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED);
    }

    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Small, Centre.X, PipY + Pip + 5.f, Def->Name,
           Level ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED, UIAlign_Center);

    if (Hot && Input->LeftButton.Pressed &&
        CanLearnTalent(Slot, Talent) == TalentRefusal_None)
    {
        RequestTalent(AppState, Talent);
    }
    return Hot;
}

internal void
DrawTalentPanel(render_context *RenderContext, app_state *AppState, app_input *Input,
                u32 WindowWidth, u32 WindowHeight)
{
    talent_panel *Panel = GetTalentPanel(AppState);
    talent_requests *Requests = GetTalentRequests(AppState);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    bool32 KeysToUi = ConnectScreenTakesInput(AppState);
    if (!KeysToUi && Input->ButtonN.Pressed)
    {
        ToggleTalentPanel(AppState);
    }
    // NOTE(zoubir): a rank that arrived flashes its medallion, whether it
    // came from a click here or the server
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        if (Panel->RanksSeen && Slot->Ranks[Talent] > Panel->LastRanks[Talent])
        {
            Panel->Flash[Talent] = 1.f;
        }
        Panel->LastRanks[Talent] = Slot->Ranks[Talent];
        Panel->Flash[Talent] = Maximum(0.f, Panel->Flash[Talent] -
                                       Input->DeltaTime / TALENT_FLASH_SECONDS);
    }
    Panel->RanksSeen = true;

    float Target = Panel->Open ? 1.f : 0.f;
    Panel->Shown += (Target - Panel->Shown) * Minimum(1.f, 14.f * Input->DeltaTime);
    bool32 Visible = Panel->Open && Slot->Active && Slot->Entity && !KeysToUi;
    Requests->PanelOpen = Visible;
    if (!Visible)
    {
        return;
    }
    if (!Panel->Atlas)
    {
        Panel->Atlas = BuildTalentIconAtlas(RenderContext->OpenGL, RenderContext->Arena);
    }

    talent_panel_layout L = LayTalentPanel(WindowWidth, WindowHeight, Panel->Shown);
    Requests->PanelX = L.X;
    Requests->PanelY = L.Y;
    Requests->PanelWidth = L.Width;
    Requests->PanelHeight = L.Height;

    // NOTE(zoubir): a dark sheet under the glass, so the world and the
    // screens behind do not show through the tree
    DrawFilledRectangle(RenderContext, L.X + 6.f, L.Y + 6.f, L.Width - 12.f, L.Height - 12.f,
                        UI_RGBA(8, 9, 14, 235), 0.f);
    DrawUIPanel(RenderContext, L.X, L.Y, L.Width, L.Height, UI_RGBA(255, 210, 120, 255));
    DrawTalentPanelHeader(RenderContext, AppState, Input, Panel, &L);
    for(u32 Branch = 0; Branch < TalentBranch_Count; Branch++)
    {
        DrawTalentColumn(RenderContext, AppState, &L, Branch);
    }
    i32 Hovered = -1;
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        if (DrawTalentNode(RenderContext, AppState, Input, Panel, &L, Talent))
        {
            Hovered = (i32)Talent;
        }
    }

    font *Small = AppState->Fonts.Small;
    UIText(RenderContext, Small, L.X + 0.5f * L.Width,
           L.Y + L.Height - 0.5f * TALENT_PANEL_FOOTER - 0.5f * UILineHeight(Small),
           "A level is one point.  XP: kill a player +100 (more if they out-level you),"
           " a monster +5, and +2 every second you play.",
           UI_COLOR_TEXT_MUTED, UIAlign_Center);

    if (Hovered >= 0)
    {
        DrawTalentTooltip(RenderContext, AppState, &L, (u32)Hovered,
                          TalentNodeCentre(&L, (u32)Hovered));
    }
}
