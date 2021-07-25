
#include <malloc.h>
#include <windows.h>
#include <winbase.h>
#include "directx/dsound.h"
#include <stdio.h>
#include "directx/xinput.h"

#include "app_platform.h"
#include "win32_app.h"
#include "win32_opengl.cpp"
//#include "GL\glew.h"

global_variable bool Running; 
global_variable bool32 GlobalPause;
global_variable bool32 GlobalInactiveApp;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable i64 GlobalPerCounterFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

// NOTE(zoubir): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_STATE*)
typedef X_INPUT_GET_STATE(x_input_get_state);
internal X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(zoubir): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD, XINPUT_VIBRATION*)
typedef X_INPUT_SET_STATE(x_input_set_state);
internal X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID, LPDIRECTSOUND*, LPUNKNOWN);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

void
CatStrings(size_t SourceALength, char *SourceA,
           size_t SourceBLength, char *SourceB,
           size_t DestCount, char *Dest)
{
    for(size_t i = 0;
        i < SourceALength;
        i++)
    {
        *Dest++ = SourceA[i];
    }

    for(size_t i = 0;
        i < SourceBLength;
        i++)
    {
        *Dest++ = SourceB[i];
    }
    *Dest = 0;
}

internal void
Win32GetExecutableFileName(win32_state *state)
{
    DWORD sizeOfFileName =
        GetModuleFileName(0, state->executableFilePath, sizeof(state->executableFilePath));
    char *onePastLastSlash = state->executableFilePath;
    for(char *scan = onePastLastSlash;
        *scan;
        scan++)
    {
        if (*scan == '\\')
        {
            onePastLastSlash = scan + 1;
        }      
    }
    state->executableFileName = onePastLastSlash;
}

internal size_t
StringLength(char *string)
{
    size_t length = 0;
    while(*string++)
    {
        length++;
    }
    return length;
}

internal void
Win32BuildExecutablePathFileName(win32_state *state, char *fileName,
                                 size_t destLength, char *dest)
{
    CatStrings(state->executableFileName - state->executableFilePath, state->executableFilePath,
               StringLength(fileName), fileName,
               destLength, dest);
}

#if 0
PLATFORM_READ_ENTIRE_FILE(PlatformReadEntireFile)
{
    read_file_result result = {};
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ,
                                    FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, 0, 0);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return result;

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize))
        return result;

    u32 fileSize32 = safeTruncateU32(fileSize.QuadPart);
    result.memory = AllocateTransient(Thread, Memory, fileSize32);
    if (!result.memory)
        return result;

    DWORD bytesRead;
    if (ReadFile(fileHandle, result.memory, fileSize32, &bytesRead, 0) &&
        bytesRead == fileSize32)
    {
        result.size = fileSize32;
    }
    else
    {
        DEBUGPlatformFreeFileMemory(thread, result.memory);
        result.memory = 0;
    }

    CloseHandle(fileHandle);
    return result;                               
}
#endif

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result result = {};
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ,
                                    FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, 0, 0);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        // TODO(zoubir): Logging
        Assert(0);
        return result;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize))
    {
        Assert(0);
        return result;
    }

    u32 fileSize32 = safeTruncateU32(fileSize.QuadPart);
    result.Memory = VirtualAlloc(0, fileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!result.Memory)
    {
        Assert(0);
        return result;
    }

    DWORD bytesRead;
    if (ReadFile(fileHandle, result.Memory, fileSize32, &bytesRead, 0) &&
        bytesRead == fileSize32)
    {
        result.Size = fileSize32;
    }
    else
    {
        DEBUGPlatformFreeFileMemory(result.Memory);
        result.Memory = 0;
    }

    CloseHandle(fileHandle);
    return result;                               
}

inline void *
Win32AllocateSize(u32 Size)
{
     void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
//     ZeroSize(Result, Size);
     return Result;
}

#define Win32AllocateStruct(Type) (Type *)(Win32AllocateSize(sizeof(Type)))
inline void
Win32Free(void *Memory)
{
    VirtualFree(Memory, 0, MEM_RELEASE);
}

struct win32_platform_file_handle
{
    platform_file_handle H;
    HANDLE Handle;
};

struct win32_platform_file_group
{
    platform_file_group H;
    HANDLE FindHandle;
    WIN32_FIND_DATAA FindData;
};

internal
PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
{
    win32_platform_file_group *Win32FileGroup =
        Win32AllocateStruct(win32_platform_file_group);

    char *TypeAt = Type;
    char WildCard[32] = "*.";
    for(u32 WildCardIndex = 2;
        WildCardIndex < sizeof(WildCard);
        WildCardIndex++)
    {
        WildCard[WildCardIndex] = *TypeAt;
        if (*TypeAt == '\0')
        {
            break;
        }
        TypeAt++;
    }
    WildCard[sizeof(WildCard) - 1] = '\0';

    Win32FileGroup->H.FileCount = 0;

    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);
    while(FindHandle != INVALID_HANDLE_VALUE)
    {
        Win32FileGroup->H.FileCount++;

        if (!FindNextFileA(FindHandle, &FindData))
        {
            break;
        }
    }
    FindClose(FindHandle);

    Win32FileGroup->FindHandle = FindFirstFileA(WildCard, &Win32FileGroup->FindData);
    
    return (platform_file_group *)Win32FileGroup;
}

internal
PLATFORM_GET_ALL_FILES_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
{
    win32_platform_file_group *Win32FileGroup =
        (win32_platform_file_group *)(FileGroup);
    if(Win32FileGroup)
    {
        FindClose(Win32FileGroup->FindHandle);

        Win32Free(Win32FileGroup);
    }

}

