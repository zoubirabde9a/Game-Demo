/* Writes every monster's sprite sheet to build/monster_art/<kind>.png,
   scaled up 4x on a dark checkerboard, so art can be reviewed without
   starting the game. Built and run by art.bat. */

#include <windows.h>
#include "../app.cpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image/stb_image_write.h"

#define PREVIEW_SCALE 4

int main()
{
    CreateDirectoryA("monster_art", 0);
    for(u32 KindIndex = 0; KindIndex < MonsterKind_Count; KindIndex++)
    {
        monster_def *Def = GetMonsterDef((monster_kind)KindIndex);
        u32 Width = MonsterSheetWidth(Def);
        u32 Height = MonsterSheetHeight(Def);
        u32 *Sheet = (u32 *)calloc(Width * Height, sizeof(u32));
        BuildMonsterSheet((monster_kind)KindIndex, Sheet);

        u32 OutWidth = Width * PREVIEW_SCALE;
        u32 OutHeight = Height * PREVIEW_SCALE;
        u32 *Out = (u32 *)calloc(OutWidth * OutHeight, sizeof(u32));
        for(u32 Y = 0; Y < OutHeight; Y++)
        {
            for(u32 X = 0; X < OutWidth; X++)
            {
                u32 Pixel = Sheet[(Y / PREVIEW_SCALE) * Width + X / PREVIEW_SCALE];
                bool32 FrameEdge = (X / PREVIEW_SCALE) % Def->FrameSize == 0 ||
                    (Y / PREVIEW_SCALE) % Def->FrameSize == 0;
                u32 Checker = ((X / 16 + Y / 16) % 2) ? ART_RGB(58, 66, 54) :
                    ART_RGB(66, 74, 60);
                Out[Y * OutWidth + X] = Pixel ? Pixel :
                    (FrameEdge ? ART_RGB(90, 90, 90) : Checker);
            }
        }
        char Path[256];
        snprintf(Path, sizeof(Path), "monster_art/%s.png", Def->Name);
        stbi_write_png(Path, OutWidth, OutHeight, 4, Out, OutWidth * 4);
        printf("%s\n", Path);
        free(Out);
        free(Sheet);
    }
    for(u32 Style = 0; Style < ShotStyle_Count; Style++)
    {
        u32 Width = SHOT_FRAME_SIZE * SHOT_FRAMES;
        u32 Sheet[SHOT_FRAME_SIZE * SHOT_FRAMES * SHOT_FRAME_SIZE];
        BuildShotSheet((monster_shot_style)Style, Sheet);
        u32 Scale = 8;
        u32 OutWidth = Width * Scale;
        u32 OutHeight = SHOT_FRAME_SIZE * Scale;
        u32 *Out = (u32 *)calloc(OutWidth * OutHeight, sizeof(u32));
        for(u32 Y = 0; Y < OutHeight; Y++)
        {
            for(u32 X = 0; X < OutWidth; X++)
            {
                u32 Pixel = Sheet[(Y / Scale) * Width + X / Scale];
                Out[Y * OutWidth + X] = Pixel ? Pixel : ART_RGB(58, 66, 54);
            }
        }
        char Path[256];
        snprintf(Path, sizeof(Path), "monster_art/shot_%u.png", Style);
        stbi_write_png(Path, OutWidth, OutHeight, 4, Out, OutWidth * 4);
        printf("%s\n", Path);
        free(Out);
    }
    return 0;
}
