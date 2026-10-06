/* Frame: the steps WinMain runs every frame. Reloading app.dll when it
   changes, reading keyboard, mouse and gamepads, writing the frame's
   sound, waiting out the rest of the frame, and presenting it. */

internal void
Win32ReloadAppCodeIfChanged(win32_app_code *appCode, char *dllPath,
                            char *tempDLLPath)
{
    FILETIME newAppDLLWriteTime = Win32GetFileLastWriteTime(dllPath);

    if (CompareFileTime(&newAppDLLWriteTime, &appCode->lastWriteTime) != 0)
    {
        Win32UnloadAppCode(appCode);
        *appCode = Win32LoadAppCode(dllPath, tempDLLPath);
    }
}

// NOTE(zoubir): keyboard controller buttons stay down from the last frame
// unless a message says otherwise
internal void
Win32CarryKeyboardController(app_input *OldInput, app_input *NewInput)
{
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
}

internal void
Win32ReadMessages(win32_state *state, app_input *OldInput, app_input *NewInput)
{
    Win32CarryKeyboardController(OldInput, NewInput);
    Win32MessageLoop(state,
                     GetController(OldInput, 0),
                     GetController(NewInput, 0));
}

// NOTE(zoubir): mouse position and buttons, then every key the game reads.
// F1 and F11 toggle fullscreen here (Alt+Enter in Win32MessageLoop). The
// mouse is in the game's 96-DPI pixels: screen pixels divided by Scale.
internal void
Win32PollKeyboardAndMouse(HWND windowHandle, app_input *OldInput,
                          app_input *NewInput, float Scale)
{
    POINT mouseP;
    GetCursorPos(&mouseP);
    ScreenToClient(windowHandle, &mouseP);
    RECT clientRect;
    GetClientRect(windowHandle, &clientRect);
    GlobalHasInputFocus = (GetForegroundWindow() == windowHandle);
    GlobalMouseInClient = (mouseP.x >= clientRect.left &&
                           mouseP.x < clientRect.right &&
                           mouseP.y >= clientRect.top &&
                           mouseP.y < clientRect.bottom);
    NewInput->MouseX = (i32)floorf((float)mouseP.x / Scale);
    NewInput->MouseY = (i32)floorf((float)mouseP.y / Scale);
    NewInput->MouseZ = 0;

    Win32ProcessKeyboardMessage(&OldInput->mouseButtons[0],
                                &NewInput->mouseButtons[0],
                                Win32MouseDown(VK_LBUTTON));
    Win32ProcessKeyboardMessage(&OldInput->mouseButtons[1],
                                &NewInput->mouseButtons[1],
                                Win32MouseDown(VK_MBUTTON));
    Win32ProcessKeyboardMessage(&OldInput->mouseButtons[2],
                                &NewInput->mouseButtons[2],
                                Win32MouseDown(VK_RBUTTON));
    Win32ProcessKeyboardMessage(&OldInput->mouseButtons[3],
                                &NewInput->mouseButtons[3],
                                Win32MouseDown(VK_XBUTTON1));
    Win32ProcessKeyboardMessage(&OldInput->mouseButtons[4],
                                &NewInput->mouseButtons[4],
                                Win32MouseDown(VK_XBUTTON2));

    Win32ProcessKeyboardMessage(&OldInput->ArrowUp,
                                &NewInput->ArrowUp,
                                Win32KeyDown(VK_UP));

    Win32ProcessKeyboardMessage(&OldInput->ArrowDown,
                                &NewInput->ArrowDown,
                                Win32KeyDown(VK_DOWN));

    Win32ProcessKeyboardMessage(&OldInput->ArrowRight,
                                &NewInput->ArrowRight,
                                Win32KeyDown(VK_RIGHT));

    Win32ProcessKeyboardMessage(&OldInput->ArrowLeft,
                                &NewInput->ArrowLeft,
                                Win32KeyDown(VK_LEFT));

    Win32ProcessKeyboardMessage(&OldInput->CapsButton,
                                &NewInput->CapsButton,
                                Win32KeyDown(VK_CAPITAL));

    Win32ProcessKeyboardMessage(&OldInput->BackspaceButton,
                                &NewInput->BackspaceButton,
                                Win32KeyDown(VK_BACK));

    Win32ProcessKeyboardMessage(&OldInput->SpaceButton,
                                &NewInput->SpaceButton,
                                Win32KeyDown(VK_SPACE));

    Win32ProcessKeyboardMessage(&OldInput->ShiftButton,
                                &NewInput->ShiftButton,
                                Win32KeyDown(VK_SHIFT));

    Win32ProcessKeyboardMessage(&OldInput->AltButton,
                                &NewInput->AltButton,
                                Win32KeyDown(VK_MENU));

    Win32ProcessKeyboardMessage(&OldInput->TabButton,
                                &NewInput->TabButton,
                                Win32KeyDown(VK_TAB));

    Win32ProcessKeyboardMessage(&OldInput->EscapeButton,
                                &NewInput->EscapeButton,
                                Win32KeyDown(VK_ESCAPE));

    for(u32 FButtonIndex = 0;
        FButtonIndex < 12;
        FButtonIndex++)
    {
        Win32ProcessKeyboardMessage(&OldInput->FButtons[FButtonIndex],
                                    &NewInput->FButtons[FButtonIndex],
                                    Win32KeyDown(VK_F1 + FButtonIndex));
    };

    if (NewInput->ButtonF1.Pressed || NewInput->ButtonF11.Pressed)
    {
        ToggleFullscreen(windowHandle);
    }

    for(int VirtualKey = '0';
        VirtualKey <= '9';
        VirtualKey++)
    {
        int Number = VirtualKey - '0';
        Win32ProcessKeyboardMessage(&OldInput->NumbersButtons[Number],
                                    &NewInput->NumbersButtons[Number],
                                    Win32KeyDown(VirtualKey));
    }

    for(int VirtualKey = 'A';
        VirtualKey <= 'Z';
        VirtualKey++)
    {
        int Alphabet = VirtualKey - 'A';
        Win32ProcessKeyboardMessage(&OldInput->AlphaButtons[Alphabet],
                                    &NewInput->AlphaButtons[Alphabet],
                                    Win32KeyDown(VirtualKey));
    }

    // NOTE(zoubir): typed text comes from WM_CHAR (Win32MessageLoop),
    // so it follows the keyboard layout, shift and key repeat
    Win32TakeTypedText(NewInput);
}

