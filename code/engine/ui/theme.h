/* The UI theme: every colour, gap and row height the widgets and screens
   use. Change the look here, not in the screens. Colours are u32 in the
   order the renderer reads (0xAABBGGRR); UI_RGBA takes plain r, g, b, a. */

#define UI_RGBA(R, G, B, A) \
    (((u32)(A) << 24) | ((u32)(B) << 16) | ((u32)(G) << 8) | (u32)(R))

// Surfaces, darkest first
#define UI_COLOR_FIELD          UI_RGBA( 12,  14,  18, 255)
#define UI_COLOR_PANEL          UI_RGBA( 20,  23,  30, 232)
#define UI_COLOR_CONTROL        UI_RGBA( 44,  49,  62, 255)
#define UI_COLOR_CONTROL_HOT    UI_RGBA( 62,  69,  86, 255)
#define UI_COLOR_BORDER         UI_RGBA( 84,  92, 110, 255)

// Text
#define UI_COLOR_TEXT           UI_RGBA(238, 238, 232, 255)
#define UI_COLOR_TEXT_MUTED     UI_RGBA(156, 162, 176, 255)
#define UI_COLOR_TEXT_SHADOW    UI_RGBA(  0,   0,   0, 170)

// Meaning: the accent marks focus and the local player; the rest are
// for state the player has to notice
#define UI_COLOR_ACCENT         UI_RGBA(240, 200,  48, 255)
#define UI_COLOR_HEALTH         UI_RGBA(208,  64,  48, 255)
#define UI_COLOR_GOOD           UI_RGBA( 92, 196, 120, 255)
#define UI_COLOR_DIM            UI_RGBA(128, 128, 128, 255)
#define UI_COLOR_TRACK          UI_RGBA( 32,  32,  32, 192)

// Spacing and sizes, in pixels
#define UI_GAP_SMALL 6.f
#define UI_GAP 12.f
#define UI_GAP_LARGE 20.f
#define UI_PADDING 6.f
// NOTE(zoubir): tall enough that descenders (g, y) in the body font are
// not clipped inside an edit box or button
#define UI_ROW_HEIGHT 40.f

// NOTE(zoubir): Color with its alpha replaced by Alpha (0..1); shaders
// that read the alpha as a parameter take it this way too
inline u32
WithAlpha(u32 Color, float Alpha)
{
    float Clamped = Alpha < 0.f ? 0.f : (Alpha > 1.f ? 1.f : Alpha);
    u32 Result = (Color & 0x00FFFFFF) | ((u32)(Clamped * 255.f + 0.5f) << 24);
    return Result;
}
