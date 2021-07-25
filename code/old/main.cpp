#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define PI32 3.14159265359f
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef int32_t bool32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct win32_offscreen_buffer
{
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int pitch;
};

global_variable bool Running; 
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

struct win32_window_dimension
{
    int width;
    int height;
};

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

HRESULT WINAPI
DirectSoundCreate(LPCGUID pcGuidDevice,
                  LPDIRECTSOUND *ppDS,
                  LPUNKNOWN pUnkOuter);

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
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                // NOTE(zoubir): "Create" a primary buffer
                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    HRESULT error = primaryBuffer->SetFormat(&waveFormat);
                    if (SUCCEEDED(error))
                    {
                        OutputDebugStringA("primary buffer was set\n");
                    }
                    else
                    {
                    }
                }
                else
                {
                }
            }
            else
            {
            }           
            // NOTE(zoubir): "Create" a seconday buffer
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            HRESULT error = directSound->CreateSoundBuffer(&bufferDescription, &GlobalSecondaryBuffer, 0);
            if (SUCCEEDED(error))
            {
                // NOTE(zoubir): Start
                OutputDebugStringA("secondary buffer was created\n");
            }
            
            
        }
        else
        {
            
        }
    }
}

internal win32_window_dimension
GetWindowDimension(HWND window)
{
    win32_window_dimension result;    
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.width = clientRect.right - clientRect.left;
    result. height = clientRect.bottom - clientRect.top;
    return result;
}

