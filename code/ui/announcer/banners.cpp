/* Announcer banners (announcer.cpp): how a card and the toasts look.

   A title card slides black film bars in at the top and bottom, darkens
   a band across the middle, and lands its title there: a small spaced
   kicker above ("DUNGEON  -  LEVEL 1"), the title in the big display
   face, punched in from a little larger, a line of light that grows out
   from under it, and the detail line below. Above it all its icon
   (icons/announce_icons.cpp) lands first with a ring of light. A callout
   is the same without the bars and the band, higher on the screen and
   shorter. Toasts are short lines under the kill feed starting with their
   small icon on a glow of their colour, newest on top, fading out. */

#define ANNOUNCE_DISPLAY_SIZE 60.f
#define ANNOUNCE_BAR_SHARE 0.085f
#define ANNOUNCE_FADE_IN 0.18f
#define ANNOUNCE_FADE_OUT 0.45f
#define ANNOUNCE_PUNCH_SECONDS 0.22f
#define ANNOUNCE_PUNCH_SCALE 0.35f
#define ANNOUNCE_TOAST_SECONDS 6.f
// NOTE(zoubir): the icon above a title card's title, and a callout's
#define ANNOUNCE_CARD_ICON_SIZE 72.f
#define ANNOUNCE_CALLOUT_ICON_SIZE 56.f
#define ANNOUNCE_TOAST_FADE 0.8f

