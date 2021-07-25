#if !defined(APP_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define APP_PLATFORM_H

//
// NOTE(zoubir): Compilers
//

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !defined(COMPILER_GCC)
#define COMPILER_GCC 0
#endif

#if !defined(__EMSCRIPTEN__)
#define COMPILER_EMSCRIPTEN 0
#endif

#if !COMPILLER_MSVC && !COMPILLER_LLVM && !COMPILER_GCC
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#elif __GNUC__
#undef COMPILER_GCC
#define COMPILER_GCC 0
#elif __EMSCRIPTEN__
#undef COMPILER_EMSCRIPTEN
#define COMPILER_EMSCRIPTEN
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#elif COMPILER_GCC
#include <x86intrin.h>
#elif COMPILER_EMSCRIPTEN
#include <emmintrin.h>
#else
#error SEE: NO optimisations are not available for this compiller
#endif


//#include "GL/glew.h"
#include "app_defs.h"

#include "opengl.h"

#include "math.h"
#include "render.h"
#include "memory.h"

inline u32
safeTruncateU32(u64 value)
{
    Assert(value <= 0xFFFFFFFF);
    return (u32)value;
}

// NOTE(zoubir): Services that the platform layer provides to the app
struct thread_context
{
    int placeHolder;
    render_context RenderContext;
};

struct read_file_result
{
    void *Memory;
    u32 Size;
};

//#define PLATFORM_READ_ENTIRE_FILE(name) read_file_result name(thread_context* Thread, app_memory* Memory, char* FileName)
//typedef PLATFORM_READ_ENTIRE_FILE(platform_read_entire_file);

#if APP_DEV
struct debug_read_file_result
{
    void *Memory;
    u32 Size;
};

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char* fileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *fileName, u32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#else
#endif

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(Name) void Name(void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

#define PLATFORM_ADD_WORK_ENTRY(Name) void Name(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data)
typedef PLATFORM_ADD_WORK_ENTRY(platform_add_work_entry);

struct app_window
{
    int Width;
    int Height;
};

struct app_sound_output_buffer
{
    int SamplesPerSecond;
    int SampleCount;

    //IMPORTANT Samples must be padded to a multiple of 4
    i16* Samples;    
};

struct app_button_state
{
    int halfTransitionCount;
    bool32 Pressed;
    bool32 EndedDown;
    bool32 Released;
};

struct app_controller_input
{
    bool32 isConnected;
    bool32 isAnalog;

    float stickAverageX;
    float stickAverageY;

    union
    {
        app_button_state buttons[12];
        struct
        {
            app_button_state stickUp;
            app_button_state stickDown;
            app_button_state stickLeft;
            app_button_state stickRight;

            app_button_state buttonUp;
            app_button_state buttonDown;
            app_button_state buttonLeft;
            app_button_state buttonRight;

            app_button_state leftShoulder;
            app_button_state rightShoulder;

            app_button_state start;
            app_button_state back;

            // NOTE(zoubir): all buttons hould be inserted above this
            app_button_state terminator;
        };
    };
};

//TODO(zoubir): mb to big for stack
struct app_input
{
    float DeltaTime;
    union
    {   
        app_button_state mouseButtons[5];
        struct
        {
            app_button_state LeftButton;
            app_button_state MidButton;
            app_button_state RightButton;
            app_button_state xButton1;
            app_button_state xButton2;
        };
    };
    i32 MouseX, MouseY, MouseZ;
    
    app_controller_input controllers[5];

    app_button_state ArrowUp;
    app_button_state ArrowDown;
    app_button_state ArrowRight;
    app_button_state ArrowLeft;
    
    app_button_state CapsButton;
    app_button_state BackspaceButton;
    app_button_state SpaceButton;
    app_button_state ShiftButton;
    app_button_state AltButton;
    union
    {
        app_button_state FButtons[12];
        struct
        {
            app_button_state ButtonF1;
            app_button_state ButtonF2;
            app_button_state ButtonF3;
            app_button_state ButtonF4;
            app_button_state ButtonF5;
            app_button_state ButtonF6;
            app_button_state ButtonF7;
            app_button_state ButtonF8;
            app_button_state ButtonF9;
            app_button_state ButtonF10;
            app_button_state ButtonF11;
            app_button_state ButtonF12;
        };
    };
    union
    {
        app_button_state NumbersButtons[10];
        struct
        {
            app_button_state Button0;
            app_button_state Button1;
            app_button_state Button2;
            app_button_state Button3;
            app_button_state Button4;
            app_button_state Button5;
            app_button_state Button6;
            app_button_state Button7;
            app_button_state Button8;
            app_button_state Button9;
        };
    };
    union
    {
        app_button_state AlphaButtons[26];
        struct
        {
            app_button_state ButtonA;
            app_button_state ButtonB;
            app_button_state ButtonC;
            app_button_state ButtonD;
            app_button_state ButtonE;
            app_button_state ButtonF;
            app_button_state ButtonG;
            app_button_state ButtonH;
            app_button_state ButtonI;
            app_button_state ButtonJ;
            app_button_state ButtonK;
            app_button_state ButtonL;
            app_button_state ButtonM;
            app_button_state ButtonN;
            app_button_state ButtonO;
            app_button_state ButtonP;
            app_button_state ButtonQ;
            app_button_state ButtonR;
            app_button_state ButtonS;
            app_button_state ButtonT;
            app_button_state ButtonU;
            app_button_state ButtonV;
            app_button_state ButtonW;
            app_button_state ButtonX;
            app_button_state ButtonY;
            app_button_state ButtonZ;
        };
    };

    bool32 CapsOn;
    bool32 TextErase;
    char TextInput[64];
    u32 TextInputCount;
    u32 TimeToTextInput;
    u32 TimeToTextErase;
};

inline app_controller_input *GetController(app_input* input, int controllerIndex)
{
    Assert(controllerIndex < ArrayCount(input->controllers));

    app_controller_input *result = &input->controllers[0];
    return result;
}

struct platform_file_handle
{
    bool32 HasErrors;
};

struct platform_file_group
{
    u32 FileCount;
};

#define PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(Name) platform_file_group *Name(char *Type)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(platform_get_all_files_of_type_begin);

#define PLATFORM_GET_ALL_FILES_OF_TYPE_END(Name) void Name(platform_file_group *FileGroup)
typedef PLATFORM_GET_ALL_FILES_OF_TYPE_END(platform_get_all_files_of_type_end);

#define PLATFORM_OPEN_NEXT_FILE(Name) platform_file_handle *Name(platform_file_group *FileGroup)
typedef PLATFORM_OPEN_NEXT_FILE(platform_open_next_file);

#define PLATFORM_READ_DATA_FROM_FILE(Name) void Name(platform_file_handle *Source, u64 Offset, u64 Size, void *Dest)
typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);

