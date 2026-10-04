/* Shader errors: when a shader file fails to build (engine/shader_library.cpp
   keeps the last good one), the driver's message across the top of the
   screen, so a typo made with the game running is seen at once. */

internal void
DrawShaderErrors(render_context *RenderContext, app_state *AppState, u32 WindowWidth)
{
    char *Error = ShaderLibraryError();
    if (!Error)
    {
        return;
    }
    font *Font = AppState->Fonts.Small;
    float Height = UILineHeight(Font) + 8.f;
    DrawFilledRectangle(RenderContext, 0.f, 0.f, (float)WindowWidth, Height,
                        UI_RGBA(120, 20, 20, 230), 0.f);
    // NOTE(zoubir): the first line of the message only
    char Line[160];
    u32 Length = 0;
    while (Error[Length] && Error[Length] != '\n' && Length + 1 < sizeof(Line))
    {
        Line[Length] = Error[Length];
        Length++;
    }
    Line[Length] = 0;
    UIText(RenderContext, Font, 8.f, 4.f, Line, UI_COLOR_TEXT);
}