internal
PLATFORM_OPEN_NEXT_FILE(Win32OpenNextFile)
{
    win32_platform_file_group *Win32FileGroup =
        (win32_platform_file_group *)FileGroup;
    win32_platform_file_handle *Result = 0;

    if (Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
    {
        Result = Win32AllocateStruct(win32_platform_file_handle);
        if(Result)
        {
            char *FileName = Win32FileGroup->FindData.cFileName;
            Result->Handle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            Result->H.HasErrors = (Result->Handle == INVALID_HANDLE_VALUE);
        }
        
        if(!FindNextFileA(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
        {
            FindClose(Win32FileGroup->FindHandle);
            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
        }        
    }

    return (platform_file_handle *)Result;
}

internal
PLATFORM_FILE_ERROR(Win32FileError)
{
#if HANDMADE_INTERNAL
    OutputDebugString("WIN32 FILE ERROR: ");
    OutputDebugString(Message);
    OutputDebugString("\n");
#endif
    
    Handle->HasErrors = true;
    
}

internal
PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
    if(PlatformNoFileErrors(Source))
    {
        win32_platform_file_handle *Handle =
            (win32_platform_file_handle *)Source;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);
    
        u32 FileSize32 = (u32)(Size);

        Assert(FileSize32 == Size) ;
        
        DWORD BytesRead;
        if(ReadFile(Handle->Handle, Dest, FileSize32, &BytesRead, &Overlapped) &&
           (FileSize32 == BytesRead))
        {
            // NOTE(Zoubir): File read succeeded!
        }
        else
        {
            Win32FileError(&Handle->H, "Read file failed.");
        }
    }
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 result = false;
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_WRITE,
                                    0, 0,
                                    CREATE_ALWAYS, 0, 0);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return result;

    DWORD bytesWritten;
    if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
    {
        result = bytesWritten == memorySize;
    }
    CloseHandle(fileHandle);
    return result;         
}
                             

HRESULT WINAPI
DirectSoundCreate(LPCGUID pcGuidDevice,
                  LPDIRECTSOUND *ppDS,
                  LPUNKNOWN pUnkOuter);

void*
PlatformLoadFile(char *fileName)
{
    return 0;
}

inline FILETIME
Win32GetFileLastWriteTime(char *fileName)
{
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(fileName, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }
    
    return lastWriteTime;
    
}

internal win32_app_code
Win32LoadAppCode(char *sourceDLLName, char*tempDLLName)
{
    win32_app_code result = {};

    CopyFile(sourceDLLName, tempDLLName, FALSE);
    result.DLL = LoadLibrary(tempDLLName);
    
    if (result.DLL)
    {
        result.lastWriteTime = Win32GetFileLastWriteTime(sourceDLLName);
        result.updateAndRender = (app_update_and_render  *)GetProcAddress(result.DLL, "AppUpdateAndRender");
        result.getSoundSamples = (app_get_sound_samples  *)GetProcAddress(result.DLL, "AppGetSoundSamples");

        result.isValid = result.updateAndRender &&
            result.getSoundSamples;
    }
    
    if (!result.isValid)
    {
        result.updateAndRender = AppUpdateAndRenderStub;
        result.getSoundSamples = AppGetSoundSamplesStub;    
    }
    
    return result;
}

internal void
Win32UnloadAppCode(win32_app_code *appCode)
{
    if (appCode->DLL)
    {
        FreeLibrary(appCode->DLL);
        appCode->DLL = 0;
    }
    
    appCode->isValid = false;
    appCode->updateAndRender = AppUpdateAndRenderStub;
    appCode->getSoundSamples = AppGetSoundSamplesStub;
}

internal void
Win32LoadXInput()
{
    HMODULE xInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibrary("xinput9_1_0.dll");        
    }
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibrary("xinput1_3.dll");
    }
    
    if (xInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
    }
}

internal void
Win32InitDSound(HWND window, i32 samplesPerSecond, i32 bufferSize)
{
    // NOTE(zoubir): Load the library
    HMODULE dSoundLibrary = LoadLibrary("dsound.dll");
    if (dSoundLibrary)
    {
        // NOTE(zoubir): Get a DirectSound object!
        direct_sound_create *DirectSoundCreate = (direct_sound_create*)
            GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND directSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2;
            waveFormat.nSamplesPerSec = samplesPerSecond;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;
            
            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                // NOTE(zoubir): "Create" a primary buffer
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    HRESULT error = primaryBuffer->SetFormat(&waveFormat);
                    if (SUCCEEDED(error))
                    {
//                        OutputDebugStringA("primary buffer was set\n");
                    }
                    else
                    {
                        // TODO(zoubir): Diagnostic
                    }
                }
                else
                {
                    // TODO(casey): Diagnostic
                }
            }
            else
            {
                    // TODO(casey): Diagnostic
            }
            
            // NOTE(zoubir): "Create" a seconday buffer
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            #if APP_DEV
            bufferDescription.dwFlags |= DSBCAPS_GLOBALFOCUS;
            #endif
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(error))
            {
                // NOTE(zoubir): Start
//                OutputDebugStringA("secondary buffer was created\n");
            }
            
            
        }
        else
        {
            // TODO(zoubir): Diagnostic
        }
    }
    else
    {
        // TODO(zoubir): Diagnostic
    }
}

internal win32_window_dimensions
GetWindowDimension(HWND window)
{
    win32_window_dimensions result;    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.Width = clientRect.right - clientRect.left;
    result. Height = clientRect.bottom - clientRect.top;
    return result;
}

