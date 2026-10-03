/* Screen text for the HUD and screens: one line at a point, aligned on
   X, with a one-pixel shadow so it reads over grass and sand. Y is the
   top of the line (RenderText takes the baseline). Every call is a no-op
   when Font is 0, so callers need no font check. */

enum ui_align
{
    UIAlign_Left,
    UIAlign_Center,
    UIAlign_Right,
};

inline float
UITextWidth(font *Font, char *Text)
{
    float Result = Font ? GetTextWidth(Font, Text) : 0.f;
    return Result;
}

inline float
UILineHeight(font *Font)
{
    float Result = Font ? Font->UpperLimit + Font->LowerLimit : 0.f;
    return Result;
}

internal void
UIText(render_context *RenderContext, font *Font, float X, float TopY,
       char *Text, u32 Color, ui_align Align = UIAlign_Left)
{
    if (!Font)
    {
        return;
    }
    if (Align != UIAlign_Left)
    {
        float Width = GetTextWidth(Font, Text);
        X -= (Align == UIAlign_Center) ? 0.5f * Width : Width;
    }
    v4 NoClip = {0.f, 0.f, 100000.f, 100000.f};
    float BaselineY = TopY + Font->UpperLimit;
    RenderText(RenderContext, X + 1.f, BaselineY + 1.f, Font,
               RenderContext->TextureProgram, Text, UI_COLOR_TEXT_SHADOW,
               1.f, 1.f, NoClip, 0.f);
    RenderText(RenderContext, X, BaselineY, Font,
               RenderContext->TextureProgram, Text, Color,
               1.f, 1.f, NoClip, 0.f);
}
