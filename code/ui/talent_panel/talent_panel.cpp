/* Talent panel (N, or the button right of the ability bar): the talent
   tree of sim/progression/talents.cpp as three glowing columns, Fire,
   Motion and Guard, four tiers each, top to bottom, and the player's
   stats down the right (talent_stats.cpp).

   Each talent is a medallion (build/shaders/fx/talent_node.frag) with its
   icon (talent_icons.cpp) inside a ring that fills with its ranks
   (talent_arc.frag): dark while its tier is shut, breathing in its
   branch's colour while a point can go in, gold once every rank is
   bought. Pips under it count its ranks, silver for the free first level
   of an ability the match starts with and gold for bought ones; an
   ability shows its key. Lines join each tier to the next; light runs
   along them once the tier below is open, and hovering a talent lights
   the lines into it.

   Hovering shows what it does now and at its next rank in numbers
   (talent_effects.cpp) and why it cannot take a point yet; clicking it
   spends one (RequestTalent, client/talent_requests.cpp), and the
   medallion flashes when the rank arrives. A click it refuses shakes the
   medallion and says why at the foot of the panel. Reset (two clicks)
   gives every point back.

   The game goes on behind it: keys still move and fight, only clicks on
   the panel stay with the panel. */

#define TALENT_PANEL_MAX_WIDTH 1260.f
#define TALENT_PANEL_MAX_HEIGHT 660.f
#define TALENT_PANEL_HEADER 70.f
#define TALENT_PANEL_FOOTER 40.f
#define TALENT_SIDEBAR_WIDTH 250.f
#define TALENT_COLUMN_GAP 16.f
#define TALENT_COLUMN_HEADER 46.f
#define TALENT_NODE_MAX 66.f
#define TALENT_FLASH_SECONDS 0.6f
#define TALENT_DENIED_SECONDS 0.45f
#define TALENT_MESSAGE_SECONDS 2.5f
#define TALENT_FOCUS_SECONDS 2.5f
#define TALENT_RESET_ARM_SECONDS 3.f
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
    float Denied[Talent_Count];
    u8 LastRanks[Talent_Count];
    bool32 RanksSeen;
    // NOTE(zoubir): the talent the ability bar sent the player to, pulsed
    // for a moment; -1 for none
    i32 Focus;
    float FocusAge;
    // NOTE(zoubir): why the last click was refused, shown in the footer
    char Message[96];
    float MessageAge;
    // NOTE(zoubir): seconds left in which a second click on Reset resets
    float ResetArmed;
    float Clock;
};

internal talent_panel *
GetTalentPanel(app_state *AppState)
{
    if (!AppState->TalentPanel)
    {
        AppState->TalentPanel = AllocateStruct(&AppState->MemoryArena, talent_panel);
        *AppState->TalentPanel = {};
        AppState->TalentPanel->Focus = -1;
        AppState->TalentPanel->MessageAge = TALENT_MESSAGE_SECONDS;
    }
    return AppState->TalentPanel;
}

internal void
ToggleTalentPanel(app_state *AppState)
{
    talent_panel *Panel = GetTalentPanel(AppState);
    Panel->Open = !Panel->Open;
}

// NOTE(zoubir): opens the panel with Talent pulsing, for the ability
// bar's upgrade badges
internal void
ShowTalentInPanel(app_state *AppState, u32 Talent)
{
    talent_panel *Panel = GetTalentPanel(AppState);
    Panel->Open = true;
    Panel->Focus = (i32)Talent;
    Panel->FocusAge = 0.f;
}

struct talent_panel_layout
{
    float X, Y, Width, Height;
    float ColumnX[TalentBranch_Count];
    float ColumnWidth;
    float ColumnTop, ColumnHeight;
    float TierTop, TierHeight;
    float NodeSize;
    float SidebarX;
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
    // NOTE(zoubir): the sidebar goes when the window is too narrow for it
    float Sidebar = L.Width >= 900.f ? TALENT_SIDEBAR_WIDTH : 0.f;
    float Inner = L.Width - 2.f * UI_GAP_LARGE - (Sidebar > 0.f ? Sidebar + TALENT_COLUMN_GAP : 0.f);
    L.ColumnWidth = (Inner - 2.f * TALENT_COLUMN_GAP) / (float)TalentBranch_Count;
    for(u32 Branch = 0; Branch < TalentBranch_Count; Branch++)
    {
        L.ColumnX[Branch] = L.X + UI_GAP_LARGE +
            (float)Branch * (L.ColumnWidth + TALENT_COLUMN_GAP);
    }
    L.SidebarX = Sidebar > 0.f ? L.X + L.Width - UI_GAP_LARGE - Sidebar : 0.f;
    L.TierTop = L.ColumnTop + TALENT_COLUMN_HEADER;
    L.TierHeight = (L.ColumnHeight - TALENT_COLUMN_HEADER - 8.f) / (float)TALENT_TIERS;
    L.NodeSize = Minimum(TALENT_NODE_MAX, 0.5f * L.TierHeight);
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
    float Y = L->TierTop + ((float)Def->Tier + 0.4f) * L->TierHeight;
    v2 Result = V2(X, Y);
    return Result;
}

