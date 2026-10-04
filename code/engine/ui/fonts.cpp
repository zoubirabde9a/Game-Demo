/* UI fonts: the sizes each job uses and where the font files come from.
   The game ships Atkinson Hyperlegible in web/fonts/ (the folder the game
   runs from; licence in web/fonts/OFL.txt). System fonts are fallbacks
   for a checkout without that folder. */

#define UI_FONT_SIZE_SMALL 16.f
#define UI_FONT_SIZE_BODY 20.f
#define UI_FONT_SIZE_TITLE 34.f
#define UI_FONT_SIZE_STRONG 22.f

internal void
LoadUIFonts(font_set *Fonts, open_gl *OpenGL, memory_arena *Arena)
{
    char *Regular[] =
        {
            "fonts/AtkinsonHyperlegible-Regular.ttf",
            "c:/windows/fonts/segoeui.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        };
    char *Bold[] =
        {
            "fonts/AtkinsonHyperlegible-Bold.ttf",
            "c:/windows/fonts/segoeuib.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        };
    Fonts->Small = CreateFirstFont(OpenGL, Arena, UI_FONT_SIZE_SMALL,
                                   Regular, ArrayCount(Regular));
    Fonts->Body = CreateFirstFont(OpenGL, Arena, UI_FONT_SIZE_BODY,
                                  Regular, ArrayCount(Regular));
    Fonts->Title = CreateFirstFont(OpenGL, Arena, UI_FONT_SIZE_TITLE,
                                   Bold, ArrayCount(Bold));
    Fonts->Strong = CreateFirstFont(OpenGL, Arena, UI_FONT_SIZE_STRONG,
                                    Bold, ArrayCount(Bold));
}
