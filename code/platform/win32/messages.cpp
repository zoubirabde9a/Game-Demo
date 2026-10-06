/* Windows messages: characters typed this frame (WM_CHAR) and the
   message loop that turns key messages into input. */

// NOTE(zoubir): characters typed since the last frame, from WM_CHAR;
// printable ASCII only, and backspace as an erase
global_variable char GlobalTypedText[64];
global_variable u32 GlobalTypedCount;
global_variable bool32 GlobalTypedErase;

internal void
Win32AddTypedCharacter(u32 Character)
{
    if (Character == '')
    {
        GlobalTypedErase = true;
    }
    else if (Character >= 32 && Character < 127 &&
             GlobalTypedCount + 1 < ArrayCount(GlobalTypedText))
    {
        GlobalTypedText[GlobalTypedCount++] = (char)Character;
    }
}

internal void
Win32TakeTypedText(app_input *Input)
{
    Input->TextInputCount = 0;
    for(u32 Index = 0;
        Index < GlobalTypedCount &&
        Input->TextInputCount + 1 < ArrayCount(Input->TextInput);
        Index++)
    {
        Input->TextInput[Input->TextInputCount++] = GlobalTypedText[Index];
    }
    Input->TextInput[Input->TextInputCount] = 0;
    Input->TextErase = GlobalTypedErase;
    GlobalTypedCount = 0;
    GlobalTypedErase = false;
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
            case WM_CHAR:
            {
                Win32AddTypedCharacter((u32)message.wParam);
                break;
            }
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                // NOTE(zoubir): posts the WM_CHAR this loop picks up next;
                // Alt combinations stay untranslated (no WM_SYSCHAR beep)
                if (message.message == WM_KEYDOWN)
                {
                    TranslateMessage(&message);
                }
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
                    else if (vKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&oldKeyboardController->stickUp,
                                                    &newKeyboardController->start, isDown);
                    }

#if APP_DEV
                    else if (vKCode == 'P')
                    {
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
                // NOTE(zoubir): Alt+Enter, the usual fullscreen key; on the
                // first press only, not on key repeat
                if (vKCode == VK_RETURN && altKeyDown &&
                    message.message == WM_SYSKEYDOWN && !wasDown)
                {
                    ToggleFullscreen(message.hwnd);
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
