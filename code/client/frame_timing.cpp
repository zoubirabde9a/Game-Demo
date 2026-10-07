/* Frame timing: where the game's own time in a frame goes, by part, when
   GAME_PROFILE names a file (the Windows layer's profile, which times the
   frame as a whole and the GPU, writes there too). Every
   FRAME_TIMING_FRAMES frames this writes <file>.parts.txt again with one
   more line: each part's average and worst, in ms.

   AppUpdateAndRender (app.cpp) calls FrameTimingStart at the top and
   FrameTimingMark after each part; a mark charges the time since the one
   before to its part. Needs Platform.WallSeconds; without it (the browser,
   the server) it does nothing. */

#define FRAME_TIMING_FRAMES 120
#define FRAME_TIMING_LOG 4096

enum frame_part
{
    FramePart_Simulate, // input, the world tick (or the server's snapshot), events
    FramePart_Ground,   // camera, the tile map and everything on the ground
    FramePart_Entities, // queueing every entity
    FramePart_Flush,    // sorting the world pass and sending it to the GPU
    FramePart_Grade,    // lights, bloom, the grade pass
    FramePart_Screens,  // overlays, HUD and screens
    FramePart_Count
};

global_variable char *FramePartNames[FramePart_Count] =
{
    "simulate", "ground", "entities", "flush", "grade", "screens",
};

struct frame_timing
{
    bool32 On;
    char Path[256];
    double Last;
    u32 Frames;
    float Sum[FramePart_Count];
    float Worst[FramePart_Count];
    float This[FramePart_Count];
    char Log[FRAME_TIMING_LOG];
    u32 LogUsed;
};

internal void
FrameTimingStart(app_state *AppState)
{
    if (!AppState->FrameTiming)
    {
        AppState->FrameTiming = AllocateStruct(&AppState->MemoryArena, frame_timing);
        *AppState->FrameTiming = {};
#pragma warning(push)
#pragma warning(disable: 4996) // getenv: read once, never kept
        char *Path = getenv("GAME_PROFILE");
#pragma warning(pop)
        if (Path && Path[0] && Platform.WallSeconds)
        {
            snprintf(AppState->FrameTiming->Path, sizeof(AppState->FrameTiming->Path),
                     "%s.parts.txt", Path);
            AppState->FrameTiming->On = true;
        }
    }
    frame_timing *Timing = AppState->FrameTiming;
    // NOTE(zoubir): a code reload clears the platform table's pointers
    // until it is copied again, so check every frame
    if (Timing->On && Platform.WallSeconds)
    {
        Timing->Last = Platform.WallSeconds();
        for(u32 Part = 0; Part < FramePart_Count; Part++)
        {
            Timing->This[Part] = 0.f;
        }
    }
}

internal void
FrameTimingMark(app_state *AppState, frame_part Part)
{
    frame_timing *Timing = AppState->FrameTiming;
    if (!Timing || !Timing->On || !Platform.WallSeconds)
    {
        return;
    }
    double Now = Platform.WallSeconds();
    Timing->This[Part] += (float)(1000.0 * (Now - Timing->Last));
    Timing->Last = Now;
}

// NOTE(zoubir): at the end of the frame
internal void
FrameTimingEnd(app_state *AppState)
{
    frame_timing *Timing = AppState->FrameTiming;
    if (!Timing || !Timing->On)
    {
        return;
    }
    for(u32 Part = 0; Part < FramePart_Count; Part++)
    {
        Timing->Sum[Part] += Timing->This[Part];
        Timing->Worst[Part] = Maximum(Timing->Worst[Part], Timing->This[Part]);
    }
    Timing->Frames++;
    if (Timing->Frames < FRAME_TIMING_FRAMES)
    {
        return;
    }
    char Line[512];
    u32 Used = 0;
    for(u32 Part = 0; Part < FramePart_Count; Part++)
    {
        int Wrote = snprintf(Line + Used, sizeof(Line) - Used, "%s %.2f (worst %.2f)%s",
                             FramePartNames[Part], Timing->Sum[Part] / (float)Timing->Frames,
                             Timing->Worst[Part], Part + 1 < FramePart_Count ? ", " : " ms\r\n");
        if (Wrote < 0 || (u32)Wrote >= sizeof(Line) - Used)
        {
            break;
        }
        Used += (u32)Wrote;
    }
    // NOTE(zoubir): the oldest lines go when the log is full
    if (Timing->LogUsed + Used > FRAME_TIMING_LOG)
    {
        Timing->LogUsed = 0;
    }
    memcpy(Timing->Log + Timing->LogUsed, Line, Used);
    Timing->LogUsed += Used;
    Platform.WriteEntireFile(Timing->Path, Timing->LogUsed, Timing->Log);
    Timing->Frames = 0;
    for(u32 Part = 0; Part < FramePart_Count; Part++)
    {
        Timing->Sum[Part] = 0.f;
        Timing->Worst[Part] = 0.f;
    }
}
