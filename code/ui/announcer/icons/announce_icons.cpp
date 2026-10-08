/* Announcer icons (announcer.cpp): one picture per kind of news, painted
   in code with the ability bar's icon canvas (ui/ability_icons/
   icon_canvas.cpp) into one texture the first time a card or toast shows.
   Cards draw theirs large above the title; toasts small at the start of
   their line. The painters are by theme: combat_icons.cpp, glory_icons.cpp
   and people_icons.cpp.

   A new icon: its painter in one of those files, a name in announce_icon
   and its painter at the same place in AnnounceIconPainters. */

#include "combat_icons.cpp"
#include "glory_icons.cpp"
#include "people_icons.cpp"

enum announce_icon
{
    AnnounceIcon_None,
    AnnounceIcon_Swords,
    AnnounceIcon_Skull,
    AnnounceIcon_Boss,
    AnnounceIcon_Blood,
    AnnounceIcon_Flame,
    AnnounceIcon_Target,
    AnnounceIcon_Grave,
    AnnounceIcon_Crown,
    AnnounceIcon_Trophy,
    AnnounceIcon_Check,
    AnnounceIcon_Cross,
    AnnounceIcon_Gate,
    AnnounceIcon_Hourglass,
    AnnounceIcon_Joined,
    AnnounceIcon_Left,
    AnnounceIcon_Heart,
    AnnounceIcon_HeartBroken,
    AnnounceIcon_Ballot,
    AnnounceIcon_Signal,
    AnnounceIcon_SignalLost,
    AnnounceIcon_Count
};

global_variable ability_icon_painter *AnnounceIconPainters[AnnounceIcon_Count] =
{
    0,
    PaintAnnounceSwords,
    PaintAnnounceSkull,
    PaintAnnounceBoss,
    PaintAnnounceBlood,
    PaintAnnounceFlame,
    PaintAnnounceTarget,
    PaintAnnounceGrave,
    PaintAnnounceCrown,
    PaintAnnounceTrophy,
    PaintAnnounceCheck,
    PaintAnnounceCross,
    PaintAnnounceGate,
    PaintAnnounceHourglass,
    PaintAnnounceJoined,
    PaintAnnounceLeft,
    PaintAnnounceHeart,
    PaintAnnounceHeartBroken,
    PaintAnnounceBallot,
    PaintAnnounceSignal,
    PaintAnnounceSignalLost,
};

// NOTE(zoubir): pixels per icon; cards draw them at about 64, toasts at
// about 20, so they are always scaled down and stay crisp
#define ANNOUNCE_ICON_SIZE 96
#define ANNOUNCE_ICON_COLUMNS 6

inline u32
AnnounceIconRows()
{
    u32 Result = (AnnounceIcon_Count + ANNOUNCE_ICON_COLUMNS - 1) / ANNOUNCE_ICON_COLUMNS;
    return Result;
}

internal u32
BuildAnnounceIconAtlas(open_gl *OpenGL, memory_arena *Scratch)
{
    u32 Width = ANNOUNCE_ICON_COLUMNS * ANNOUNCE_ICON_SIZE;
    u32 Height = AnnounceIconRows() * ANNOUNCE_ICON_SIZE;
    temporary_memory Temp = BeginTemporaryMemory(Scratch);
    u32 *Pixels = AllocateArray(Scratch, Width * Height, u32);
    memset(Pixels, 0, Width * Height * sizeof(u32));
    icon_canvas Canvas = {};
    Canvas.Size = ANNOUNCE_ICON_SIZE;
    Canvas.Pixels = AllocateArray(Scratch, ANNOUNCE_ICON_SIZE * ANNOUNCE_ICON_SIZE, v4);
    for(u32 Index = 1; Index < AnnounceIcon_Count; Index++)
    {
        memset(Canvas.Pixels, 0, ANNOUNCE_ICON_SIZE * ANNOUNCE_ICON_SIZE * sizeof(v4));
        AnnounceIconPainters[Index](&Canvas);
        u32 Column = Index % ANNOUNCE_ICON_COLUMNS;
        u32 Row = Index / ANNOUNCE_ICON_COLUMNS;
        IconFinish(&Canvas, Pixels + Row * ANNOUNCE_ICON_SIZE * Width + Column * ANNOUNCE_ICON_SIZE,
                   Width);
    }
    u32 Result = RenderUploadTexture(OpenGL, Width, Height, Pixels, true);
    EndTemporaryMemory(Temp);
    return Result;
}

// NOTE(zoubir): the cell's texture coordinates, as AbilityIconUvs gives
// them (left, bottom, right, top, V from the bottom)
inline v4
AnnounceIconUvs(u32 Icon)
{
    float Width = (float)(ANNOUNCE_ICON_COLUMNS * ANNOUNCE_ICON_SIZE);
    float Height = (float)(AnnounceIconRows() * ANNOUNCE_ICON_SIZE);
    float Left = (float)((Icon % ANNOUNCE_ICON_COLUMNS) * ANNOUNCE_ICON_SIZE) + 0.5f;
    float Top = (float)((Icon / ANNOUNCE_ICON_COLUMNS) * ANNOUNCE_ICON_SIZE) + 0.5f;
    float Size = (float)ANNOUNCE_ICON_SIZE - 1.f;
    v4 Result = V4(Left / Width, 1.f - (Top + Size) / Height, (Left + Size) / Width,
                   1.f - Top / Height);
    return Result;
}

// NOTE(zoubir): Icon centred on Center, Size pixels across, faded by
// Alpha; the texture is painted the first time
internal void
DrawAnnounceIcon(render_context *RenderContext, u32 *Atlas, u32 Icon, v2 Center, float Size,
                 float Alpha)
{
    if (Icon == AnnounceIcon_None || Icon >= AnnounceIcon_Count || Alpha <= 0.f)
    {
        return;
    }
    if (!*Atlas)
    {
        *Atlas = BuildAnnounceIconAtlas(RenderContext->OpenGL, RenderContext->Arena);
    }
    DrawTexturedQuad(RenderContext, *Atlas, Center.X - 0.5f * Size, Center.Y - 0.5f * Size,
                     Size, Size, AnnounceIconUvs(Icon),
                     WithAlpha(UI_RGBA(255, 255, 255, 255), Alpha));
}