internal void
Win32DisplayBufferInWindow(HDC deviceContext)
{
#if 0
    StretchDIBits(deviceContext,
                  0, 0, buffer->width, buffer->height, //windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->height,
                  buffer->memory,
                  &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
#endif
//    glClearColor(1.f, 0.f, 1.f, 0.f);
//    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers(deviceContext);
}

internal void
ToggleFullscreen(HWND Window)
{
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

LRESULT CALLBACK
MainWindowCallBack(
    HWND   window,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam)

{
    LRESULT result = 0; 
    switch(message)

    {
        case WM_SIZE:
        {
            break;
        }
        
        case WM_DESTROY:
        {
            Running = false;
            PostQuitMessage(0);
            break;
        }

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(0);
        }
        
        case WM_ACTIVATEAPP:
        {
            GlobalInactiveApp = (wParam == FALSE);
            OutputDebugString("WM_ACTIVATEAPP\n");            
            break;
        }

        case WM_CLOSE:
        {
            Running = false;
            OutputDebugString("WM_CLOSE\n");
            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT paint;            
            HDC deviceContext = BeginPaint(window, &paint);
            Win32DisplayBufferInWindow(deviceContext);
            EndPaint(window, &paint);
        }

        default:
        {
            result = DefWindowProcA(window, message, wParam, lParam); 
            break;
        }
    }
    
    return result; 
}

internal void
Win32ClearSoundBuffer(win32_sound_output *soundOutput)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;                        
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, soundOutput->secondaryBufferSize,
                                              &region1, &region1Size,
                                              &region2, &region2Size,
                                              0)))
    {
        // TODO(zoubir): assert region1 region2 size valid
        u8 *destByte = (u8 *)region1;
        for(DWORD byteIndex = 0;
            byteIndex < region1Size;
            byteIndex++)
        {
            *destByte++ = 0;
        }
                         
        destByte = (u8 *)region2;
        for(DWORD byteIndex = 0;
            byteIndex < region2Size;
            byteIndex++)
        {
            *destByte++ = 0;
        }
        GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite,
                     app_sound_output_buffer *srcBuffer)
{
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;                        
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(byteToLock, bytesToWrite,
                                              &region1, &region1Size,
                                              &region2, &region2Size,
                                              0)))
    {
        DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
        i16 *destSample = (i16 *)region1;
        i16 *srcSample = srcBuffer->Samples;
        for(DWORD sampleIndex = 0;
            sampleIndex < region1SampleCount;
            sampleIndex++)
        {
            *destSample++ = *srcSample++;
            *destSample++ = *srcSample++;
            ++soundOutput->runningSampleIndex;
        }
                         
        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        destSample = (i16 *)region2;
        for(DWORD sampleIndex = 0;
            sampleIndex < region2SampleCount;
            sampleIndex++)
        {
            *destSample++ = *srcSample++;
            *destSample++ = *srcSample++;
            ++soundOutput->runningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void
Win32ProcessXInputDigitalButton(DWORD xInputButtonState,
                                app_button_state *oldState,
                                DWORD buttonBit,
                                app_button_state *newState)
{
    newState->EndedDown = ((xInputButtonState & buttonBit) == buttonBit)? 1 : 0;
    newState->halfTransitionCount = (oldState->EndedDown != newState->EndedDown)? 1 : 0;
}

internal void
Win32ProcessKeyboardMessage(app_button_state *old,
                            app_button_state *State,
                            bool32 isDown)
{

    State->Pressed = !old->EndedDown && isDown;
    State->Released = old->EndedDown && !isDown;
    if (State->EndedDown != isDown)
    {
        State->EndedDown = isDown;
        State->halfTransitionCount++;
    }
}

internal float
Win32ProcessXInputStickValue(SHORT value,
                             SHORT deadZoneThreshold)
{
    float result = 0;
    if (value < -deadZoneThreshold)
    {
        result = (value + deadZoneThreshold) / (32768.f - deadZoneThreshold);
    }
    else if (value > deadZoneThreshold)
    {
        result = (value - deadZoneThreshold) / (32767.f - deadZoneThreshold);
    }
    return result;
}

internal void
Win32GetInputRecordingFileLocation(win32_state *state, int slotIndex,
                                   size_t destLength, char *dest)
{
    Assert(slotIndex == 1);
    Win32BuildExecutablePathFileName(state, "loop_input_1.input",
                                     destLength, dest);
}

internal void
Win32BeginRecordingInput(win32_state *state, int inputRecordingIndex)
{
    char fileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputRecordingFileLocation(state, inputRecordingIndex,
                                       sizeof(fileName), fileName);
    
    state->inputRecordingIndex = inputRecordingIndex;
    state->recordingHandle =
        CreateFileA(fileName, GENERIC_WRITE, 0, 0,
                    CREATE_ALWAYS, 0, 0);

    DWORD bytesToWrite = (DWORD)state->appMemorySize;
    DWORD bytesWritten;
    Assert(state->appMemorySize < 0xFFFFFFFF);
    WriteFile(state->recordingHandle, state->appMemoryBlock,
              bytesToWrite, &bytesWritten, 0);    
}

internal void
Win32EndRecordingInput(win32_state *state)
{
    CloseHandle(state->recordingHandle);
    state->inputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayback(win32_state *state, int inputPlaybackIndex)
{
    char fileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputRecordingFileLocation(state, inputPlaybackIndex,
                                       sizeof(fileName), fileName);

    state->inputPlaybackIndex = inputPlaybackIndex;
    state->playbackHandle = CreateFileA(fileName, GENERIC_READ,
                                        FILE_SHARE_READ, 0,
                                        OPEN_EXISTING, 0, 0);
    DWORD bytesToRead = (DWORD)state->appMemorySize;
    DWORD bytesRead;
    ReadFile(state->playbackHandle, state->appMemoryBlock, bytesToRead,
             &bytesRead, 0);
    Assert(bytesRead == bytesToRead);

}

internal void
Win32EndInputPlayback(win32_state *state)
{
    CloseHandle(state->playbackHandle);
    state->inputPlaybackIndex = 0;
}

internal void
Win32RecordInput(win32_state *state, app_input *NewInput)
{
    DWORD bytesWritten;
    WriteFile(state->recordingHandle, NewInput, sizeof(*NewInput),
              &bytesWritten, 0);
}

internal void
win32PlaybackInput(win32_state *state, app_input *NewInput)
{
    DWORD bytesRead;
    if (ReadFile(state->playbackHandle, NewInput, sizeof(*NewInput),
                 &bytesRead, 0))
    {
        if (bytesRead == 0)
        {   
            int playingIndex = state->inputPlaybackIndex;
            Win32EndInputPlayback(state);
            Win32BeginInputPlayback(state, playingIndex);
            ReadFile(state->playbackHandle, NewInput, sizeof(*NewInput),
                     &bytesRead, 0);
        }
    }
}

internal void
Win32MessageLoop(win32_state *state,
                 app_controller_input *oldKeyboardController,
                 app_controller_input *newKeyboardController)
{
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
            case WM_QUIT:
            {
                Running = false;
                break;
            }
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                    
                u32 vKCode = (u32)message.wParam;
                bool32 wasDown = ((message.lParam & (1 << 30)) != 0);
                bool32 isDown = ((message.lParam & (1 << 31)) == 0);
                if (wasDown != isDown)
                {
                    if (vKCode == 'Z')
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->stickUp, isDown);
                    }
                    else if (vKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->stickDown, isDown);
                    }
                    else if (vKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->stickLeft, isDown);
                    }
                    else if (vKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->stickRight, isDown);
                    }
                    else if (vKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->leftShoulder, isDown);
                    }
                    else if (vKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->rightShoulder, isDown);
                    }
                    else if (vKCode == VK_UP)
                    {                            
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->buttonUp, isDown);
                    }
                    else if (vKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->buttonDown, isDown);
                    }
                    else if (vKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->buttonLeft, isDown);
                    }
                    else if (vKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->buttonRight, isDown);
                    }
                    else if (vKCode == VK_ESCAPE)
                    {
                        Running = false;
                    }
                    else if (vKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->start, isDown);
                    }

