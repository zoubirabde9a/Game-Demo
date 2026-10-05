/* Experience on screen: what the player sees of sim/progression/ outside
   the talent panel.

   - The XP strip: a thin liquid-gold bar inside the ability bar's plate,
     between health and the slots (ability_bar.cpp draws it with
     DrawXpStrip, here).
   - The level badge left of the plate: the level in a medallion with a
     ring filling toward the next one.
   - The talent button right of the plate while points wait to be spent:
     it pulses and opens the talent panel (also N).
   - Gains: "+100 XP" rising from the strip for a kill, smaller for a
     monster. The trickle over time is too small to call out.
   - Level up: a banner across the top of the screen with the new level
     and how many points wait.

   Offline the slot's experience comes from the local simulation, online
   from snapshots (client/replicas/apply.cpp); this file only watches it
   change. */

#define XP_STRIP_HEIGHT 8.f
#define XP_BADGE_SIZE 52.f
#define XP_GAIN_SECONDS 1.4f
#define XP_GAIN_RISE 46.f
#define XP_GAINS 6
// NOTE(zoubir): a gain this big or bigger gets its own number; smaller
// ones (the trickle) only move the bar
#define XP_GAIN_SHOWN 5
#define LEVEL_BANNER_SECONDS 3.2f
#define XP_TOAST_SECONDS 2.8f
#define XP_COLOR UI_RGBA(255, 196, 70, 255)

struct xp_gain
{
    u32 Amount;
    float Age;
};

struct xp_bar
{
    // NOTE(zoubir): what the slot held last frame; a jump in it is a gain
    u32 LastXp;
    u32 LastLevel;
    bool32 Seen;
    xp_gain Gains[XP_GAINS];
    // NOTE(zoubir): the strip shows this, chasing the real share so a
    // gain fills it smoothly
    float ShownShare;
    float BannerAge;
    u32 BannerLevel;
    // NOTE(zoubir): "Frost Nova unlocked", over the ability bar for a moment
    // after the talent tree gives an ability (ability_bar.cpp)
    char Toast[64];
    u32 ToastAccent;
    float ToastAge;
    // NOTE(zoubir): the ability bar's plate last frame, which the badge,
    // the button and the gains are placed around
    float PlateX, PlateY, PlateWidth, PlateHeight;
    float StripX, StripY, StripWidth;
};

internal xp_bar *
GetXpBar(app_state *AppState)
{
    if (!AppState->XpBar)
    {
        AppState->XpBar = AllocateStruct(&AppState->MemoryArena, xp_bar);
        *AppState->XpBar = {};
        AppState->XpBar->BannerAge = LEVEL_BANNER_SECONDS;
        AppState->XpBar->ToastAge = XP_TOAST_SECONDS;
    }
    return AppState->XpBar;
}

// NOTE(zoubir): the level shown: the slot's, never below 1
inline u32
ShownLevel(player_slot *Slot)
{
    u32 Result = Slot->Level ? Slot->Level : 1;
    return Result;
}

// NOTE(zoubir): watches the local slot for gains and new levels
internal void
TrackExperience(app_state *AppState, xp_bar *Bar, float DeltaTime)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    u32 Level = ShownLevel(Slot);
    if (!Bar->Seen || Slot->Xp < Bar->LastXp)
    {
        // NOTE(zoubir): first look, or a new slot or server: nothing to
        // celebrate
        Bar->Seen = true;
        Bar->LastXp = Slot->Xp;
        Bar->LastLevel = Level;
        Bar->ShownShare = LevelProgress(Slot->Xp);
    }
    u32 Gain = Slot->Xp - Bar->LastXp;
    if (Gain >= XP_GAIN_SHOWN && Bar->Gains[0].Amount &&
        Bar->Gains[0].Age < 0.35f)
    {
        // NOTE(zoubir): gains close together (a kill and its level, two
        // kills) add up into one number instead of piling on each other
        Bar->Gains[0].Amount += Gain;
        Bar->Gains[0].Age = 0.f;
    }
    else if (Gain >= XP_GAIN_SHOWN)
    {
        for(u32 Index = XP_GAINS - 1; Index > 0; Index--)
        {
            Bar->Gains[Index] = Bar->Gains[Index - 1];
        }
        Bar->Gains[0].Amount = Gain;
        Bar->Gains[0].Age = 0.f;
    }
    // NOTE(zoubir): the trickle moves the strip but gets no number
    Bar->LastXp = Slot->Xp;
    if (Level > Bar->LastLevel)
    {
        Bar->BannerAge = 0.f;
        Bar->BannerLevel = Level;
        // NOTE(zoubir): the strip empties into the new level
        Bar->ShownShare = 0.f;
    }
    Bar->LastLevel = Level;
    float Share = LevelProgress(Slot->Xp);
    Bar->ShownShare += (Share - Bar->ShownShare) * Minimum(1.f, 6.f * DeltaTime);
    Bar->BannerAge += DeltaTime;
    Bar->ToastAge += DeltaTime;
    for(u32 Index = 0; Index < XP_GAINS; Index++)
    {
        Bar->Gains[Index].Age += DeltaTime;
    }
}

