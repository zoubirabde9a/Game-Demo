/* Process-wide state of the Windows layer: input focus, the run and pause
   flags, the sound buffer, the timer frequency, the saved window
   placement, and stubs for XInput and DirectSound until they load. */

// NOTE(zoubir): input only counts while the game window is in front.
// GetKeyState reports keys pressed in other windows too, and a key held
// while switching away would otherwise stay down forever.
global_variable bool32 GlobalHasInputFocus;
global_variable bool32 GlobalMouseInClient;

inline bool32
Win32KeyDown(int VirtualKey)
{
    bool32 Result = GlobalHasInputFocus &&
        (GetKeyState(VirtualKey) & (1 << 15));
    return Result;
}

inline bool32
Win32MouseDown(int VirtualKey)
{
    bool32 Result = GlobalMouseInClient && Win32KeyDown(VirtualKey);
    return Result;
}
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