internal font *
GetDisplayFont(app_state *AppState, announcer *Announcer)
{
    if (!Announcer->DisplayTried && AppState->OpenGL)
    {
        Announcer->DisplayTried = true;
        char *Paths[] =
            {
                "fonts/AtkinsonHyperlegible-Bold.ttf",
                "c:/windows/fonts/segoeuib.ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            };
        for(u32 Index = 0; !Announcer->Display && Index < ArrayCount(Paths); Index++)
        {
            Announcer->Display = CreateFont(AppState->OpenGL, &AppState->MemoryArena,
                                            ANNOUNCE_DISPLAY_SIZE, 1024, 1024, Paths[Index]);
        }
    }
    font *Result = Announcer->Display ? Announcer->Display : AppState->Fonts.Title;
    return Result;
}

// NOTE(zoubir): one line centred on CenterX, Scale times its size, kept
// centred on its own middle as it scales, with a shadow
internal void
AnnounceText(render_context *RenderContext, font *Font, float CenterX, float TopY,
             char *Text, u32 Color, float Scale)
{
    if (!Font || !Text[0])
    {
        return;
    }
    float Height = UILineHeight(Font);
    float Width = GetTextWidth(Font, Text) * Scale;
    float X = CenterX - 0.5f * Width;
    float Top = TopY - 0.5f * (Scale - 1.f) * Height;
    float Baseline = Top + Font->UpperLimit * Scale;
    v4 NoClip = {0.f, 0.f, 100000.f, 100000.f};
    float Alpha = (float)(Color >> 24) / 255.f;
    RenderText(RenderContext, X + 2.f, Baseline + 3.f, Font, RenderContext->TextureProgram,
               Text, WithAlpha(UI_RGBA(0, 0, 0, 255), 0.6f * Alpha), Scale, Scale, NoClip, 0.f);
    RenderText(RenderContext, X, Baseline, Font, RenderContext->TextureProgram, Text, Color,
               Scale, Scale, NoClip, 0.f);
}

// NOTE(zoubir): "D U N G E O N": letters spread apart, in capitals, centred
internal void
SpacedText(render_context *RenderContext, font *Font, float CenterX, float TopY,
           char *Text, u32 Color, float Spacing)
{
    if (!Font || !Text[0])
    {
        return;
    }
    char Upper[64];
    u32 Length = 0;
    for(; Text[Length] && Length + 1 < sizeof(Upper); Length++)
    {
        char Letter = Text[Length];
        Upper[Length] = (Letter >= 'a' && Letter <= 'z') ? (char)(Letter - 'a' + 'A') : Letter;
    }
    Upper[Length] = 0;
    float Width = GetTextWidth(Font, Upper) + Spacing * (float)(Length - 1);
    float X = CenterX - 0.5f * Width;
    char One[2] = {0, 0};
    for(u32 Index = 0; Index < Length; Index++)
    {
        One[0] = Upper[Index];
        UIText(RenderContext, Font, X, TopY, One, Color);
        X += GetCharacterWidth(Font, One[0]) + Spacing;
    }
}

internal void
DrawAnnounceCard(render_context *RenderContext, app_state *AppState, announcer *Announcer,
                 u32 WindowWidth, u32 WindowHeight)
{
    if (!Announcer->Showing)
    {
        return;
    }
    announce_card *Card = &Announcer->Now;
    bool32 Title = Card->Style == AnnounceStyle_Title;
    float Width = (float)WindowWidth;
    float Height = (float)WindowHeight;
    float Age = Card->Age;
    float In = SmoothStep01(Age / ANNOUNCE_FADE_IN);
    float Out = SmoothStep01((Card->Seconds - Age) / ANNOUNCE_FADE_OUT);
    float Alpha = Minimum(In, Out);

    font *Display = GetDisplayFont(AppState, Announcer);
    font *Small = AppState->Fonts.Small;
    font *Body = AppState->Fonts.Body;
    float KickerHeight = Card->Kicker[0] ? UILineHeight(Small) + UI_GAP_SMALL : 0.f;
    float DetailHeight = Card->Detail[0] ? UILineHeight(Body) + UI_GAP + 4.f : 0.f;
    float IconSize = Title ? ANNOUNCE_CARD_ICON_SIZE : ANNOUNCE_CALLOUT_ICON_SIZE;
    float IconHeight = Card->Icon ? IconSize + UI_GAP_SMALL : 0.f;
    float Block = IconHeight + KickerHeight + UILineHeight(Display) + DetailHeight;
    float CenterX = 0.5f * Width;

    // NOTE(zoubir): a callout sits above the player; while the local
    // player is dead the death plate has that spot, so it goes higher
    player_slot *Local = &AppState->Players[AppState->LocalPlayerIndex];
    bool32 Dead = Local->Active && Local->Entity && IsDeadPlayer(Local->Entity);
    float Top = Title ? 0.5f * Height - 0.5f * Block - 0.06f * Height :
        (Dead ? 0.14f : 0.24f) * Height;

    if (Title)
    {
        float Bar = ANNOUNCE_BAR_SHARE * Height * Minimum(SmoothStep01(Age / 0.35f), Out);
        u32 Black = UI_RGBA(0, 0, 0, 255);
        DrawFilledRectangle(RenderContext, 0.f, 0.f, Width, Bar, Black, 0.f);
        DrawFilledRectangle(RenderContext, 0.f, Height - Bar, Width, Bar, Black, 0.f);
        float BandPad = UI_GAP_LARGE;
        DrawShaderQuad(RenderContext, Shader_Glow, -0.25f * Width, Top - 3.f * BandPad,
                       1.5f * Width, Block + 6.f * BandPad,
                       WithAlpha(UI_RGBA(0, 0, 0, 255), 0.75f * Alpha));
    }
    // NOTE(zoubir): light behind the title in its colour
    float GlowWidth = Minimum(Width, GetTextWidth(Display, Card->Title) + 260.f);
    DrawShaderQuad(RenderContext, Shader_Glow, CenterX - 0.5f * GlowWidth,
                   Top + IconHeight + KickerHeight - 50.f, GlowWidth,
                   UILineHeight(Display) + 100.f,
                   WithAlpha(Card->Color, (Title ? 0.22f : 0.3f) * Alpha),
                   RenderBlend_Additive);

    float Y = Top;
    float Punch = 1.f + ANNOUNCE_PUNCH_SCALE * (1.f - SmoothStep01(Age / ANNOUNCE_PUNCH_SECONDS));
    if (Card->Icon)
    {
        // NOTE(zoubir): the icon lands first, a ring of its light
        // breaking out from it as it does
        v2 IconCenter = V2(CenterX, Y + 0.5f * IconSize);
        float Ring = IconSize * (0.8f + 1.6f * SmoothStep01(Age / 0.5f));
        float RingAlpha = Alpha * (1.f - SmoothStep01(Age / 0.5f));
        DrawShaderQuad(RenderContext, Shader_Glow, IconCenter.X - IconSize, IconCenter.Y - IconSize,
                       2.f * IconSize, 2.f * IconSize, WithAlpha(Card->Color, 0.35f * Alpha),
                       RenderBlend_Additive);
        DrawShaderQuad(RenderContext, Shader_Ring, IconCenter.X - 0.5f * Ring,
                       IconCenter.Y - 0.5f * Ring, Ring, Ring,
                       WithAlpha(Card->Color, 0.8f * RingAlpha), RenderBlend_Additive);
        DrawAnnounceIcon(RenderContext, &Announcer->IconAtlas, Card->Icon, IconCenter,
                         IconSize * Punch, Alpha);
        Y += IconHeight;
    }
    if (Card->Kicker[0])
    {
        SpacedText(RenderContext, Small, CenterX, Y, Card->Kicker,
                   WithAlpha(UIMixColor(Card->Color, UI_COLOR_TEXT, 0.35f), Alpha), 3.f);
        Y += KickerHeight;
    }
    // NOTE(zoubir): a white flash as it lands, settling into its colour
    u32 TitleColor = UIMixColor(UI_RGBA(255, 255, 255, 255), Card->Color,
                                Clamp01(Age / 0.3f) * 0.85f);
    AnnounceText(RenderContext, Display, CenterX, Y, Card->Title,
                 WithAlpha(TitleColor, Alpha), Punch);
    Y += UILineHeight(Display);

    // NOTE(zoubir): the line of light under the title grows from the middle
    float LineWidth = Minimum(0.6f * Width, GetTextWidth(Display, Card->Title) + 120.f) *
        SmoothStep01((Age - 0.08f) / 0.5f);
    if (LineWidth > 1.f)
    {
        DrawFilledRectangle(RenderContext, CenterX - 0.5f * LineWidth, Y + 2.f, LineWidth, 2.f,
                            WithAlpha(Card->Color, 0.8f * Alpha), 0.f);
        DrawShaderQuad(RenderContext, Shader_Glow, CenterX - 0.5f * LineWidth, Y - 8.f,
                       LineWidth, 20.f, WithAlpha(Card->Color, 0.5f * Alpha),
                       RenderBlend_Additive);
    }
    if (Card->Detail[0])
    {
        float DetailAlpha = Alpha * SmoothStep01((Age - 0.2f) / 0.3f);
        UIText(RenderContext, Body, CenterX, Y + UI_GAP + 4.f, Card->Detail,
               WithAlpha(UI_COLOR_TEXT, DetailAlpha), UIAlign_Center);
    }
}

// NOTE(zoubir): under the kill feed, right-aligned like it
internal void
DrawAnnounceToasts(render_context *RenderContext, app_state *AppState, announcer *Announcer,
                   u32 WindowWidth)
{
    font *Font = AppState->Fonts.Small;
    if (!Font)
    {
        return;
    }
    u32 FeedLines = AppState->KillFeed ? AppState->KillFeed->Count : 0;
    float Line = UILineHeight(Font) + 2.f;
    float Y = KILL_FEED_TOP + (float)FeedLines * Line + (FeedLines ? UI_GAP : 0.f);
    float Right = (float)WindowWidth - UI_GAP_LARGE;
    for(u32 Index = 0; Index < Announcer->ToastCount; Index++)
    {
        announce_toast *Toast = &Announcer->Toasts[Index];
        if (Toast->Age >= ANNOUNCE_TOAST_SECONDS)
        {
            Announcer->ToastCount = Index;
            break;
        }
        float Alpha = Minimum(Clamp01(Toast->Age / 0.2f),
                              Clamp01((ANNOUNCE_TOAST_SECONDS - Toast->Age) / ANNOUNCE_TOAST_FADE));
        // NOTE(zoubir): slides in from the right as it appears
        float Slide = 24.f * (1.f - SmoothStep01(Toast->Age / 0.25f));
        float TextWidth = UITextWidth(Font, Toast->Text);
        float IconSize = Line + 4.f;
        float PlateWidth = TextWidth + 2.f * UI_GAP_SMALL + IconSize + 4.f;
        float PlateX = Right - PlateWidth + Slide;
        DrawRoundRect(RenderContext, PlateX, Y - 3.f, PlateWidth, Line + 4.f,
                      WithAlpha(UI_COLOR_PANEL, 0.85f * Alpha));
        // NOTE(zoubir): the toast's colour as a soft light behind its icon
        v2 IconCenter = V2(PlateX + 4.f + 0.5f * IconSize, Y - 1.f + 0.5f * Line);
        DrawShaderQuad(RenderContext, Shader_Glow, IconCenter.X - IconSize, IconCenter.Y - IconSize,
                       2.f * IconSize, 2.f * IconSize, WithAlpha(Toast->Color, 0.3f * Alpha),
                       RenderBlend_Additive);
        DrawAnnounceIcon(RenderContext, &Announcer->IconAtlas, Toast->Icon, IconCenter, IconSize,
                         Alpha);
        UIText(RenderContext, Font, Right - UI_GAP_SMALL + Slide, Y, Toast->Text,
               WithAlpha(UI_COLOR_TEXT, Alpha), UIAlign_Right);
        Y += Line + 7.f;
    }
}
