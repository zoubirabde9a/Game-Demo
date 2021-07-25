#if !defined(WIN32_APP_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */
struct win32_sound_output
{
    int samplesPerSecond;
    u32 runningSampleIndex;
    int bytesPerSample;
    DWORD secondaryBufferSize;
    float tSine;
    int latencySampleCount;
    DWORD safetyBytes;
};

struct win32_window_dimensions
{
    int Width;
    int Height;
};

struct win32_debug_time_marker
{
    DWORD outputPlayCursor;
    DWORD outputWriteCursor;
    DWORD outputLocation;
    DWORD outputByteCount;
    
    DWORD expectedFlipPlayCursor;
    DWORD flipPlayCursor;
    DWORD flipWriteCursor;
    
};

struct win32_app_code
{
    HMODULE DLL;
    FILETIME lastWriteTime;
    app_update_and_render *updateAndRender;
    app_get_sound_samples *getSoundSamples;

    bool32 isValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_replay_buffer
{
    char replayFileName[WIN32_STATE_FILE_NAME_COUNT];
    void *memoryBlock;
};

struct win32_state
{
    size_t appMemorySize;
    void *appMemoryBlock;
    win32_replay_buffer replayBuffers[4];
        
    HANDLE recordingHandle;
    int inputRecordingIndex;

    HANDLE playbackHandle;
    int inputPlaybackIndex;

    char executableFilePath[WIN32_STATE_FILE_NAME_COUNT];
    char *executableFileName;
};

#define WIN32_APP_H
#endif
