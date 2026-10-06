/* Panels and rounded parts. DrawUIPanel is the rounded glass plate behind
   HUD groups, tooltips and screens (build/shaders/fx/panel.frag), with a
   soft shadow under it; Tint colours its border and Opacity fades it.
   DrawRoundRect fills a small raised rounded part, such as a bar, button,
   field or chip (round_rect.frag), and DrawRoundOutline draws just the
   rim of one, for focus (round_outline.frag). The shaders measure the
   quad themselves, so any size keeps round corners. */

// NOTE(zoubir): the quad is this much bigger than the panel on every
// side, room for panel.frag's shadow
#define UI_PANEL_SHADOW 16.f

internal void
DrawUIPanel(render_context *RenderContext, float X, float Y, float Width, float Height,
            u32 Tint = UI_RGBA(150, 160, 190, 255), float Opacity = 1.f)
{
    // NOTE(zoubir): UV runs 0..1 over the panel itself and past it over
    // the shadow margin, so the shader finds the panel's edges in any
    // pixel scale
    float Margin = UI_PANEL_SHADOW;
    float MarginU = Width > 0.f ? Margin / Width : 0.f;
    float MarginV = Height > 0.f ? Margin / Height : 0.f;
    BeginBatch(RenderContext, 0, 0.f, RenderContext->Programs[Shader_Panel]);
    RenderContext->AllocatedBatches[RenderContext->BatchCount].Blend = RenderBlend_Alpha;
    RenderQuadTexture(RenderContext, X - Margin, Y - Margin,
                      Width + 2.f * Margin, Height + 2.f * Margin,
                      V4(-MarginU, 1.f + MarginV, 1.f + MarginU, -MarginV),
                      WithAlpha(Tint, Opacity), 0.f);
    EndBatch(RenderContext);
}

internal void
DrawRoundRect(render_context *RenderContext, float X, float Y, float Width,
              float Height, u32 Color)
{
    DrawShaderQuad(RenderContext, Shader_RoundRect, X, Y, Width, Height, Color);
}

internal void
DrawRoundOutline(render_context *RenderContext, float X, float Y, float Width,
                 float Height, u32 Color)
{
    DrawShaderQuad(RenderContext, Shader_RoundOutline, X, Y, Width, Height, Color);
}
