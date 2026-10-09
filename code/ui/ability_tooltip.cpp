/* Ability tooltip (ability_bar.cpp): the card over a hovered slot. A
   header with the ability's icon, its name in the slot's colour, what
   kind it is and its key; what it does, wrapped to the card; its numbers
   one to a line, the figure in a colour for what it is (damage, healing,
   shields, control); what the next level gives when a point can go in;
   then under a rule its cooldown and whether it is ready on the left and
   what a cast costs on the right, red when the player is short; the rule
   fills as the ability recharges. What the
   card says is ability_tooltip/tip_card.cpp. The card fades in after a
   moment over a slot, so sweeping the mouse across the bar does not
   flash cards, and moves straight to the next slot once it shows. */

#include "ability_tooltip/spell_numbers.cpp"
#include "ability_tooltip/tip_card.cpp"

#define ABILITY_TIP_WIDTH 300.f
#define ABILITY_TIP_MOST_WIDTH 380.f
#define ABILITY_TIP_PAD 14.f
#define ABILITY_TIP_ICON 40.f
#define ABILITY_TIP_ROWS 4 // the description wraps to at most this many rows
// NOTE(zoubir): in a dungeon the class meters (ui/dungeon/classes/) sit
// just over the bar; the card stays above them
#define ABILITY_TIP_LIFT 10.f
#define ABILITY_TIP_DUNGEON_LIFT 64.f
// NOTE(zoubir): over a slot this long before the card starts to show,
// then this long to fade in
#define ABILITY_TIP_DELAY 0.06f
#define ABILITY_TIP_FADE 0.12f

