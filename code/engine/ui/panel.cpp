/* Panels: the rounded glass plate behind HUD groups, tooltips and
   screens, drawn by build/shaders/fx/panel.frag. Tint colours the border.
   The shader reads the panel's height over its width from the colour's
   alpha, divided by UI_PANEL_ASPECT_RANGE so panels up to that many times
   taller than wide keep round corners. */

#define UI_PANEL_ASPECT_RANGE 4.f

internal void
DrawUIPanel(render_context *RenderContext, float X, float Y, float Width, float Height,
            u32 Tint = UI_RGBA(150, 160, 190, 255))
{
    float Aspect = Width > 0.f ? Height / Width : 1.f;
    DrawShaderQuad(RenderContext, Shader_Panel, X, Y, Width, Height,
                   WithAlpha(Tint, Aspect / UI_PANEL_ASPECT_RANGE));
}