// NOTE(zoubir): the strip, inside the ability bar's plate (ability_bar.cpp)
internal void
DrawXpStrip(render_context *RenderContext, app_state *AppState,
            float X, float Y, float Width)
{
    xp_bar *Bar = GetXpBar(AppState);
    Bar->StripX = X;
    Bar->StripY = Y;
    Bar->StripWidth = Width;
    DrawShaderQuad(RenderContext, Shader_XpBar, X, Y, Width, XP_STRIP_HEIGHT,
                   WithAlpha(XP_COLOR, Bar->ShownShare));
}

// NOTE(zoubir): a rounded pill of text, for keys, counts and badges
internal void
DrawUIPill(render_context *RenderContext, font *Font, float CentreX, float Y,
           char *Text, u32 Fill, u32 Border, u32 TextColor)
{
    float Width = UITextWidth(Font, Text) + 12.f;
    float Height = UILineHeight(Font) + 2.f;
    float X = CentreX - 0.5f * Width;
    DrawFilledRectangle(RenderContext, X, Y, Width, Height, Fill, 0.f);
    DrawRectangle(RenderContext, X, Y, Width, Height, Border, 0.f);
    UIText(RenderContext, Font, CentreX, Y + 1.f, Text, TextColor, UIAlign_Center);
}

// NOTE(zoubir): the level badge left of the plate: a medallion with the
// level, ringed by how far the next level is
internal void
DrawLevelBadge(render_context *RenderContext, app_state *AppState,
               float CentreX, float CentreY)
{
    xp_bar *Bar = GetXpBar(AppState);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    float Size = XP_BADGE_SIZE;
    float Banner = Clamp01(1.f - Bar->BannerAge / 0.6f);
    float Quad = Size / 0.74f;
    DrawShaderQuad(RenderContext, Shader_TalentNode, CentreX - 0.5f * Quad,
                   CentreY - 0.5f * Quad, Quad, Quad,
                   WithAlpha(XP_COLOR, 0.5f + 0.5f * Banner));
    // NOTE(zoubir): the ring toward the next level (talent_arc.frag)
    float Arc = Size * 1.22f;
    DrawShaderQuad(RenderContext, Shader_TalentArc, CentreX - 0.5f * Arc, CentreY - 0.5f * Arc,
                   Arc, Arc, WithAlpha(XP_COLOR, Bar->ShownShare));
    if (Banner > 0.f)
    {
        float Ring = Size * (1.f + 1.2f * (1.f - Banner));
        DrawShaderQuad(RenderContext, Shader_Ring, CentreX - 0.5f * Ring, CentreY - 0.5f * Ring,
                       Ring, Ring, WithAlpha(XP_COLOR, Banner), RenderBlend_Additive);
    }
    font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    char Text[8];
    snprintf(Text, sizeof(Text), "%u", ShownLevel(Slot));
    UIText(RenderContext, Title, CentreX, CentreY - 0.5f * UILineHeight(Title) + 1.f, Text,
           UI_COLOR_TEXT, UIAlign_Center);
    UIText(RenderContext, Small, CentreX, CentreY + 0.5f * Size - 4.f, "LEVEL",
           UI_RGBA(255, 220, 140, 255), UIAlign_Center);
}

