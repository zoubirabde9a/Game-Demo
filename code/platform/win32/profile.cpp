/* Frame profile: with GAME_PROFILE=<file> set, how long each frame's work
   takes, written to that file every WIN32_PROFILE_FRAMES frames as the
   average and the worst of three times:
     frame: from the frame's start until the GPU has finished it
     game:  the game's update and render call, on the CPU (simulation,
            drawing setup, sending the GL commands)
     gpu:   what the GPU still had to do once the game call returned
   Off unless the variable is set, since measuring makes the CPU wait for
   the GPU (glFinish) and so costs the overlap the two normally have.

     set GAME_PROFILE=build\profile.txt
     misc\screenshot.bat out.png 600

   At 60 frames a second the budget is 16.7 ms. */

#define WIN32_PROFILE_FRAMES 120

// NOTE(zoubir): OpenGL 1.1, exported by opengl32.dll
extern "C" __declspec(dllimport) void __stdcall glFinish(void);

enum win32_profile_part
{
    Win32Profile_Frame,
    Win32Profile_Game,
    Win32Profile_Gpu,
    Win32Profile_Count
};

struct win32_profile
{
    char Path[MAX_PATH];
    u32 Frame;
    u32 Count;
    LARGE_INTEGER GameStart;
    LARGE_INTEGER GameEnd;
    float Sum[Win32Profile_Count];
    float Worst[Win32Profile_Count];
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

// NOTE(zoubir): around the game's update and render call
internal void
Win32ProfileGameStart(void)
{
    if (Win32Profile.Path[0])
    {
        Win32Profile.GameStart = Win32GetWallClock();
    }
}

internal void
Win32ProfileGameEnd(void)
{
    if (Win32Profile.Path[0])
    {
        Win32Profile.GameEnd = Win32GetWallClock();
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
    LARGE_INTEGER Now = Win32GetWallClock();
    float Ms[Win32Profile_Count];
    Ms[Win32Profile_Frame] = 1000.f * Win32GetSecondsElapsed(FrameStart, Now);
    Ms[Win32Profile_Game] = 1000.f * Win32GetSecondsElapsed(Profile->GameStart, Profile->GameEnd);
    Ms[Win32Profile_Gpu] = 1000.f * Win32GetSecondsElapsed(Profile->GameEnd, Now);
    Profile->Frame++;
    // NOTE(zoubir): the first frames load assets and build shaders
    if (Profile->Frame <= 30)
    {
        return;
    }
    Profile->Count++;
    for(u32 Part = 0; Part < Win32Profile_Count; Part++)
    {
        Profile->Sum[Part] += Ms[Part];
        Profile->Worst[Part] = Ms[Part] > Profile->Worst[Part] ? Ms[Part] : Profile->Worst[Part];
    }
    if (Profile->Count == WIN32_PROFILE_FRAMES)
    {
        float N = (float)Profile->Count;
        char Line[192];
        int Length = snprintf(Line, sizeof(Line),
                              "frame %u: frame %.2f (worst %.2f), game %.2f (worst %.2f), "
                              "gpu %.2f (worst %.2f) ms\r\n", Profile->Frame,
                              Profile->Sum[0] / N, Profile->Worst[0],
                              Profile->Sum[1] / N, Profile->Worst[1],
                              Profile->Sum[2] / N, Profile->Worst[2]);
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
        for(u32 Part = 0; Part < Win32Profile_Count; Part++)
        {
            Profile->Sum[Part] = 0.f;
            Profile->Worst[Part] = 0.f;
        }
    }
}
