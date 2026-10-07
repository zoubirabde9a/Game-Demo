/* Frame profile: with GAME_PROFILE=<file> set, how long each frame's work
   takes, CPU and GPU together, written to that file every
   WIN32_PROFILE_FRAMES frames as the average and the worst. Off unless
   the variable is set, since measuring makes the CPU wait for the GPU
   (glFinish) and so costs the overlap the two normally have.

     set GAME_PROFILE=build\profile.txt
     misc\screenshot.bat out.png 600

   Each line: the frame it ends on, average ms, worst ms. At 60 frames a
   second the budget is 16.7 ms. */

#define WIN32_PROFILE_FRAMES 120

// NOTE(zoubir): OpenGL 1.1, exported by opengl32.dll
extern "C" __declspec(dllimport) void __stdcall glFinish(void);

struct win32_profile
{
    char Path[MAX_PATH];
    u32 Frame;
    u32 Count;
    float Sum;
    float Worst;
};

global_variable win32_profile Win32Profile;

internal void
Win32InitProfile(void)
{
    Win32Profile = {};
    if (!GetEnvironmentVariableA("GAME_PROFILE", Win32Profile.Path,
                                 sizeof(Win32Profile.Path)))
    {
        Win32Profile.Path[0] = 0;
    }
}

// NOTE(zoubir): after the frame is drawn and before waiting out the rest
// of it; FrameStart is when the frame began
internal void
Win32ProfileFrame(LARGE_INTEGER FrameStart)
{
    win32_profile *Profile = &Win32Profile;
    if (!Profile->Path[0])
    {
        return;
    }
    glFinish();
    float Ms = 1000.f * Win32GetSecondsElapsed(FrameStart, Win32GetWallClock());
    Profile->Frame++;
    // NOTE(zoubir): the first frames load assets and build shaders
    if (Profile->Frame <= 30)
    {
        return;
    }
    Profile->Count++;
    Profile->Sum += Ms;
    Profile->Worst = Ms > Profile->Worst ? Ms : Profile->Worst;
    if (Profile->Count == WIN32_PROFILE_FRAMES)
    {
        char Line[96];
        int Length = snprintf(Line, sizeof(Line), "frame %u: average %.2f ms, worst %.2f ms\r\n",
                              Profile->Frame, Profile->Sum / (float)Profile->Count,
                              Profile->Worst);
        HANDLE File = CreateFileA(Profile->Path, FILE_APPEND_DATA, FILE_SHARE_READ, 0,
                                  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (File != INVALID_HANDLE_VALUE)
        {
            DWORD Written;
            if (Length > 0)
            {
                WriteFile(File, Line, (DWORD)Length, &Written, 0);
            }
            CloseHandle(File);
        }
        Profile->Count = 0;
        Profile->Sum = 0.f;
        Profile->Worst = 0.f;
    }
}