// NOTE(zoubir): the talent button right of the plate while points wait;
// returns whether it was clicked this frame
internal bool32
DrawTalentPointsButton(render_context *RenderContext, app_state *AppState,
                       app_input *Input, float X, float CentreY, bool32 PanelOpen)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    u32 Points = TalentPointsLeft(Slot);
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    char Text[32];
    if (Points)
    {
        snprintf(Text, sizeof(Text), "+%u", Points);
    }
    else
    {
        snprintf(Text, sizeof(Text), "Talents");
    }
    float Size = 44.f;
    float Y = CentreY - 0.5f * Size;
    bool32 Hot = IsMouseOnRectangle(Input->MouseX, Input->MouseY, X, Y, Size, Size);
    float Pulse = Points ? 0.5f + 0.5f * Sin(4.f * RenderContext->Time) : 0.f;
    float Quad = Size / 0.78f;
    u32 Accent = Points ? XP_COLOR : UI_RGBA(150, 160, 190, 255);
    DrawShaderQuad(RenderContext, Shader_SlotFrame, X + 0.5f * Size - 0.5f * Quad,
                   CentreY - 0.5f * Quad, Quad, Quad,
                   WithAlpha(Accent, Points ? 0.6f + 0.4f * Pulse : (Hot || PanelOpen ? 0.5f : 0.15f)));
    if (Points)
    {
        DrawShaderQuad(RenderContext, Shader_Glow, X + 0.5f * Size - 0.5f * Quad,
                       CentreY - 0.5f * Quad, Quad, Quad,
                       WithAlpha(XP_COLOR, 0.35f * Pulse), RenderBlend_Additive);
        UIText(RenderContext, Strong, X + 0.5f * Size,
               CentreY - 0.5f * UILineHeight(Strong) - 1.f, Text, UI_COLOR_TEXT,
               UIAlign_Center);
    }
    else
    {
        // NOTE(zoubir): a little tree: a trunk and three nodes
        u32 Line = Hot || PanelOpen ? UI_COLOR_TEXT : UI_COLOR_TEXT_MUTED;
        float CX = X + 0.5f * Size;
        DrawFilledRectangle(RenderContext, CX - 1.f, CentreY - 8.f, 2.f, 16.f, Line, 0.f);
        DrawFilledRectangle(RenderContext, CX - 9.f, CentreY - 2.f, 18.f, 2.f, Line, 0.f);
        DrawFilledRectangle(RenderContext, CX - 3.f, CentreY - 12.f, 6.f, 6.f, Line, 0.f);
        DrawFilledRectangle(RenderContext, CX - 12.f, CentreY - 4.f, 6.f, 6.f, Line, 0.f);
        DrawFilledRectangle(RenderContext, CX + 6.f, CentreY - 4.f, 6.f, 6.f, Line, 0.f);
    }
    UIText(RenderContext, Small, X + 0.5f * Size, CentreY + 0.5f * Size + 1.f, "N",
           UI_COLOR_TEXT_MUTED, UIAlign_Center);
    bool32 Result = Hot && Input->LeftButton.Pressed;
    if (Hot)
    {
        char Tip[64];
        if (Points)
        {
            snprintf(Tip, sizeof(Tip), "%u talent point%s to spend  (N)", Points,
                     Points == 1 ? "" : "s");
        }
        else
        {
            snprintf(Tip, sizeof(Tip), "Talent tree  (N)");
        }
        font *Body = AppState->Fonts.Body;
        float TipWidth = UITextWidth(Body, Tip) + 24.f;
        float TipHeight = UILineHeight(Body) + 12.f;
        float TipX = X + Size - TipWidth;
        float TipY = Y - TipHeight - 14.f;
        DrawUIPanel(RenderContext, TipX, TipY, TipWidth, TipHeight, Accent);
        UIText(RenderContext, Body, TipX + 12.f, TipY + 6.f, Tip, UI_COLOR_TEXT);
    }
    return Result;
}

// NOTE(zoubir): an ability just unlocked, said over the ability bar
internal void
ShowUnlockToast(app_state *AppState, char *Name, char *Key, u32 Accent)
{
    xp_bar *Bar = GetXpBar(AppState);
    snprintf(Bar->Toast, sizeof(Bar->Toast), "%s unlocked  -  %s", Name, Key);
    Bar->ToastAccent = Accent;
    Bar->ToastAge = 0.f;
}