#if APP_DEV
                    else if (vKCode == 'P')
                    {
                        #if 0
                        if (isDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                        #endif
                    }
                    else if (vKCode == VK_F2)
                    {
                        if (isDown)
                        {
                            if (state->inputRecordingIndex == 0)
                            {
                                Win32BeginRecordingInput(state, 1);
                            }
                            else
                            {
                                Win32EndRecordingInput(state);
                                Win32BeginInputPlayback(state, 1);
                            }
                        }
                    }
#endif
                }
                bool32 altKeyDown = (message.lParam & (1 << 29));
                if (vKCode == VK_F4 && altKeyDown)
                {
                    Running = false;
                }
                break;
            }
            default:
            {                    
                TranslateMessage(&message);
                DispatchMessageA(&message);
                break;
            }
        }
    }
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline float
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    float result = (float)(end.QuadPart - start.QuadPart) /
        (float)GlobalPerCounterFrequency;
    return result;
}

#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

global_variable int Win32OpenGLAttribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#if 0
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#else
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
    0
};

internal void
Win32InitOpenGL(open_gl *OpenGL, HWND Window, HDC WindowDC)
{
    Win32LoadWGLExtensions(OpenGL);
    Win32SetPixelFormat(OpenGL, WindowDC);
    bool32 ModernContext = true;
    HGLRC OpenGLRC = 0;
    if(wglCreateContextAttribsARB)
    {
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }
    
    if(!OpenGLRC)
    {
        ModernContext = false;
        OpenGLRC = wglCreateContext(WindowDC);
    }
    
    if(wglMakeCurrent(WindowDC, OpenGLRC))
    {
        Win32LoadOpenglFunctions(OpenGL);
    }
    
    //TODO https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
    // check for creating opengl context befor calling  glewInit();
    // https://learnopengl.com/Advanced-OpenGL/Framebuffers
    // for frame buffer
    // https://stackoverflow.com/questions/12157646/how-to-render-offscreen-on-opengl
    // to render offscreern
    #if 0
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.dwFlags =
        PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    DesiredPixelFormat.cColorBits = 24;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
    HDC WindowDC = GetDC(Window);
    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
    
    HGLRC context = wglCreateContext(WindowDC);
    if (!wglMakeCurrent(WindowDC, context))
    {
        Assert(0);
    }
    ReleaseDC(Window, WindowDC);
#endif
    
//    const GLubyte *version = glGetString(GL_VERSION);
    
//    GLenum error = glewInit();
//    Assert(error == GLEW_OK);
    #if 0
    Win32LoadOpenGLFunctions();
    #endif

#if 0
    // Screen Frame Buffer
    {
        glGenFramebuffers(1, &AppState->ScreenFboID);
        glBindFramebuffer(GL_FRAMEBUFFER, AppState->ScreenFboID);
        Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            
        glGenTextures(1, &AppState->ScreenFboTexture);
        glBindTexture(GL_TEXTURE_2D, AppState->ScreenFboTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffer->width, buffer->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, AppState->ScreenFboTexture, 0);
    }
#endif
}

inline bool32
Win32VerifyInput(app_input *Input, app_button_state *Button,
                 u32 VirtualKey, u32 TimeToSpam)
{
    if (Button->Pressed ||
        (Button->EndedDown && Input->TimeToTextInput > TimeToSpam))
    {
        Input->TimeToTextInput = 0;
        if (Input->CapsOn)
        {
            Input->TextInput[Input->TextInputCount++] =
                (char)VirtualKey;
        }
        else
        {
            Input->TextInput[Input->TextInputCount++] =
                (char)VirtualKey + 32;
        }
    }
    return Button->EndedDown;
}

struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
};

struct platform_work_queue
{    
    HANDLE SemaphoreHandle;
    
    platform_work_queue_entry Entries[256];    
    u32 volatile CurrentEntryToRead;
    u32 volatile CurrentEntryToWrite;
};

