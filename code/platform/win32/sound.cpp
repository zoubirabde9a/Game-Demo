/* Sound output: clearing and filling the DirectSound ring buffer. */

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