// NOTE(zoubir): the gains rising from the strip and the level-up banner
internal void
DrawXpOverlays(render_context *RenderContext, app_state *AppState,
               u32 WindowWidth, u32 WindowHeight)
{
    xp_bar *Bar = GetXpBar(AppState);
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    for(u32 Index = 0; Index < XP_GAINS; Index++)
    {
        xp_gain *Gain = &Bar->Gains[Index];
        if (!Gain->Amount || Gain->Age >= XP_GAIN_SECONDS || Bar->StripWidth <= 0.f)
        {
            continue;
        }
        float T = Gain->Age / XP_GAIN_SECONDS;
        float Fade = Minimum(1.f, 3.f * (1.f - T));
        float Pop = 1.f + 0.4f * Clamp01(1.f - T * 6.f);
        char Text[24];
        snprintf(Text, sizeof(Text), "+%u XP", Gain->Amount);
        font *Font = Gain->Amount >= XP_PLAYER_KILL_MIN ? Strong : Small;
        float X = Bar->StripX + 0.5f * Bar->StripWidth + 70.f * (float)(Index % 3) - 70.f;
        float Y = Bar->PlateY - 20.f - XP_GAIN_RISE * (1.f - (1.f - T) * (1.f - T)) * Pop;
        UIText(RenderContext, Font, X, Y, Text, WithAlpha(XP_COLOR, Fade), UIAlign_Center);
    }

    if (Bar->ToastAge < XP_TOAST_SECONDS && Bar->Toast[0] && Bar->PlateWidth > 0.f)
    {
        float In = Clamp01(Bar->ToastAge / 0.2f);
        float Out = Clamp01((XP_TOAST_SECONDS - Bar->ToastAge) / 0.5f);
        float Alpha = In * Out;
        font *Body = AppState->Fonts.Body;
        float Width = UITextWidth(Strong, Bar->Toast) + 40.f;
        float Height = UILineHeight(Strong) + 16.f;
        float X = Bar->PlateX + 0.5f * Bar->PlateWidth - 0.5f * Width;
        float Y = Bar->PlateY - Height - 52.f - 8.f * (1.f - In);
        DrawShaderQuad(RenderContext, Shader_Glow, X - 30.f, Y - 20.f, Width + 60.f, Height + 40.f,
                       WithAlpha(Bar->ToastAccent, 0.4f * Alpha), RenderBlend_Additive);
        if (Alpha > 0.05f)
        {
            DrawFilledRectangle(RenderContext, X + 4.f, Y + 4.f, Width - 8.f, Height - 8.f,
                                WithAlpha(UI_RGBA(8, 9, 14, 255), 0.9f * Alpha), 0.f);
            DrawUIPanel(RenderContext, X, Y, Width, Height, Bar->ToastAccent);
        }
        UIText(RenderContext, Strong, X + 0.5f * Width, Y + 8.f, Bar->Toast,
               WithAlpha(UI_COLOR_TEXT, Alpha), UIAlign_Center);
        (void)Body;
    }

    // NOTE(zoubir): the talent panel says the same, and the banner would
    // sit behind it
    bool32 PanelOpen = AppState->TalentRequests && AppState->TalentRequests->PanelOpen;
    if (Bar->BannerAge < LEVEL_BANNER_SECONDS && Bar->BannerLevel && !PanelOpen)
    {
        float T = Bar->BannerAge / LEVEL_BANNER_SECONDS;
        float In = Clamp01(Bar->BannerAge / 0.25f);
        float Out = Clamp01((LEVEL_BANNER_SECONDS - Bar->BannerAge) / 0.5f);
        float Alpha = In * Out;
        float Width = 420.f;
        float Height = 92.f;
        float X = 0.5f * ((float)WindowWidth - Width);
        float Y = 0.16f * (float)WindowHeight - 10.f * (1.f - In);
        // NOTE(zoubir): a band of light behind it, wider than the panel
        DrawShaderQuad(RenderContext, Shader_Glow, X - 120.f, Y - 60.f, Width + 240.f,
                       Height + 120.f, WithAlpha(XP_COLOR, 0.35f * Alpha),
                       RenderBlend_Additive);
        if (Alpha > 0.05f)
        {
            DrawUIPanel(RenderContext, X, Y, Width, Height, XP_COLOR);
        }
        float Ring = 80.f + 260.f * T;
        DrawShaderQuad(RenderContext, Shader_Ring, 0.5f * (float)WindowWidth - 0.5f * Ring,
                       Y + 0.5f * Height - 0.5f * Ring, Ring, Ring,
                       WithAlpha(XP_COLOR, 0.6f * (1.f - T)), RenderBlend_Additive);
        font *Title = AppState->Fonts.Title ? AppState->Fonts.Title : Strong;
        char Text[48];
        snprintf(Text, sizeof(Text), "LEVEL %u", Bar->BannerLevel);
        UIText(RenderContext, Title, 0.5f * (float)WindowWidth, Y + 12.f, Text,
               WithAlpha(UI_RGBA(255, 232, 160, 255), Alpha), UIAlign_Center);
        u32 Points = TalentPointsLeft(Slot);
        Text[0] = 0;
        if (Points)
        {
            snprintf(Text, sizeof(Text), "%u talent point%s ready, press N", Points,
                     Points == 1 ? "" : "s");
        }
        UIText(RenderContext, AppState->Fonts.Body, 0.5f * (float)WindowWidth,
               Y + Height - UILineHeight(AppState->Fonts.Body) - 12.f, Text,
               WithAlpha(UI_COLOR_TEXT, Alpha), UIAlign_Center);
    }
}
