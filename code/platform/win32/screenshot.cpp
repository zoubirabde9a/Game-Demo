/* Developer screenshots. Set GAME_SCREENSHOT=<file.png> and the game
   saves the frame it drew after GAME_SCREENSHOT_FRAME frames (default 90,
   1.5 seconds) and quits. Reads the back buffer, so it works when the
   window is covered or off screen, where a desktop capture shows nothing.
   misc\screenshot.bat wraps it.

   GAME_SCREENSHOT_KEYS scripts the keys, so a shot can show an action:
   space-separated From[-To]:Key, holding Key from frame From to To (just
   From when there is no To). Key is a letter, _ for Space, < and > for
   the left and right mouse buttons, Alt, or F1 to F9 (F4 closes the Play
   screen offline shots open on; F6 gives a level in developer builds).
   M:X,Y puts the mouse at X,Y in the window; From:@X,Y moves it there
   from frame From on, so one shot can click several places.
   "2:F4 5-90:D 40:A M:900,400" closes the Play screen, walks right,
   launches at frame 40 and aims to the right of the middle.

   GAME_WINDOW=1920x1080 (or =fullscreen) opens the game at that size, so
   a shot can check a layout at a resolution this monitor does not have
   (window.cpp). The PNG is in screen pixels: on a monitor at 150% it is
   1.5 times the size asked for. */

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

struct win32_scripted_key
{
    u32 From;
    u32 To;
    char Key[4];
    // NOTE(zoubir): for a mouse move (Key "@"), where to
    i32 MouseX, MouseY;
};

struct win32_screenshot
{
    char Path[MAX_PATH];
    u32 Frame;
    u32 FramesDrawn;
    win32_scripted_key Keys[32];
    u32 KeyCount;
    bool32 HasMouse;
    i32 MouseX, MouseY;
};

// NOTE(zoubir): reads GAME_SCREENSHOT_KEYS (see the top of this file)
internal void
Win32ParseScriptedKeys(win32_screenshot *Shot, char *Text)
{
    while (*Text)
    {
        while (*Text == ' ') Text++;
        if (!*Text) break;
        if (Text[0] == 'M' && Text[1] == ':')
        {
            Text += 2;
            Shot->MouseX = (i32)strtol(Text, &Text, 10);
            if (*Text == ',') Text++;
            Shot->MouseY = (i32)strtol(Text, &Text, 10);
            Shot->HasMouse = true;
        }
        else
        {
            win32_scripted_key Key = {};
            Key.From = (u32)strtoul(Text, &Text, 10);
            Key.To = Key.From;
            if (*Text == '-') Key.To = (u32)strtoul(Text + 1, &Text, 10);
            if (*Text == ':' && Text[1] == '@')
            {
                Text += 2;
                Key.Key[0] = '@';
                Key.MouseX = (i32)strtol(Text, &Text, 10);
                if (*Text == ',') Text++;
                Key.MouseY = (i32)strtol(Text, &Text, 10);
                if (Shot->KeyCount < ArrayCount(Shot->Keys))
                {
                    Shot->Keys[Shot->KeyCount++] = Key;
                }
            }
            else if (*Text == ':' && Text[1])
            {
                Text++;
                for(u32 Index = 0; Index < 3 && *Text && *Text != ' '; Index++)
                {
                    Key.Key[Index] = *Text++;
                }
                if (Shot->KeyCount < ArrayCount(Shot->Keys))
                {
                    Shot->Keys[Shot->KeyCount++] = Key;
                }
            }
        }
        while (*Text && *Text != ' ') Text++;
    }
}

internal app_button_state *
Win32ScriptedButton(app_input *Input, char *Name)
{
    char Key = Name[0];
    if (Key == 'F' && Name[1] >= '1' && Name[1] <= '9') return &Input->FButtons[Name[1] - '1'];
    if (Key == 'A' && Name[1] == 'l') return &Input->AltButton;
    if (Key >= 'a' && Key <= 'z') Key = (char)(Key - 'a' + 'A');
    if (Key >= 'A' && Key <= 'Z') return &Input->AlphaButtons[Key - 'A'];
    if (Key == '_') return &Input->SpaceButton;
    if (Key == '<') return &Input->LeftButton;
    if (Key == '>') return &Input->RightButton;
    return 0;
}

// NOTE(zoubir): after the real keyboard and mouse are read, so the script
// wins; the frame counted is the one about to be drawn
internal void
Win32ApplyScriptedKeys(win32_screenshot *Shot, app_input *Input)
{
    u32 Frame = Shot->FramesDrawn + 1;
    for(u32 Index = 0; Index < Shot->KeyCount; Index++)
    {
        win32_scripted_key *Key = &Shot->Keys[Index];
        app_button_state *Button = Key->Key[0] == '@' ? 0 :
            Win32ScriptedButton(Input, Key->Key);
        if (Button && Frame >= Key->From && Frame <= Key->To)
        {
            Button->EndedDown = true;
            Button->Pressed = Button->Pressed || Frame == Key->From;
        }
    }
    if (Shot->HasMouse)
    {
        Input->MouseX = Shot->MouseX;
        Input->MouseY = Shot->MouseY;
    }
    u32 LatestMove = 0;
    for(u32 Index = 0; Index < Shot->KeyCount; Index++)
    {
        win32_scripted_key *Key = &Shot->Keys[Index];
        if (Key->Key[0] == '@' && Frame >= Key->From && Key->From >= LatestMove)
        {
            LatestMove = Key->From;
            Input->MouseX = Key->MouseX;
            Input->MouseY = Key->MouseY;
        }
    }
}

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
    char Keys[512];
    if (GetEnvironmentVariableA("GAME_SCREENSHOT_KEYS", Keys, sizeof(Keys)))
    {
        Win32ParseScriptedKeys(Shot, Keys);
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
