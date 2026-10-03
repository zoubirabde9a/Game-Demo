/* Input: gamepad buttons and sticks, keyboard state, and recording and
   playing back input (the F-key loop in developer builds). */

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
