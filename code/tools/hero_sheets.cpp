/* Writes the code-drawn hero animations (art/heroes/) for review: for each class one
   PNG strip per animation and facing, a manifest.json saying how to play
   them, and a contact sheet scaled 4x. Usage: hero_sheets <out folder>. */

#include <direct.h>
#include "../app.cpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image/stb_image_write.h"

struct hero_class_out
{
    char *Name;
    hero_look Look;
};

global_variable char *HeroFacingNames[HeroFacing_Count] = {"down", "up", "right"};

int main(int ArgCount, char **Args)
{
    char *OutDir = ArgCount > 1 ? Args[1] : "hero_art";
    _mkdir(OutDir);
    hero_class_out Classes[] =
    {
        {"firemage", FireMageLook()},
        {"bulwark", BulwarkLook()},
        {"mender", MenderLook()},
    };
    u32 Size = HERO_FRAME_SIZE;
    char Path[512];
    snprintf(Path, sizeof(Path), "%s/manifest.json", OutDir);
    FILE *Manifest = fopen(Path, "wb");
    fprintf(Manifest, "{\"frameSize\": %u, \"feetY\": %d, \"bodyHeight\": 34, \"classes\": {",
            Size, (int)HERO_FEET_Y);
    for(u32 ClassIndex = 0; ClassIndex < ArrayCount(Classes); ClassIndex++)
    {
        hero_class_out *Class = &Classes[ClassIndex];
        snprintf(Path, sizeof(Path), "%s/%s", OutDir, Class->Name);
        _mkdir(Path);
        fprintf(Manifest, "%s\"%s\": {", ClassIndex ? ", " : "", Class->Name);

        u32 SheetW = Size * HERO_SHEET_COLUMNS;
        u32 SheetH = Size * HeroAnim_Count * HeroFacing_Count;
        u32 *Sheet = (u32 *)calloc(SheetW * SheetH, sizeof(u32));
        bool32 First = true;
        for(u32 Anim = 0; Anim < HeroAnim_Count; Anim++)
        {
            hero_anim_def *Def = &HeroAnimDefs[Anim];
            for(u32 Facing = 0; Facing < HeroFacing_Count; Facing++)
            {
                u32 Row = Anim * HeroFacing_Count + Facing;
                u32 *Strip = (u32 *)calloc(Size * Def->FrameCount * Size, sizeof(u32));
                for(u32 Frame = 0; Frame < Def->FrameCount; Frame++)
                {
                    sprite_canvas Canvas = CanvasFrame(Sheet, SheetW, Size, Frame, Row);
                    hero_pose Pose = HeroPoseFor(&Class->Look, (hero_anim)Anim,
                                                 (hero_facing)Facing, Frame);
                    DrawHero(&Canvas, &Class->Look, &Pose);
                    for(u32 Y = 0; Y < Size; Y++)
                    {
                        for(u32 X = 0; X < Size; X++)
                        {
                            Strip[Y * Size * Def->FrameCount + Frame * Size + X] =
                                Canvas.Pixels[Y * Canvas.Stride + X];
                        }
                    }
                }
                snprintf(Path, sizeof(Path), "%s/%s/%s_%s.png", OutDir, Class->Name,
                         Def->Name, HeroFacingNames[Facing]);
                stbi_write_png(Path, Size * Def->FrameCount, Size, 4, Strip,
                               Size * Def->FrameCount * 4);
                free(Strip);
                fprintf(Manifest, "%s\"%s_%s\": {\"frames\": %u, \"fps\": %.1f, \"loop\": %s}",
                        First ? "" : ", ", Def->Name, HeroFacingNames[Facing], Def->FrameCount,
                        1.f / Def->SecondsPerFrame, Def->Loops ? "true" : "false");
                First = false;
            }
        }
        fprintf(Manifest, "}");

        u32 Scale = 4;
        u32 OutW = SheetW * Scale;
        u32 OutH = SheetH * Scale;
        u32 *Out = (u32 *)calloc(OutW * OutH, sizeof(u32));
        for(u32 Y = 0; Y < OutH; Y++)
        {
            for(u32 X = 0; X < OutW; X++)
            {
                u32 Pixel = Sheet[(Y / Scale) * SheetW + X / Scale];
                bool32 Edge = (X / Scale) % Size == 0 || (Y / Scale) % Size == 0;
                u32 Checker = ((X / 16 + Y / 16) % 2) ? ART_RGB(58, 66, 54) : ART_RGB(66, 74, 60);
                Out[Y * OutW + X] = Pixel ? Pixel : (Edge ? ART_RGB(90, 90, 90) : Checker);
            }
        }
        snprintf(Path, sizeof(Path), "%s/%s_sheet.png", OutDir, Class->Name);
        stbi_write_png(Path, OutW, OutH, 4, Out, OutW * 4);
        printf("%s\n", Path);
        free(Out);
        free(Sheet);
    }
    fprintf(Manifest, "}}\n");
    fclose(Manifest);
    return 0;
}