//platform_work_queue *Queue, platform_work_queue_callback *CallBack, void *Data
PLATFORM_ADD_WORK_ENTRY(PlatformAddWorkEntry)
{
    // TODO(zoubir): maybe make this an if stement and do something
    // about it
    // TODO(zoubir): Handle the case where we are out
    // of entries
    u32 NextEntryToWrite = (Queue->CurrentEntryToWrite + 1) %
        ArrayCount(Queue->Entries);
    platform_work_queue_entry *Entry = &Queue->Entries[Queue->CurrentEntryToWrite];
    Entry->Callback = Callback;
    Entry->Data = Data;
    CompletePreviousWritesBeforeFutureWrites;
    Queue->CurrentEntryToWrite = NextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    platform_work_queue *Queue =
        (platform_work_queue *)lpParameter;

    for(;;)
    {
        u32 CurrentEntryToRead = Queue->CurrentEntryToRead;
        u32 CurrentEntryToWrite = Queue->CurrentEntryToWrite;
        u32 NextEntryToRead = (CurrentEntryToRead + 1) %
                ArrayCount(Queue->Entries);
        if (CurrentEntryToWrite != CurrentEntryToRead)
        {
            u32 Index =
                InterlockedCompareExchange((LONG volatile *)&Queue->CurrentEntryToRead,
                                           NextEntryToRead,
                                           CurrentEntryToRead);
            if(Index == CurrentEntryToRead)
            {
                platform_work_queue_entry *Entry = &Queue->Entries[Index];
                Entry->Callback(Entry->Data);
            }

        }
        else
        {            
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }
    
}

internal void
Win32InitWorkQueue(platform_work_queue *Queue, u32 ThreadCount)
{
    Queue->SemaphoreHandle =
        CreateSemaphoreExA(0, 0, ThreadCount,
                          0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ThreadIndex++)
    {
        DWORD ThreadID;
        HANDLE ThreadHandle =
            CreateThread(0, 0, ThreadProc, Queue, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    void *Result = VirtualAlloc(0, Size,
                                MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
    return Result;
}

PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    if (Ptr)
    {
        VirtualFree(Ptr, 0, MEM_RELEASE);
    }
}

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prevInstance,
        LPSTR commandLine,
        int showCode)
{
    platform_work_queue WorkQueue = {};
    Win32InitWorkQueue(&WorkQueue, 2);

    win32_state state = {};

    Win32GetExecutableFileName(&state);

    char appCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildExecutablePathFileName(&state, "app.dll",
                                     sizeof(appCodeDLLFullPath), appCodeDLLFullPath);
    char appCodeTempDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildExecutablePathFileName(&state, "app_temp.dll",
                                     sizeof(appCodeTempDLLFullPath), appCodeTempDLLFullPath);
    
    LARGE_INTEGER perCounterFrequencyResult;
    QueryPerformanceFrequency(&perCounterFrequencyResult);
    GlobalPerCounterFrequency = perCounterFrequencyResult.QuadPart;

    u32 desiredSchedulerMs = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedulerMs) == TIMERR_NOERROR);
    
    Win32LoadXInput();
    WNDCLASSA WindowClass = {};
    
    WindowClass.style =  CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallBack;
    WindowClass.hInstance = instance;
    
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    WindowClass.lpszClassName = "Some Name";

    if (!RegisterClass(&WindowClass))
    {
        return 0;
    }
    HWND windowHandle =
        CreateWindowEx(0,
                       //WS_EX_TOPMOST|WS_EX_LAYERED,
                       WindowClass.lpszClassName,
                       "Zoubir",
                       WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       1280,
                       720,
                       0,
                       0,
                       instance,
                       0);
    if (!windowHandle)
    {
        // TODO logging
        return 0;
    }
#if 0
    SetLayeredWindowAttributes(windowHandle, RGB(0, 0, 0),
                               128, LWA_ALPHA);
#endif

    HDC windowDC = GetDC(windowHandle);
    int monitorRefreshRate = GetDeviceCaps(windowDC, VREFRESH);
    open_gl OpenGL;
    Win32InitOpenGL(&OpenGL, windowHandle, windowDC);
    ReleaseDC(windowHandle, windowDC);
    if (monitorRefreshRate < 1)
    {
        monitorRefreshRate = 60;
    }
    monitorRefreshRate = 30;
    int appUpdateHz = (monitorRefreshRate);
    float targetSecondsPerFrame = 1.f / (float)appUpdateHz;
    
    win32_sound_output soundOutput = {};
    soundOutput.samplesPerSecond = 44100;
    soundOutput.bytesPerSample = sizeof(i16) * 2;
    soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond
        * soundOutput.bytesPerSample;
    soundOutput.safetyBytes =
        (soundOutput.samplesPerSecond *
         soundOutput.bytesPerSample / appUpdateHz);
    soundOutput.tSine = 0;
            
    Win32InitDSound(windowHandle, soundOutput.samplesPerSecond,
                    soundOutput.secondaryBufferSize);
    Win32ClearSoundBuffer(&soundOutput);
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    Running = true;
    
#if 0
            // NOTE(casey): This tests the PlayCursor/WriteCursor update frequency
            // On the Handmade Hero machine, it was 480 samples.
            while(GlobalRunning)
            {
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "PC:%u WC:%u\n", PlayCursor, WriteCursor);
                OutputDebugStringA(TextBuffer);
            }
