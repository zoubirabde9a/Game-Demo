/* Ability icons: the slots of the ability bar (icon_list.inc) and their
   icons, painted in code by icon_canvas.cpp into one texture on first use.

   A new ability icon: a file here with its Paint function, an #include
   line below, and an ABILITY_ICON line in icon_list.inc. */

#include "icon_canvas.cpp"
#include "fireball.cpp"
#include "sword.cpp"
#include "jump.cpp"
#include "dash.cpp"
#include "shockwave.cpp"
#include "blink.cpp"
#include "push.cpp"
#include "launch.cpp"
#include "slam.cpp"
#include "rewind.cpp"
#include "shield.cpp"
#include "frost_nova.cpp"
#include "gravity_well.cpp"

// NOTE(zoubir): pixels per icon in the texture; the bar draws them at
// about 36, so they are always scaled down and stay crisp
#define ABILITY_ICON_SIZE 64
#define ABILITY_ATLAS_COLUMNS 4

typedef void ability_icon_painter(icon_canvas *Canvas);

struct ability_slot_def
{
    u32 Button;          // player_button
    char *Name;
    ability_icon_painter *Paint; // 0 for a gap between groups
    u32 Accent;          // UI_RGBA
};

#define ABILITY_ICON(Button, Name, Paint, R, G, B) {Button, Name, Paint, UI_RGBA(R, G, B, 255)},
#define ABILITY_GAP() {0, 0, 0, 0},
global_variable ability_slot_def AbilitySlotDefs[] =
{
#include "icon_list.inc"
};
#undef ABILITY_ICON
#undef ABILITY_GAP

#define ABILITY_SLOT_DEF_COUNT ArrayCount(AbilitySlotDefs)

inline u32
AbilityAtlasRows()
{
    u32 Result = (ABILITY_SLOT_DEF_COUNT + ABILITY_ATLAS_COLUMNS - 1) / ABILITY_ATLAS_COLUMNS;
    return Result;
}

// NOTE(zoubir): every icon painted into one texture, cell N for entry N
// of AbilitySlotDefs. Scratch is only used until the upload
internal u32
BuildAbilityIconAtlas(open_gl *OpenGL, memory_arena *Scratch)
{
    u32 Width = ABILITY_ATLAS_COLUMNS * ABILITY_ICON_SIZE;
    u32 Height = AbilityAtlasRows() * ABILITY_ICON_SIZE;
    temporary_memory Temp = BeginTemporaryMemory(Scratch);
    u32 *Pixels = AllocateArray(Scratch, Width * Height, u32);
    memset(Pixels, 0, Width * Height * sizeof(u32));
    icon_canvas Canvas = {};
    Canvas.Size = ABILITY_ICON_SIZE;
    Canvas.Pixels = AllocateArray(Scratch, ABILITY_ICON_SIZE * ABILITY_ICON_SIZE, v4);
    for(u32 Index = 0; Index < ABILITY_SLOT_DEF_COUNT; Index++)
    {
        ability_slot_def *Def = &AbilitySlotDefs[Index];
        if (!Def->Paint)
        {
            continue;
        }
        memset(Canvas.Pixels, 0, ABILITY_ICON_SIZE * ABILITY_ICON_SIZE * sizeof(v4));
        Def->Paint(&Canvas);
        u32 Column = Index % ABILITY_ATLAS_COLUMNS;
        u32 Row = Index / ABILITY_ATLAS_COLUMNS;
        IconFinish(&Canvas, Pixels + Row * ABILITY_ICON_SIZE * Width + Column * ABILITY_ICON_SIZE,
                   Width);
    }
    u32 Result = RenderUploadTexture(OpenGL, Width, Height, Pixels, true);
    EndTemporaryMemory(Temp);
    return Result;
}

// NOTE(zoubir): the texture coordinates of entry Index's icon, in the
// order RenderQuadTexture takes them (left, bottom, right, top), half a
// texel in from the cell's edge so neighbours never bleed in. The sprite
// shader turns V upside down (texture_shading.vert), so V here counts
// from the bottom of the texture
inline v4
AbilityIconUvs(u32 Index)
{
    float Width = (float)(ABILITY_ATLAS_COLUMNS * ABILITY_ICON_SIZE);
    float Height = (float)(AbilityAtlasRows() * ABILITY_ICON_SIZE);
    float Left = (float)((Index % ABILITY_ATLAS_COLUMNS) * ABILITY_ICON_SIZE) + 0.5f;
    float Top = (float)((Index / ABILITY_ATLAS_COLUMNS) * ABILITY_ICON_SIZE) + 0.5f;
    float Size = (float)ABILITY_ICON_SIZE - 1.f;
    v4 Result = V4(Left / Width, 1.f - (Top + Size) / Height, (Left + Size) / Width,
                   1.f - Top / Height);
    return Result;
}
