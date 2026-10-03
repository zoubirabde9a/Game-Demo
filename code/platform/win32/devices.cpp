/* Devices: loading XInput for gamepads and creating the DirectSound
   buffers. */

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