#endif

    u32 MaxPossibleOverrun = 2 * 4 * sizeof(16);
    i16 *Samples =
       (i16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize +
                              MaxPossibleOverrun,
                              MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
#if APP_DEV
    LPVOID baseAddress = (LPVOID)Terabytes(2);
#else
    LPVOID baseAddress = (LPVOID)0;
#endif
    thread_context thread = {};
    thread.RenderContext.OpenGL = &OpenGL;
    app_memory appMemory = {};
    appMemory.WorkQueue = &WorkQueue;
    appMemory.PermanentStorageSize = Megabytes(64);
    appMemory.TransientStorageSize = Megabytes(8);

    platform_api *PlatformApi = &appMemory.PlatformApi;
    PlatformApi->GetAllFilesOfTypeBegin = Win32GetAllFilesOfTypeBegin;
    PlatformApi->GetAllFilesOfTypeEnd = Win32GetAllFilesOfTypeEnd;
    PlatformApi->OpenNextFile = Win32OpenNextFile;
    PlatformApi->ReadDataFromFile = Win32ReadDataFromFile;
    PlatformApi->FileError = Win32FileError;

    PlatformApi->AllocateMemory = Win32AllocateMemory;
    PlatformApi->DeallocateMemory = Win32DeallocateMemory;
    PlatformApi->ReadEntireFile = DEBUGPlatformReadEntireFile;
    PlatformApi->FreeFileMemory = DEBUGPlatformFreeFileMemory;
    PlatformApi->WriteEntireFile = DEBUGPlatformWriteEntireFile;
    PlatformApi->AddWorkEntry = PlatformAddWorkEntry;

    state.appMemorySize = appMemory.PermanentStorageSize +
        appMemory.TransientStorageSize;
    state.appMemoryBlock = 
        VirtualAlloc(baseAddress, state.appMemorySize,
                     MEM_RESERVE | MEM_COMMIT,
                     PAGE_READWRITE);
    
    appMemory.PermanentStorage = state.appMemoryBlock;
    appMemory.TransientStorage = (u8*)appMemory.PermanentStorage +
        appMemory.PermanentStorageSize;

    for(int replayBufferIndex = 0;
        replayBufferIndex < ArrayCount(state.replayBuffers);
        replayBufferIndex++)
    {
        win32_replay_buffer *replayBuffer =&state.replayBuffers[replayBufferIndex];
        replayBuffer->memoryBlock = VirtualAlloc(0, state.appMemorySize,
                                                 MEM_RESERVE | MEM_COMMIT,
                                                 PAGE_READWRITE);
        Assert(replayBuffer->memoryBlock);
    }

    if (!(Samples && appMemory.PermanentStorage && appMemory.TransientStorage))
    {
        return 0;
    }

    app_input input[2] = {};
    app_input *OldInput = &input[0];
    app_input *NewInput = &input[1];

    win32_debug_time_marker debugTimeMarker[30] = {};
    int debugTimeMarkerIndex = 0;

    DWORD audioLatencyBytes = 0;
    float audioLatencySeconds = 0;
    bool32 soundIsValid = false;


    win32_app_code appCode = Win32LoadAppCode(appCodeDLLFullPath,
                                                 appCodeTempDLLFullPath);

    LARGE_INTEGER lastCounter = Win32GetWallClock();
    LARGE_INTEGER flipWallClock = Win32GetWallClock();
    
    u64 lastCycleCount = __rdtsc(); 
    while(Running)
    {
        win32_window_dimensions WindowDimensions = GetWindowDimension(windowHandle);
        FILETIME newAppDLLWriteTime = Win32GetFileLastWriteTime(appCodeDLLFullPath);
        
        if (CompareFileTime(&newAppDLLWriteTime, &appCode.lastWriteTime) != 0)
        {
            Win32UnloadAppCode(&appCode);
            appCode = Win32LoadAppCode(appCodeDLLFullPath, appCodeTempDLLFullPath);
        }

        app_controller_input *newKeyboardController = GetController(NewInput, 0);
        app_controller_input *oldKeyboardController = GetController(OldInput, 0);
        *newKeyboardController = {};
        for(int buttonIndex = 0;
            buttonIndex < ArrayCount(newKeyboardController->buttons);
            buttonIndex++)
        {
            newKeyboardController->buttons[buttonIndex].EndedDown =
                oldKeyboardController->buttons[buttonIndex].EndedDown;
        }
        Win32MessageLoop(&state,
                         oldKeyboardController,
                         newKeyboardController);
        if (!GlobalPause) //&& !GlobalInactiveApp)
        {
            NewInput->DeltaTime = targetSecondsPerFrame;
            POINT mouseP;
            GetCursorPos(&mouseP);
            ScreenToClient(windowHandle, &mouseP);
            NewInput->MouseX = mouseP.x;
            NewInput->MouseY = mouseP.y;
            NewInput->MouseZ = 0;
            
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[0],
                                        &NewInput->mouseButtons[0],
                                        GetKeyState(VK_LBUTTON) & (1 << 15));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[1],
                                        &NewInput->mouseButtons[1],
                                        GetKeyState(VK_MBUTTON) & (1 << 15));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[2],
                                        &NewInput->mouseButtons[2],
                                        GetKeyState(VK_RBUTTON) & (1 << 15));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[3],
                                        &NewInput->mouseButtons[3],
                                        GetKeyState(VK_XBUTTON1) & (1 << 15));
            Win32ProcessKeyboardMessage(&OldInput->mouseButtons[4],
                                        &NewInput->mouseButtons[4],
                                        GetKeyState(VK_XBUTTON2) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowUp,
                                        &NewInput->ArrowUp,
                                        GetKeyState(VK_UP) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowDown,
                                        &NewInput->ArrowDown,
                                        GetKeyState(VK_DOWN) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowRight,
                                        &NewInput->ArrowRight,
                                        GetKeyState(VK_RIGHT) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->ArrowLeft,
                                        &NewInput->ArrowLeft,
                                        GetKeyState(VK_LEFT) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->CapsButton,
                                        &NewInput->CapsButton,
                                        GetKeyState(VK_CAPITAL) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->BackspaceButton,
                                        &NewInput->BackspaceButton,
                                        GetKeyState(VK_BACK) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->SpaceButton,
                                        &NewInput->SpaceButton,
                                        GetKeyState(VK_SPACE) & (1 << 15));

            Win32ProcessKeyboardMessage(&OldInput->ShiftButton,
                                        &NewInput->ShiftButton,
                                        GetKeyState(VK_SHIFT) & (1 << 15));
            
            Win32ProcessKeyboardMessage(&OldInput->AltButton,
                                        &NewInput->AltButton,
                                        GetKeyState(VK_MENU) & (1 << 15));
            
            for(u32 FButtonIndex = 0;
                FButtonIndex < 12;
                FButtonIndex++)
            {
                Win32ProcessKeyboardMessage(&OldInput->FButtons[FButtonIndex],
                                            &NewInput->FButtons[FButtonIndex],
                                            GetKeyState(VK_F1 + FButtonIndex) & (1 << 15));
            };

            if (NewInput->ButtonF1.Pressed)
            {
                ToggleFullscreen(windowHandle);
            }
            
            //TODO(zoubir): Handle the case where TextInput
            //is full
            //TODO(zoubir): compression
            u32 TimeToSpam = 11;
            int NumberOfKeysDown = 0;
            NewInput->TextInputCount = 0;
            NewInput->TextErase = false;

            BYTE KeyState[256];
            GetKeyboardState((LPBYTE)&KeyState);
            NewInput->CapsOn = KeyState[VK_CAPITAL] & 1;
            
            if (NewInput->BackspaceButton.Pressed)
            {
                NewInput->TimeToTextErase = 0;
                NewInput->TextErase = true;
            }
            
            if (NewInput->TimeToTextErase > TimeToSpam)
            {                
                NewInput->TextErase = true;
            }
            
            if (NewInput->BackspaceButton.EndedDown)
            {
                NewInput->TimeToTextErase =
                    OldInput->TimeToTextErase + 1;
                NumberOfKeysDown++;
            }
            else
            {
                NewInput->TimeToTextErase = 0;
            }

            bool32 OneInputIsDown = false;

            for(int VirtualKey = '0';
                VirtualKey <= '9';
                VirtualKey++)
            {
                int Number = VirtualKey - '0';
                Win32ProcessKeyboardMessage(&OldInput->NumbersButtons[Number],
                                            &NewInput->NumbersButtons[Number],
                                            GetKeyState(VirtualKey) & (1 << 15));
                if (Win32VerifyInput(NewInput,
                                     &NewInput->NumbersButtons[Number],
                                     VirtualKey, TimeToSpam))
                {
                    OneInputIsDown = true;
                    NumberOfKeysDown++;
                }
            }

            for(int VirtualKey = 'A';
                VirtualKey <= 'Z';
                VirtualKey++)
            {
                int Alphabet = VirtualKey - 'A';
                Win32ProcessKeyboardMessage(&OldInput->AlphaButtons[Alphabet],
                                            &NewInput->AlphaButtons[Alphabet],
                                            GetKeyState(VirtualKey) & (1 << 15));
                if (Win32VerifyInput(NewInput,
                                     &NewInput->AlphaButtons[Alphabet],
                                     VirtualKey, TimeToSpam))
                {
                    OneInputIsDown = true;
                    NumberOfKeysDown++;
                }
            }
            
            if (NewInput->SpaceButton.Pressed ||
                (NewInput->SpaceButton.EndedDown &&
                 NewInput->TimeToTextInput > TimeToSpam))
            {
                NewInput->TimeToTextInput = 0;
                NewInput->TextInput[NewInput->TextInputCount++] =
                    ' ';
            }

            NewInput->TextInput[NewInput->TextInputCount] =
                '\0';
                
            if (NewInput->SpaceButton.EndedDown)
            {
                OneInputIsDown = true;
                    NumberOfKeysDown++;
            }

            if (OneInputIsDown)
            {
                NewInput->TimeToTextInput = 
                    OldInput->TimeToTextInput + 1;
            }
            else
            {
                NewInput->TimeToTextInput = 0;
            }

            if (NumberOfKeysDown > 1)
            {
                NewInput->TimeToTextInput = 1;
            }
            

            
            DWORD maxControllerCount = XUSER_MAX_COUNT;
            if (maxControllerCount > ArrayCount(NewInput->controllers) - 1)
            {
                maxControllerCount =  ArrayCount(NewInput->controllers) - 1;
            }
            for(DWORD controllerIndex = 0;
                controllerIndex < maxControllerCount;
                controllerIndex++)
            {
                DWORD myControllerIndex = controllerIndex + 1;
                app_controller_input *oldController = GetController(OldInput, myControllerIndex);
                app_controller_input *newController = GetController(NewInput, myControllerIndex);
                XINPUT_STATE controllerState;
                if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                {
                    newController->isConnected = true;
                    newController->isAnalog = oldController->isAnalog;
                
                    XINPUT_GAMEPAD *pad = &controllerState.Gamepad;


                    newController->stickAverageX = Win32ProcessXInputStickValue(
                        pad->sThumbLX,
                        XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                    newController->stickAverageY = Win32ProcessXInputStickValue(
                        pad->sThumbLY,
                        XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                    if (newController->stickAverageX != 0.f ||
                        newController->stickAverageY != 0.f)
                    {
                        newController->isAnalog = true;
                    }
                
                    if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                    {
                        newController->stickAverageY = 1.f;
                        newController->isAnalog = false;
                    }                
                    if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                    {
                        newController->stickAverageY = -1.f;
                        newController->isAnalog = false;
                    }                
                    if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                    {
                        newController->stickAverageX = -1.f;
                        newController->isAnalog = false;
                    }
                    if ( pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                    {
                        newController->stickAverageX = 1.f;
                        newController->isAnalog = false;
                    }

                    float threshold = 0.5;
                    Win32ProcessXInputDigitalButton(newController->stickAverageX < -threshold,
                                                    &oldController->stickLeft, 1,
                                                    &newController->stickLeft);
                    Win32ProcessXInputDigitalButton(newController->stickAverageX > threshold,
                                                    &oldController->stickRight, 1,
                                                    &newController->stickRight);
                    Win32ProcessXInputDigitalButton(newController->stickAverageY < -threshold,
                                                    &oldController->stickDown, 1,
                                                    &newController->stickDown);
                    Win32ProcessXInputDigitalButton(newController->stickAverageY > threshold,
                                                    &oldController->stickUp, 1,
                                                    &newController->stickUp);
                
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonDown, XINPUT_GAMEPAD_A,
                                                    &newController->buttonDown);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonRight, XINPUT_GAMEPAD_B,
                                                    &newController->buttonRight);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonLeft, XINPUT_GAMEPAD_X,
                                                    &newController->buttonLeft);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->buttonUp, XINPUT_GAMEPAD_Y,
                                                    &newController->buttonUp);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->leftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                    &newController->leftShoulder);
                    Win32ProcessXInputDigitalButton(pad->wButtons,
                                                    &oldController->rightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                    &newController->rightShoulder);
                                                        
                    bool32 start = pad->wButtons & XINPUT_GAMEPAD_START;
                    bool32 back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                }
                else
                {
                    newController->isConnected = false;
                }
            }

            if (state.inputRecordingIndex)
            {
                Win32RecordInput(&state, NewInput);
            }
            
            if (state.inputPlaybackIndex)
            {
                win32PlaybackInput(&state, NewInput); 
            }

            app_window AppWindow;
            AppWindow.Width = WindowDimensions.Width;
            AppWindow.Height = WindowDimensions.Height;
            u64 timer2 = __rdtsc();                        
            appCode.updateAndRender(&thread, &appMemory, NewInput, &AppWindow);
            u64 period2 = __rdtsc() - timer2;                        
            
            LARGE_INTEGER audioWallClock = Win32GetWallClock();
            float fromBeginToAudioSeconds = 1000.f * Win32GetSecondsElapsed(flipWallClock, audioWallClock);
            
            DWORD playCursor = 0;
            DWORD writeCursor = 0;
            if (GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
            {
                if (!soundIsValid)
                {
                    soundOutput.runningSampleIndex =
                        writeCursor / soundOutput.bytesPerSample;
                    soundIsValid = true;
                }

                DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
                    soundOutput.secondaryBufferSize;

                DWORD expectedSoundBytesPerFrame =
                    (soundOutput.samplesPerSecond *
                     soundOutput.bytesPerSample) / appUpdateHz;
                float secondsLeftUntilFlip = (targetSecondsPerFrame - fromBeginToAudioSeconds);
                
                DWORD expectedBytesUntilFlip =
                    (DWORD)((secondsLeftUntilFlip / targetSecondsPerFrame) * (float)expectedSoundBytesPerFrame);
                                                                       
                DWORD expectedFrameBoundaryByte = playCursor + expectedSoundBytesPerFrame;

                DWORD safeWriteCursor = writeCursor;
                if (safeWriteCursor < playCursor)
                {
                    safeWriteCursor += soundOutput.secondaryBufferSize;
                }
                Assert(safeWriteCursor >= playCursor);
                safeWriteCursor += soundOutput.safetyBytes;

                bool32 audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundaryByte;
            
                DWORD targetCursor = 0;
                if (audioCardIsLowLatency)
                {
                    targetCursor = (expectedFrameBoundaryByte + expectedSoundBytesPerFrame);
                }
                else
                {
                    targetCursor = (writeCursor + expectedSoundBytesPerFrame +
                                    soundOutput.safetyBytes);
                }
                targetCursor = targetCursor % soundOutput.secondaryBufferSize;

                DWORD bytesToWrite = 0;
                if (byteToLock > targetCursor)
                {
                    bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                    bytesToWrite += targetCursor;
                }
                else
                {
                    bytesToWrite = targetCursor - byteToLock;
                }
                app_sound_output_buffer soundBuffer;
                soundBuffer.SamplesPerSecond = soundOutput.samplesPerSecond;
                soundBuffer.SampleCount = Align8(bytesToWrite  / soundOutput.bytesPerSample);
                bytesToWrite = soundBuffer.SampleCount * soundOutput.bytesPerSample;
                soundBuffer.Samples = Samples;
                appCode.getSoundSamples(&thread, &appMemory, &soundBuffer);

#if APP_DEV
                win32_debug_time_marker *marker = &debugTimeMarker[debugTimeMarkerIndex];
                marker->outputPlayCursor = playCursor;
                marker->outputWriteCursor = writeCursor;
                marker->outputLocation = byteToLock;
                marker->outputByteCount = bytesToWrite;
                marker->expectedFlipPlayCursor = expectedFrameBoundaryByte;
        
                DWORD unwrappedWriteCursor = writeCursor;
                if (unwrappedWriteCursor < playCursor)
                {
                    unwrappedWriteCursor += soundOutput.secondaryBufferSize;                
                }            
                audioLatencyBytes = unwrappedWriteCursor - playCursor;
                audioLatencySeconds =
                    (float)(audioLatencySeconds / soundOutput.bytesPerSample) /
                    (float)soundOutput.samplesPerSecond;
//                char buffer[256];
//                sprintf_s(buffer, "delta=%d, audioLatency=%f\n", audioLatencyBytes, audioLatencySeconds);
//                OutputDebugStringA(buffer);
#endif
                Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
            }
            else
            {
                soundIsValid = false;
            }

            LARGE_INTEGER workCounter = Win32GetWallClock();
            float workSecondsElapsed = Win32GetSecondsElapsed(lastCounter,
                                                              workCounter);
            float secondsElapsedForFrame = workSecondsElapsed;
            if (secondsElapsedForFrame < targetSecondsPerFrame)
            {
                if (sleepIsGranular)
                {
                    DWORD sleepMs = (DWORD)(1000.f * (targetSecondsPerFrame -
                                                      secondsElapsedForFrame));
                    if (sleepMs > 0)
                    {
                        Sleep(sleepMs);
                    }
                }
                float testSecondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter,
                                                                          Win32GetWallClock());
                if (testSecondsElapsedForFrame < targetSecondsPerFrame)
                {
                    //TODO(zoubir): INSERT LOG HERE
                }
            
                while(secondsElapsedForFrame < targetSecondsPerFrame)
                {
                    secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter,
                                                                    Win32GetWallClock());
                }
            }
            else
            {
                // TODO(zoubir): MISSED FRAME RATE!
                // TODO(zoubir): Logging
            }

            LARGE_INTEGER endCounter = Win32GetWallClock();
            float msPerFrame = 1000.f * Win32GetSecondsElapsed(lastCounter, endCounter);
            lastCounter = endCounter;

            HDC deviceContext = GetDC(windowHandle);
            u64 timer1 = __rdtsc();
            Win32DisplayBufferInWindow(deviceContext);
            u64 period1 = __rdtsc() - timer1;

            ReleaseDC(windowHandle, deviceContext);

            flipWallClock = Win32GetWallClock();