// NOTE(zoubir): a straight band from A to B, Width wide
internal void
DrawUIBand(render_context *RenderContext, v2 A, v2 B, float Width, u32 Color,
           u32 Blend = RenderBlend_Alpha)
{
    v2 Along = B - A;
    float Length = SquareRoot(LengthSq(Along));
    if (Length < 0.01f)
    {
        return;
    }
    v2 Side = (0.5f * Width / Length) * V2(-Along.Y, Along.X);
    DrawFilledQuad(RenderContext, A - Side, B - Side, B + Side, A + Side,
                   Color, Color, Color, Color, Blend);
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

// NOTE(zoubir): why Talent will not take a point, as a sentence
internal void
TalentRefusalText(player_slot *Slot, u32 Talent, talent_refusal Refusal, char *Out, u32 OutSize)
{
    talent_def *Def = &TalentDefs[Talent];
    switch (Refusal)
    {
        case TalentRefusal_None:
        {
            snprintf(Out, OutSize, "Click to spend a point");
        } break;
        case TalentRefusal_MaxRank:
        {
            snprintf(Out, OutSize, "%s is fully trained", Def->Name);
        } break;
        case TalentRefusal_TierLocked:
        {
            u32 Spent = TalentPointsSpent(Slot, Def->Branch);
            u32 Need = TalentTierCost(Def->Tier);
            snprintf(Out, OutSize, "Spend %u more in %s to open this tier",
                     Need - Spent, TalentBranchNames[Def->Branch]);
        } break;
        case TalentRefusal_NoPoints:
        {
            snprintf(Out, OutSize, "No points left: kills, monsters and time earn the next");
        } break;
    }
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

#include "talent_effects.cpp"
#include "talent_tooltip.cpp"
#include "talent_nodes.cpp"
#include "talent_stats.cpp"

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
    float BarWidth = Minimum(320.f, 0.3f * L->Width);
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

// NOTE(zoubir): Reset on the left of the footer, the last refusal or the
// way to earn experience in the middle
internal void
DrawTalentPanelFooter(render_context *RenderContext, app_state *AppState,
                      app_input *Input, talent_panel *Panel, talent_panel_layout *L)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    font *Small = AppState->Fonts.Small;
    font *Body = AppState->Fonts.Body;
    float CentreY = L->Y + L->Height - 0.5f * TALENT_PANEL_FOOTER - 2.f;

    u32 Spent = TalentPointsSpent(Slot);
    char *Label = (char *)(Panel->ResetArmed > 0.f ? "Click again to reset" : "Reset talents");
    float ButtonWidth = UITextWidth(Small, Label) + 24.f;
    float ButtonHeight = UILineHeight(Small) + 10.f;
    float ButtonX = L->X + UI_GAP_LARGE;
    float ButtonY = CentreY - 0.5f * ButtonHeight;
    bool32 Hot = Spent && IsMouseOnRectangle(Input->MouseX, Input->MouseY, ButtonX, ButtonY,
                                             ButtonWidth, ButtonHeight);
    u32 Tint = Panel->ResetArmed > 0.f ? UI_RGBA(255, 110, 90, 255) :
        (Spent ? UI_RGBA(150, 160, 190, 255) : UI_RGBA(70, 74, 86, 255));
    DrawUIPanel(RenderContext, ButtonX, ButtonY, ButtonWidth, ButtonHeight, Tint);
    if (Hot)
    {
        DrawFilledRectangle(RenderContext, ButtonX + 3.f, ButtonY + 3.f, ButtonWidth - 6.f,
                            ButtonHeight - 6.f, UI_RGBA(255, 255, 255, 20), 0.f);
    }
    UIText(RenderContext, Small, ButtonX + 0.5f * ButtonWidth, ButtonY + 5.f, Label,
           Spent ? (Panel->ResetArmed > 0.f ? UI_RGBA(255, 170, 150, 255) : UI_COLOR_TEXT) :
           UI_COLOR_TEXT_MUTED, UIAlign_Center);
    if (Hot && Input->LeftButton.Pressed)
    {
        if (Panel->ResetArmed > 0.f)
        {
            RequestTalentReset(AppState);
            Panel->ResetArmed = 0.f;
            snprintf(Panel->Message, sizeof(Panel->Message), "Every point is back to spend");
            Panel->MessageAge = 0.f;
        }
        else
        {
            Panel->ResetArmed = TALENT_RESET_ARM_SECONDS;
        }
    }

    float Middle = 0.5f * (ButtonX + ButtonWidth + (L->SidebarX ? L->SidebarX : L->X + L->Width));
    if (Panel->MessageAge < TALENT_MESSAGE_SECONDS && Panel->Message[0])
    {
        float Fade = Minimum(1.f, 2.f * (TALENT_MESSAGE_SECONDS - Panel->MessageAge));
        UIText(RenderContext, Body, Middle, CentreY - 0.5f * UILineHeight(Body),
               Panel->Message, WithAlpha(UI_RGBA(255, 190, 150, 255), Fade), UIAlign_Center);
    }
    else
    {
        UIText(RenderContext, Small, Middle, CentreY - 0.5f * UILineHeight(Small),
               "A level is a point.  Kill a player +100 XP, a monster +5, and +2 every second.",
               UI_COLOR_TEXT_MUTED, UIAlign_Center);
    }
}

internal void
DrawTalentPanel(render_context *RenderContext, app_state *AppState, app_input *Input,
                u32 WindowWidth, u32 WindowHeight)
{
    talent_panel *Panel = GetTalentPanel(AppState);
    talent_requests *Requests = GetTalentRequests(AppState);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    bool32 KeysToUi = ConnectScreenTakesInput(AppState);
    float DeltaTime = Input->DeltaTime;
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
                                       DeltaTime / TALENT_FLASH_SECONDS);
        Panel->Denied[Talent] = Maximum(0.f, Panel->Denied[Talent] -
                                        DeltaTime / TALENT_DENIED_SECONDS);
    }
    Panel->RanksSeen = true;
    Panel->Clock += DeltaTime;
    Panel->MessageAge += DeltaTime;
    Panel->FocusAge += DeltaTime;
    Panel->ResetArmed = Maximum(0.f, Panel->ResetArmed - DeltaTime);
    if (Panel->FocusAge > TALENT_FOCUS_SECONDS)
    {
        Panel->Focus = -1;
    }

    float Target = Panel->Open ? 1.f : 0.f;
    Panel->Shown += (Target - Panel->Shown) * Minimum(1.f, 14.f * DeltaTime);
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

    // NOTE(zoubir): which talent the mouse is on, first, so the lines into
    // it can light up under the medallions
    i32 Hovered = -1;
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        v2 Centre = TalentNodeCentre(&L, Talent);
        if (IsMouseOnRectangle(Input->MouseX, Input->MouseY, Centre.X - 0.5f * L.NodeSize,
                               Centre.Y - 0.5f * L.NodeSize, L.NodeSize, L.NodeSize))
        {
            Hovered = (i32)Talent;
        }
    }
    for(u32 Branch = 0; Branch < TalentBranch_Count; Branch++)
    {
        DrawTalentColumn(RenderContext, AppState, Panel, &L, Branch, Hovered);
    }
    for(u32 Talent = 0; Talent < Talent_Count; Talent++)
    {
        DrawTalentNode(RenderContext, AppState, Input, Panel, &L, Talent,
                       Hovered == (i32)Talent);
    }
    if (L.SidebarX > 0.f)
    {
        DrawTalentStats(RenderContext, AppState, &L);
    }
    DrawTalentPanelFooter(RenderContext, AppState, Input, Panel, &L);
    if (Hovered >= 0)
    {
        DrawTalentTooltip(RenderContext, AppState, &L, (u32)Hovered,
                          TalentNodeCentre(&L, (u32)Hovered));
    }
}
