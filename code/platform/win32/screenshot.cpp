/* Developer screenshots. Set GAME_SCREENSHOT=<file.png> and the game
   saves the frame it drew after GAME_SCREENSHOT_FRAME frames (default 90,
   1.5 seconds) and quits. Reads the back buffer, so it works when the
   window is covered or off screen, where a desktop capture shows nothing.
   misc\screenshot.bat wraps it. */

#pragma warning(push, 0)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "../../third_party/stb_image/stb_image_write.h"
#pragma warning(pop)

// NOTE(zoubir): OpenGL 1.1, exported by opengl32.dll; engine/opengl.h
// declares only what the game loads by pointer
#define WIN32_GL_PACK_ALIGNMENT 0x0D05
#define WIN32_GL_BACK 0x0405
extern "C" __declspec(dllimport) void __stdcall
glPixelStorei(unsigned int pname, int param);
extern "C" __declspec(dllimport) void __stdcall
glReadBuffer(unsigned int mode);
extern "C" __declspec(dllimport) void __stdcall
glReadPixels(int x, int y, int width, int height, unsigned int format,
             unsigned int type, void *pixels);

struct win32_screenshot
{
    char Path[MAX_PATH];
    u32 Frame;
    u32 FramesDrawn;
};

internal void
Win32InitScreenshot(win32_screenshot *Shot)
{
    *Shot = {};
    if (!GetEnvironmentVariableA("GAME_SCREENSHOT", Shot->Path,
                                 sizeof(Shot->Path)))
    {
        Shot->Path[0] = 0;
        return;
    }
    char Frame[16];
    Shot->Frame = 90;
    if (GetEnvironmentVariableA("GAME_SCREENSHOT_FRAME", Frame,
                                sizeof(Frame)))
    {
        Shot->Frame = (u32)atoi(Frame);
    }
}

internal void
Win32WritePngChunk(void *Context, void *Data, int Size)
{
    HANDLE File = (HANDLE)Context;
    DWORD Written;
    WriteFile(File, Data, (DWORD)Size, &Written, 0);
}

// NOTE(zoubir): call after the frame is drawn, before the swap. Returns
// true once the file is written, which means the game should quit
internal bool32
Win32SaveScreenshotIfDue(win32_screenshot *Shot, int Width, int Height)
{
    if (!Shot->Path[0] || ++Shot->FramesDrawn < Shot->Frame ||
        Width <= 0 || Height <= 0)
    {
        return false;
    }

    u32 RowSize = 4 * (u32)Width;
    u8 *Pixels = (u8 *)VirtualAlloc(0, RowSize * Height,
                                    MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (Pixels)
    {
        glPixelStorei(WIN32_GL_PACK_ALIGNMENT, 1);
        glReadBuffer(WIN32_GL_BACK);
        glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
        // NOTE(zoubir): the back buffer's alpha is not meant to be seen
        for(u32 Index = 3; Index < RowSize * Height; Index += 4)
        {
            Pixels[Index] = 255;
        }
        HANDLE File = CreateFileA(Shot->Path, GENERIC_WRITE, 0, 0,
                                  CREATE_ALWAYS, 0, 0);
        if (File != INVALID_HANDLE_VALUE)
        {
            // NOTE(zoubir): OpenGL rows run bottom to top
            stbi_flip_vertically_on_write(1);
            stbi_write_png_to_func(Win32WritePngChunk, File, Width, Height,
                                   4, Pixels, (int)RowSize);
            CloseHandle(File);
        }
        VirtualFree(Pixels, 0, MEM_RELEASE);
    }
    return true;
}