#define PLATFORM_FILE_ERROR(Name) void Name(platform_file_handle *Handle, char *Error)
typedef PLATFORM_FILE_ERROR(platform_file_error);

inline bool32 PlatformNoFileErrors(platform_file_handle *Handle)
{
    bool32 Result = !Handle->HasErrors;
    return Result;
}

#define PLATFORM_ALLOCATE_MEMORY(Name) void *Name(memory_index Size)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(Name) void Name(void *Ptr)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

struct platform_api
{    

    platform_get_all_files_of_type_begin *GetAllFilesOfTypeBegin;
    platform_get_all_files_of_type_end *GetAllFilesOfTypeEnd;
    platform_open_next_file *OpenNextFile;
    platform_read_data_from_file *ReadDataFromFile;
    platform_file_error *FileError;

    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
    
    debug_platform_read_entire_file *ReadEntireFile;
    debug_platform_free_file_memory *FreeFileMemory;
    debug_platform_write_entire_file *WriteEntireFile;
    platform_add_work_entry *AddWorkEntry;
};


struct app_memory
{
    u32 PermanentStorageSize;
    void *PermanentStorage; // NOTE(zoubir): Required to be set to 0 at startup
    u32 PermanentStorageOffset;
    
    size_t TransientStorageSize;
    size_t TransientStorageOffset;
    void *TransientStorage;  // NOTE(zoubir): Required to be set to 0 at startup


    platform_api PlatformApi;

    platform_work_queue *WorkQueue;
};


#define APP_UPDATE_AND_RENDER(name)void name(thread_context *Thread, app_memory *Memory, app_input *Input, app_window *Window)
typedef APP_UPDATE_AND_RENDER(app_update_and_render);
internal APP_UPDATE_AND_RENDER(AppUpdateAndRenderStub)
{
}

//NOTE has to be a very fast function
#define APP_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, app_memory *Memory, app_sound_output_buffer *SoundBuffer)
typedef APP_GET_SOUND_SAMPLES(app_get_sound_samples);
internal APP_GET_SOUND_SAMPLES(AppGetSoundSamplesStub)
{
}


#endif