// NOTE(zoubir): what the ability bar saw hovered this frame, drawn late
// (DrawAbilityTip in ability_bar.cpp)
struct ability_tip
{
    i32 Slot;      // index into AbilitySlotDefs, -1 for none
    i32 LastSlot;  // last frame's, to tell a new hover from a held one
    float Seconds; // since the mouse came onto the bar's slots
    float X;       // the slot's centre
    float Bottom;  // the bar's plate top
    float Full;    // the whole cooldown as it stands, 0 for none
    float Left;    // seconds still to run
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

// NOTE(zoubir): Color with its own alpha times the card's fade
inline u32
TipFade(u32 Color, float Fade)
{
    u32 Result = WithAlpha(Color, ((float)(Color >> 24) / 255.f) * Fade);
    return Result;
}

internal void
DrawAbilityTooltip(render_context *RenderContext, app_state *AppState, world_entity *Player,
                   ability_slot_def *Def, u32 Atlas, ability_tip *Tip, u32 WindowWidth)
{
    font *Body = AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : Body;
    float Fade = Maximum(0.f, Minimum(1.f, (Tip->Seconds - ABILITY_TIP_DELAY) / ABILITY_TIP_FADE));
    if (!Body || Fade <= 0.f)
    {
        return;
    }
    ability_tip_card Card = {};
    BuildAbilityTipCard(&Card, AppState, Player, Def, Tip->Full, Tip->Left);
    char *Key = ActionKeyLabel(Def->Button);
    // NOTE(zoubir): "Ready" only when a press would cast it now; short of
    // its cost the red cost says why
    bool32 Short = Card.CostColor == TIP_COLOR_BAD;
    char *Status = (char *)(Card.Left[0] ? Card.Left :
                            ((Card.HasCooldown && !Short) ? "Ready" : ""));

    // NOTE(zoubir): wide enough for the header, the numbers and the
    // footer, the description wraps to what is left
    float KeyWidth = UITextWidth(Small, Key) + 10.f;
    float HeadText = ABILITY_TIP_ICON + 10.f;
    float Width = ABILITY_TIP_WIDTH - 2.f * ABILITY_TIP_PAD;
    Width = Maximum(Width, HeadText + UITextWidth(Strong, Card.Name) + KeyWidth + 12.f);
    Width = Maximum(Width, HeadText + UITextWidth(Small, Card.Kind));
    Width = Maximum(Width, UITextWidth(Small, Card.Cooldown) + UITextWidth(Small, Status) +
                    UITextWidth(Small, Card.Cost) + 30.f);
    for(u32 Index = 0; Index < Card.StatCount; Index++)
    {
        Width = Maximum(Width, UITextWidth(Strong, Card.Stat[Index].Value) + 6.f +
                        UITextWidth(Body, Card.Stat[Index].Label));
    }
    Width = Maximum(Width, UITextWidth(Small, Card.Next));
    float Inner = Minimum(Width, ABILITY_TIP_MOST_WIDTH - 2.f * ABILITY_TIP_PAD);
    Width = Inner + 2.f * ABILITY_TIP_PAD;

    u32 Starts[ABILITY_TIP_ROWS];
    u32 Ends[ABILITY_TIP_ROWS];
    u32 Rows = (Card.Description && Card.Description[0]) ?
        WrapAbilityTipText(Body, Card.Description, Inner, Starts, Ends) : 0;

    float Gap = 4.f;
    float StatHeight = Maximum(UILineHeight(Strong), UILineHeight(Body));
    float HeadHeight = Maximum(ABILITY_TIP_ICON, UILineHeight(Strong) + UILineHeight(Small));
    float Height = 2.f * ABILITY_TIP_PAD + HeadHeight;
    if (Rows) Height += 3.f * Gap + (float)Rows * UILineHeight(Body);
    if (Card.StatCount) Height += 2.f * Gap + (float)Card.StatCount * StatHeight;
    if (Card.Next[0]) Height += 2.f * Gap + UILineHeight(Small);
    if (Card.Hint[0]) Height += UILineHeight(Small);
    Height += 3.f * Gap + 1.f + UILineHeight(Small);

    // NOTE(zoubir): it rises a few pixels as it fades in
    float X = Maximum(8.f, Minimum(Tip->X - 0.5f * Width, (float)WindowWidth - Width - 8.f));
    float Y = Tip->Bottom - Height - 6.f * (1.f - Fade) -
        (IsDungeon(AppState) ? ABILITY_TIP_DUNGEON_LIFT : ABILITY_TIP_LIFT);
    u32 Accent = Def->Accent;
    // NOTE(zoubir): a dark sheet under the glass, the slot's colour down
    // the left edge, as the talent panel's cards
    DrawFilledRectangle(RenderContext, X + 4.f, Y + 4.f, Width - 8.f, Height - 8.f,
                        TipFade(UI_RGBA(6, 7, 11, 242), Fade), 0.f);
    DrawUIPanel(RenderContext, X, Y, Width, Height, Accent, Fade);
    DrawFilledRectangle(RenderContext, X + 6.f, Y + 10.f, 3.f, Height - 20.f,
                        TipFade(WithAlpha(Accent, 0.9f), Fade), 0.f);

    float TextX = X + ABILITY_TIP_PAD;
    float Right = X + Width - ABILITY_TIP_PAD;
    float LineY = Y + ABILITY_TIP_PAD;

    // NOTE(zoubir): the header: the slot's icon on a dark tile, name and
    // kind beside it, the key on a tab at the right
    DrawRoundRect(RenderContext, TextX, LineY, ABILITY_TIP_ICON, ABILITY_TIP_ICON,
                  TipFade(UI_RGBA(12, 13, 20, 240), Fade));
    DrawRoundOutline(RenderContext, TextX, LineY, ABILITY_TIP_ICON, ABILITY_TIP_ICON,
                     TipFade(WithAlpha(Accent, 0.8f), Fade));
    float Icon = ABILITY_TIP_ICON - 6.f;
    DrawTexturedQuad(RenderContext, Atlas, TextX + 3.f, LineY + 3.f, Icon, Icon,
                     AbilityIconUvs(Card.IconCell), TipFade(0xFFFFFFFF, Fade));
    float NameX = TextX + HeadText;
    float NameY = LineY + 0.5f * (HeadHeight - UILineHeight(Strong) - UILineHeight(Small));
    UIText(RenderContext, Strong, NameX, NameY, Card.Name, TipFade(Accent, Fade));
    UIText(RenderContext, Small, NameX, NameY + UILineHeight(Strong), Card.Kind,
           TipFade(UI_COLOR_TEXT_MUTED, Fade));
    float KeyHeight = UILineHeight(Small) + 2.f;
    float KeyX = Right - KeyWidth;
    float KeyY = NameY + 0.5f * (UILineHeight(Strong) - KeyHeight);
    DrawRoundRect(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                  TipFade(UI_RGBA(12, 13, 20, 235), Fade));
    DrawRoundOutline(RenderContext, KeyX, KeyY, KeyWidth, KeyHeight,
                     TipFade(WithAlpha(Accent, 0.7f), Fade));
    UIText(RenderContext, Small, KeyX + 5.f, KeyY + 1.f, Key, TipFade(UI_COLOR_TEXT, Fade));
    LineY += HeadHeight;

    if (Rows)
    {
        LineY += 3.f * Gap;
        for(u32 Row = 0; Row < Rows; Row++)
        {
            char Text[160];
            u32 Length = Minimum(Ends[Row] - Starts[Row], (u32)sizeof(Text) - 1);
            memcpy(Text, Card.Description + Starts[Row], Length);
            Text[Length] = 0;
            UIText(RenderContext, Body, TextX, LineY, Text, TipFade(UI_COLOR_TEXT, Fade));
            LineY += UILineHeight(Body);
        }
    }
    if (Card.StatCount)
    {
        LineY += 2.f * Gap;
        for(u32 Index = 0; Index < Card.StatCount; Index++)
        {
            ability_tip_stat *Stat = &Card.Stat[Index];
            float ValueWidth = UITextWidth(Strong, Stat->Value);
            UIText(RenderContext, Strong, TextX, LineY, Stat->Value, TipFade(Stat->Color, Fade));
            // NOTE(zoubir): the label sits on the figure's baseline
            float LabelY = LineY + UILineHeight(Strong) - UILineHeight(Body);
            UIText(RenderContext, Body, TextX + ValueWidth + (ValueWidth > 0.f ? 6.f : 0.f),
                   LabelY, Stat->Label, TipFade(UI_RGBA(176, 182, 196, 255), Fade));
            LineY += StatHeight;
        }
    }
    if (Card.Next[0])
    {
        LineY += 2.f * Gap;
        UIText(RenderContext, Small, TextX, LineY, Card.Next, TipFade(TIP_COLOR_GOOD, Fade));
        LineY += UILineHeight(Small);
    }
    if (Card.Hint[0])
    {
        UIText(RenderContext, Small, TextX, LineY, Card.Hint, TipFade(XP_COLOR, Fade));
        LineY += UILineHeight(Small);
    }

    // NOTE(zoubir): a thin rule, then cooldown and readiness on the left,
    // cost on the right
    LineY += 2.f * Gap;
    DrawFilledRectangle(RenderContext, TextX, LineY, Inner, 1.f,
                        TipFade(UI_RGBA(255, 255, 255, 34), Fade), 0.f);
    // NOTE(zoubir): while it recharges the rule fills from the left in the
    // slot's colour, as far as it has come back
    if (Tip->Left > 0.f && Tip->Full > 0.f)
    {
        float Back = Maximum(0.f, Minimum(1.f, 1.f - Tip->Left / Tip->Full));
        DrawFilledRectangle(RenderContext, TextX, LineY - 1.f, Inner * Back, 3.f,
                            TipFade(WithAlpha(Accent, 0.85f), Fade), 0.f);
    }
    LineY += 1.f + Gap;
    UIText(RenderContext, Small, TextX, LineY, Card.Cooldown, TipFade(UI_COLOR_TEXT, Fade));
    if (Status[0])
    {
        UIText(RenderContext, Small, TextX + UITextWidth(Small, Card.Cooldown) + 10.f, LineY,
               Status, TipFade(Card.Left[0] ? TIP_COLOR_BAD : TIP_COLOR_GOOD, Fade));
    }
    UIText(RenderContext, Small, Right, LineY, Card.Cost, TipFade(Card.CostColor, Fade),
           UIAlign_Right);
}