#if APP_DEV
            if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
            {
                Assert(debugTimeMarkerIndex < ArrayCount(debugTimeMarker));

                win32_debug_time_marker *marker = &debugTimeMarker[debugTimeMarkerIndex];
                marker->flipPlayCursor = playCursor;
                marker->flipWriteCursor = writeCursor;        
            }
#endif
                                
            app_input *tmp = OldInput;                
            OldInput = NewInput;
            NewInput = tmp;

            u64 endCycleCount = __rdtsc();
            u64 cyclesElapsed = endCycleCount - lastCycleCount;
            lastCycleCount = endCycleCount;

            float fps = 0.f;//(float)GlobalPerCounterFrequency / (float)counterElapsed;
            float mcpf = (float)((float)cyclesElapsed / 1000.f / 1000.f);
//            char buffer[256];
//            sprintf_s(buffer, "%.2fms/f, %.2ff/s, %.2fmc/f, %lu %lu \n", msPerFrame, fps, mcpf, (unsigned long)period1, (unsigned long)period2);
//            OutputDebugStringA(buffer);
#if APP_DEV
            debugTimeMarkerIndex++;
            if (debugTimeMarkerIndex >= ArrayCount(debugTimeMarker))
            {
                debugTimeMarkerIndex = 0;
            }       
#endif
        }
    }       
                    
    return 0;
}