// NOTE(zoubir): XInput pads fill controllers 1 and up; controller 0 is the
// keyboard
internal void
Win32PollGamepads(app_input *OldInput, app_input *NewInput)
{
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
        }
        else
        {
            newController->isConnected = false;
        }
    }
}

// NOTE(zoubir): in developer builds the F-keys record and replay input
internal void
Win32RecordOrPlayBackInput(win32_state *state, app_input *NewInput)
{
    if (state->inputRecordingIndex)
    {
        Win32RecordInput(state, NewInput);
    }

    if (state->inputPlaybackIndex)
    {
        win32PlaybackInput(state, NewInput);
    }
}

// NOTE(zoubir): asks the game for enough samples to fill the ring buffer up
// to the next frame boundary (or past the write cursor and a safety margin
// on a high-latency card) and copies them in. *soundIsValid is cleared when
// DirectSound cannot report its cursors, so the next good frame restarts
// writing from the write cursor.
internal void
Win32WriteFrameSound(win32_sound_output *soundOutput, bool32 *soundIsValid,
                     i16 *Samples, LARGE_INTEGER flipWallClock,
                     float targetSecondsPerFrame, int appUpdateHz,
                     win32_app_code *appCode, thread_context *thread,
                     app_memory *appMemory)
{
    LARGE_INTEGER audioWallClock = Win32GetWallClock();
    // NOTE(zoubir): despite its name this is in milliseconds
    float fromBeginToAudioSeconds = 1000.f * Win32GetSecondsElapsed(flipWallClock, audioWallClock);

    DWORD playCursor = 0;
    DWORD writeCursor = 0;
    if (GlobalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
    {
        if (!*soundIsValid)
        {
            soundOutput->runningSampleIndex =
                writeCursor / soundOutput->bytesPerSample;
            *soundIsValid = true;
        }

        DWORD byteToLock = (soundOutput->runningSampleIndex * soundOutput->bytesPerSample) %
            soundOutput->secondaryBufferSize;

        DWORD expectedSoundBytesPerFrame =
            (soundOutput->samplesPerSecond *
             soundOutput->bytesPerSample) / appUpdateHz;
        float secondsLeftUntilFlip = (targetSecondsPerFrame - fromBeginToAudioSeconds);

        DWORD expectedBytesUntilFlip =
            (DWORD)((secondsLeftUntilFlip / targetSecondsPerFrame) * (float)expectedSoundBytesPerFrame);

        DWORD expectedFrameBoundaryByte = playCursor + expectedSoundBytesPerFrame;

        DWORD safeWriteCursor = writeCursor;
        if (safeWriteCursor < playCursor)
        {
            safeWriteCursor += soundOutput->secondaryBufferSize;
        }
        Assert(safeWriteCursor >= playCursor);
        safeWriteCursor += soundOutput->safetyBytes;

        bool32 audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundaryByte;

        DWORD targetCursor = 0;
        if (audioCardIsLowLatency)
        {
            targetCursor = (expectedFrameBoundaryByte + expectedSoundBytesPerFrame);
        }
        else
        {
            targetCursor = (writeCursor + expectedSoundBytesPerFrame +
                            soundOutput->safetyBytes);
        }
        targetCursor = targetCursor % soundOutput->secondaryBufferSize;

        DWORD bytesToWrite = 0;
        if (byteToLock > targetCursor)
        {
            bytesToWrite = soundOutput->secondaryBufferSize - byteToLock;
            bytesToWrite += targetCursor;
        }
        else
        {
            bytesToWrite = targetCursor - byteToLock;
        }
        app_sound_output_buffer soundBuffer;
        soundBuffer.SamplesPerSecond = soundOutput->samplesPerSecond;
        soundBuffer.SampleCount = Align8(bytesToWrite  / soundOutput->bytesPerSample);
        bytesToWrite = soundBuffer.SampleCount * soundOutput->bytesPerSample;
        soundBuffer.Samples = Samples;
        appCode->getSoundSamples(thread, appMemory, &soundBuffer);

        Win32FillSoundBuffer(soundOutput, byteToLock, bytesToWrite, &soundBuffer);
    }
    else
    {
        *soundIsValid = false;
    }
}

// NOTE(zoubir): sleeps, then spins, until the frame that began at
// *lastCounter has lasted targetSecondsPerFrame, and starts the next one
internal void
Win32WaitForFrameEnd(LARGE_INTEGER *lastCounter, float targetSecondsPerFrame,
                     bool32 sleepIsGranular)
{
    LARGE_INTEGER workCounter = Win32GetWallClock();
    float workSecondsElapsed = Win32GetSecondsElapsed(*lastCounter,
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

        while(secondsElapsedForFrame < targetSecondsPerFrame)
        {
            secondsElapsedForFrame = Win32GetSecondsElapsed(*lastCounter,
                                                            Win32GetWallClock());
        }
    }
    else
    {
        // TODO(zoubir): MISSED FRAME RATE!
        // TODO(zoubir): Logging
    }

    *lastCounter = Win32GetWallClock();
}

internal void
Win32PresentFrame(HWND windowHandle)
{
    HDC deviceContext = GetDC(windowHandle);
    Win32DisplayBufferInWindow(deviceContext);
    ReleaseDC(windowHandle, deviceContext);
}
