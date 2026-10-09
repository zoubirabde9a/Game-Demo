/* The talent panel's frame (talent_panel.cpp): the header (title, level
   and experience, points to spend, the close button) and the footer
   (Reset, which takes two clicks, and the last refusal or how to earn
   experience). */

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
    if (Level < TopLevel(AppState))
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
                   WithAlpha(XP_COLOR, LevelProgress(Slot->Xp, TopLevel(AppState))));

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

    u32 Points = TalentPointsToSpend(Slot, IsDungeon(AppState));
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