internal void
RenderWeirdGradiant(win32_offscreen_buffer *buffer, int xOffset, int yOffset)
{
    u8* row = (u8*)buffer->memory;
    for(int y = 0; y < buffer->height; y++)
    {
        u32* pixel = (u32*)row;
        for(int x = 0; x < buffer->width; x++)
        {
            // BGRX
            u8 r = x + xOffset;
            u8 g = y + yOffset;
            u8 b = 0;
            *pixel++ = b | (g << 8) | (r << 16);
        }   
        row += buffer->pitch; 
    }
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer* buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }    
    buffer->width = width;
    buffer->height = height;
    int bytesPerPixel = 4;    
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;
    int bitmapMemorySize = buffer->width * buffer->height * bytesPerPixel;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * bytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(HDC deviceContext,
                           int windowWidth, int windowHeight,
                           win32_offscreen_buffer *buffer)
{
    StretchDIBits(deviceContext,
                  0, 0, windowWidth, windowHeight,
                  0, 0, buffer->width, buffer->
                  height,
                  buffer->memory,
                  &buffer->info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
MainWindowCallBack(
    _In_ HWND   window,
    _In_ UINT   message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)

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
            u32 vKCode = wParam;
            bool32 wasDown = ((lParam & (1 << 30)) != 0);
            bool32 isDown = ((lParam & (1 << 31)) == 0);
            if (wasDown != isDown)
            {
                if (vKCode == 'Z')
                {
                }
                else if (vKCode == 'S')
                {
                }
                else if (vKCode == 'Q')
                {
                }
                else if (vKCode == 'D')
                {
                }
                else if (vKCode == 'A')
                {
                }
                else if (vKCode == 'E')
                {
                }
                else if (vKCode == VK_UP)
                {
                }
                else if (vKCode == VK_DOWN)
                {
                }
                else if (vKCode == VK_LEFT)
                {
                }
                else if (vKCode == VK_RIGHT)
                {
                }
                else if (vKCode == VK_ESCAPE)
                {
                    OutputDebugStringA("ESCAPE: \n");
                    if (isDown)
                    {                     
                        OutputDebugStringA("isDown ");
                    }
                    if (wasDown)
                    {
                        OutputDebugStringA("wasDown ");
                    }
                    OutputDebugStringA("\n");
                }
                else if (vKCode == VK_SPACE)
                {
                }
            }
            bool32 altKeyDown = (lParam & (1 << 29));
            if (vKCode == VK_F4 && altKeyDown)
            {
                Running = false;
            }
            break;
        }
        
        case WM_ACTIVATEAPP:
        {
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

            win32_window_dimension dimension = GetWindowDimension(window);
            Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height,
                                       &GlobalBackBuffer);
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

struct win32_sound_output
{
    int samplesPerSecond;
    int toneHz;
    i16 toneVolume;
    u32 runningSampleIndex;
    int wavePeriod;
    int bytesPerSample;
    int secondaryBufferSize;
    float tSine;
    int latencySampleCount;
};

internal void
Win32FillSoundBuffer(win32_sound_output *soundOutput, DWORD byteToLock, DWORD bytesToWrite)
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
        i16* sampleOut = (i16 *)region1;
        for(DWORD sampleIndex = 0;
            sampleIndex < region1SampleCount;
            sampleIndex++)
        {
            float sineValue = sinf(soundOutput->tSine);
            i16 sampleValue = (i16)(sineValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            soundOutput->runningSampleIndex++;
            soundOutput->tSine += 2.f * PI32 * 1.f / (float)soundOutput->wavePeriod;
        }
                         
        DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample;
        sampleOut = (i16 *)region2;
        for(DWORD sampleIndex = 0;
            sampleIndex < region2SampleCount;
            sampleIndex++)
        {
            float sineValue = sinf(soundOutput->tSine);
            i16 sampleValue = (i16)(sineValue * soundOutput->toneVolume);
            *sampleOut++ = sampleValue;
            *sampleOut++ = sampleValue;
            soundOutput->runningSampleIndex++;
            soundOutput->tSine += 2.f * PI32 * 1.f / (float)soundOutput->wavePeriod;
        }
    }
    GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
}
int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prevInstance,
        LPSTR commandLine,
        int showCode)
{
    LARGE_INTEGER perCounterFrequencyResult;
    QueryPerformanceFrequency(&perCounterFrequencyResult);
    i64 perCounterFrequency = perCounterFrequencyResult.QuadPart;

    Win32LoadXInput();
    WNDCLASSA WindowClass = {};
    Win32ResizeDIBSection(&GlobalBackBuffer,
                          1280, 720);

    
    WindowClass.style =  CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallBack;
    WindowClass.hInstance = instance;
    WindowClass.lpszClassName = "Some Name";
    
    if (RegisterClass(&WindowClass))
    {
        HWND windowHandle =
            CreateWindowEx(0,
                           WindowClass.lpszClassName,
                           "Zoubir",
                           WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           0,
                           0,
                           instance,
                           0);

        if (windowHandle)
        {
            // NOTE(zoubir): Graphics test
            int xOffset = 0;
            int yOffset = 0;

            //NOTE(zoubir): Sound Test
            HDC deviceContext = GetDC(windowHandle);

            win32_sound_output soundOutput;
            soundOutput.samplesPerSecond = 48000;
            soundOutput.toneHz = 256;
            soundOutput.toneVolume = 6000;
            soundOutput.runningSampleIndex = 0;
            soundOutput.wavePeriod = soundOutput.samplesPerSecond / soundOutput.toneHz;
            soundOutput.bytesPerSample = sizeof(i16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
            soundOutput.latencySampleCount = soundOutput.samplesPerSecond / 15;
            soundOutput.tSine = 0;
            
            Win32InitDSound(windowHandle, soundOutput.samplesPerSecond, soundOutput.secondaryBufferSize);
            Win32FillSoundBuffer(&soundOutput, 0, soundOutput.latencySampleCount * soundOutput.bytesPerSample);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            LARGE_INTEGER lastCounter;
            QueryPerformanceCounter(&lastCounter);
            u64 lastCycleCount = __rdtsc();            
            
            Running = true;
            while(Running)
            {
                MSG message;
                while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessageA(&message);
                }

                for(DWORD i = 0; i < XUSER_MAX_COUNT; i++)
                {
                    XINPUT_STATE controllerState;
                    if (XInputGetState(i, &controllerState) == ERROR_SUCCESS)
                    {
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        bool32 up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        bool32 down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        bool32 left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        bool32 right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        bool32 start = pad->wButtons & XINPUT_GAMEPAD_START;
                        bool32 back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                        bool32 leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        bool32 rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        bool32 aButton = pad->wButtons & XINPUT_GAMEPAD_A;
                        bool32 bButton = pad->wButtons & XINPUT_GAMEPAD_B;
                        bool32 xButton = pad->wButtons & XINPUT_GAMEPAD_X;
                        bool32 yButton = pad->wButtons & XINPUT_GAMEPAD_Y;

                        i16 stickX = pad->sThumbLX;
                        i16 stickY = pad->sThumbLY;
                    }
                    else
                    {
                    }
                }
                RenderWeirdGradiant(&GlobalBackBuffer, xOffset, yOffset);

                // NOTE(zoubir): DirectSound output test
                DWORD playCursor;
                DWORD writeCursor;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                {
                    DWORD byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
                        soundOutput.secondaryBufferSize;
                    DWORD targetCursor = (playCursor +
                                          (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) %
                        soundOutput.secondaryBufferSize;
                    DWORD bytesToWrite;
                    if (byteToLock > targetCursor)
                    {
                        bytesToWrite = soundOutput.secondaryBufferSize - byteToLock;
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - byteToLock;
                    }
                    Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
                }    
                
                HDC deviceContext = GetDC(windowHandle);
                win32_window_dimension dimension = GetWindowDimension(windowHandle);
                Win32DisplayBufferInWindow(deviceContext,
                                           dimension.width, dimension.height,
                                           &GlobalBackBuffer);
                
                
                xOffset++;
                yOffset++;

                u64 endCycleCount = __rdtsc();

                LARGE_INTEGER endCounter;
                QueryPerformanceCounter(&endCounter);

                u64 cyclesElapsed = endCycleCount - lastCycleCount;
                i64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
                float msPerFrame = (float)(1000.f * (float)counterElapsed / (float)perCounterFrequency);
                float fps = (float)perCounterFrequency / (float)counterElapsed;
                float mcpf = (float)((float)cyclesElapsed / 1000.f / 1000.f);

                char buffer[256];
                sprintf(buffer, "%.2fms/f, %.2ff/s, %.2fmc/f\n", msPerFrame, fps, mcpf);
                OutputDebugStringA(buffer);
                
                lastCounter = endCounter;
                lastCycleCount = endCycleCount;
            }       
            
        }
        else
        {
            //TODO logging
        }
    }
    else
    { 
       //TODO logging
    }
        
}
