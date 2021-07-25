/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */
#include "SDL\SDL.h"
//#undef main
#include <emscripten\emscripten.h>
#include <GLES3/gl3.h>
#include "app_platform.h"

#include "app.cpp"


DEBUG_PLATFORM_READ_ENTIRE_FILE(WebReadEntireFile)
{
    debug_read_file_result Result = {};

    FILE *File = fopen(fileName, "rb");
    if (File)
    {
        fseek(File, 0l, SEEK_END);
        u32 Size = ftell(File);
        fseek(File, 0l, SEEK_SET);
        void *Memory = malloc(Size);
        fread(Memory, Size, 1, File);

        Result.Memory = Memory;
        Result.Size = Size;        
    }
    
    return Result;
}
DEBUG_PLATFORM_FREE_FILE_MEMORY(WebFreeFileMemory)
{
    free(memory);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(WebWriteEntireFile)
{    
    bool32 Result = 0;

    FILE *File = fopen(fileName, "wb+");
    if (File)
    {
        Result = (fwrite(memory, memorySize, 1, File) == memorySize);
    }
    
    return Result;
}

PLATFORM_ADD_WORK_ENTRY(WebAddWorkEntry)
{
    
}

inline void *
WebAllocateSize(u32 Size)
{
    void *Result = calloc(1, Size);
    return Result;
}

#define WebAllocateStruct(Type) (Type *)(WebAllocateSize(sizeof(Type)))

inline void
WebFree(void *Memory)
{
    free(Memory);
}

struct web_platform_file_handle
{
    platform_file_handle H;
    FILE *Handle;
};

struct web_platform_file_group
{
    platform_file_group H;
};

internal
PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(WebGetAllFilesOfTypeBegin)
{
    web_platform_file_group *WebFileGroup =
        WebAllocateStruct(web_platform_file_group);
    WebFileGroup->H.FileCount = 1;
    
    return (platform_file_group *)WebFileGroup;    
}
    
internal
PLATFORM_GET_ALL_FILES_OF_TYPE_END(WebGetAllFilesOfTypeEnd)
{
    
    web_platform_file_group *WebFileGroup =
        (web_platform_file_group *)(FileGroup);
    if(WebFileGroup)
    {
        WebFree(WebFileGroup);
    }
}

internal
PLATFORM_OPEN_NEXT_FILE(WebOpenNextFile)
{
    web_platform_file_group *WebFileGroup =
        (web_platform_file_group *)FileGroup;
    web_platform_file_handle *Result = 0;

    Result = WebAllocateStruct(web_platform_file_handle);
    char *FileName = "asset_1.zas";
    Result->Handle = fopen(FileName, "rb");
    Result->H.HasErrors = (Result->Handle == 0);
    return (platform_file_handle *)Result;
}

internal
PLATFORM_FILE_ERROR(WebFileError)
{
    printf("fail with %s\n", Error);
    Handle->HasErrors = true;
}

internal
PLATFORM_READ_DATA_FROM_FILE(WebReadDataFromFile)
{
    if(PlatformNoFileErrors(Source))
    {
        web_platform_file_handle *Handle =
            (web_platform_file_handle *)Source;
        
        u32 FileSize32 = (u32)(Size);

        Assert(FileSize32 == Size) ;
        fseek(Handle->Handle, Offset, SEEK_SET);
        
        fseek(Handle->Handle, Offset, SEEK_SET);
        u32 BytesRead = fread(Dest, FileSize32, 1, Handle->Handle);
    }
}

internal void
WebLoadOpenglFunctions(open_gl *OpenGL)
{
#define GetOpenglFunction(Name) OpenGL->Name = Name
    
    GetOpenglFunction(glGenTextures);
    GetOpenglFunction(glBindTexture);
    GetOpenglFunction(glTexImage2D);
    GetOpenglFunction(glTexParameteri);
    GetOpenglFunction(glEnable);
    GetOpenglFunction(glBlendFunc);
    GetOpenglFunction(glClear);
    GetOpenglFunction(glClearColor);
    GetOpenglFunction(glGetString);
    
    GetOpenglFunction(glDrawArrays);
    GetOpenglFunction(glCreateProgram);
    GetOpenglFunction(glGenerateMipmap);
    GetOpenglFunction(glGenVertexArrays);
    GetOpenglFunction(glGenBuffers);
    GetOpenglFunction(glUseProgram);
    GetOpenglFunction(glVertexAttribPointer); 
    GetOpenglFunction(glEnableVertexAttribArray);
    GetOpenglFunction(glGetUniformLocation);
    GetOpenglFunction(glUniformMatrix4fv);
    GetOpenglFunction(glDisableVertexAttribArray);    
    GetOpenglFunction(glBindBuffer);
    GetOpenglFunction(glBufferData);
    GetOpenglFunction(glBufferSubData);
    GetOpenglFunction(glBindVertexArray);
    GetOpenglFunction(glShaderSource);
    GetOpenglFunction(glCompileShader);
    GetOpenglFunction(glGetShaderiv);
    GetOpenglFunction(glGetShaderInfoLog);
    GetOpenglFunction(glDeleteShader);
    GetOpenglFunction(glUniform1i);
    GetOpenglFunction(glCreateShader);
    GetOpenglFunction(glBindAttribLocation);
    GetOpenglFunction(glAttachShader);
    GetOpenglFunction(glLinkProgram);
    GetOpenglFunction(glGetProgramiv);
    GetOpenglFunction(glGetProgramInfoLog);
    GetOpenglFunction(glDeleteProgram);
    GetOpenglFunction(glDetachShader);    
}

internal app_button_state *
WebMapSDLKeyToAppKey(app_input *Input, u32 SDLKey)
{
    
    app_button_state *Result = 0;
    
    switch(SDLKey)
    {
        case SDLK_UP:
        {
            Result = &Input->ArrowUp;
            break;
        };
        case SDLK_DOWN:
        {
            Result = &Input->ArrowDown;            
            break;
        };
        case SDLK_RIGHT:
        {
            Result = &Input->ArrowRight;
            break;
        };
        case SDLK_LEFT:
        {
            Result = &Input->ArrowLeft;
            break;

        };
        
        case SDLK_CAPSLOCK:
        {
            Result = &Input->CapsButton;
            break;
        };
        case SDLK_BACKSPACE:
        {
            Result = &Input->BackspaceButton;
            break;
        };
        case SDLK_SPACE:
        {
            Result = &Input->SpaceButton;
            break;
        };
        case SDLK_LSHIFT:
        {
            Result = &Input->ShiftButton;
            break;
        };
        case SDLK_LALT:
        {
            Result = &Input->AltButton;
            break;
        };
        
        case SDLK_z:
        {
            Result = &(Input->ButtonZ);
            break;
        };
        case SDLK_s:
        {
            Result = &Input->ButtonS;
            break;
        };
        case SDLK_q:
        {
            Result = &Input->ButtonQ;
            break;
        };
        case SDLK_d:
        {
            Result = &Input->ButtonD;
            break;
        };

        case SDL_BUTTON_LEFT:
        {
            Result = &Input->LeftButton;
            break;
        };
        case SDL_BUTTON_MIDDLE:
        {
            Result = &Input->MidButton;
            break;
        };
        case SDL_BUTTON_RIGHT:
        {
            Result = &Input->RightButton;
            break;
        };
        




    }
    return Result;    
}

internal void
WebOnSDLKeyDown(app_input *Input,
                u32 SDLKey)
{
    app_button_state *State =
        WebMapSDLKeyToAppKey(Input, SDLKey);

    if (State)
    {
    
        State->Pressed = !State->EndedDown;
        State->Released = 0;

        if (State->EndedDown != true)
        {
            State->EndedDown = true;
            State->halfTransitionCount++;
        }
    }
    
}

internal void
WebOnSDLKeyUp(app_input *Input,
              u32 SDLKey)
{
    app_button_state *State =
        WebMapSDLKeyToAppKey(Input, SDLKey);

    if (State)
    {
    
        State->Pressed = 0;
        State->Released = State->EndedDown;
        if (State->EndedDown != false)
        {
            State->EndedDown = false;
            State->halfTransitionCount++;
        }
    }
    
}

global_variable SDL_Window *Window;
global_variable thread_context *Thread;
global_variable app_memory AppMemory;
global_variable app_input Input;
global_variable int WindowWidth;
global_variable int WindowHeight;
global_variable int MouseX;
global_variable int MouseY;
global_variable float Fps = 24.f;

internal void
MainLoop()
{

    float TargetSecondsPerFrame = 1.f / Fps;
    Input.DeltaTime = TargetSecondsPerFrame;
    
    SDL_Event Event;
    while(SDL_PollEvent(&Event)){
        switch (Event.type) {
            case SDL_QUIT:
            {
                // QUIT
                exit(0);
                break;
            }
            case SDL_MOUSEMOTION:
            {
                MouseX = Event.motion.x;
                MouseY = WindowHeight - Event.motion.y;
                break;
            }
            case SDL_KEYDOWN:
            {
                WebOnSDLKeyDown(&Input,
                                Event.key.keysym.sym);
                break;
            }
            case SDL_KEYUP:
            {
                WebOnSDLKeyUp(&Input,
                              Event.key.keysym.sym);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                WebOnSDLKeyDown(&Input,
                              Event.button.button);
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                WebOnSDLKeyUp(&Input,
                              Event.button.button);
                break;
            }
            default:
            {
                break;
            }
        }
    }    

    
    Input.MouseX = MouseX;
    Input.MouseY = MouseY;
    Input.MouseZ = 0;
    
    render_context *RenderContext = &Thread->RenderContext;
    open_gl *OpenGL = RenderContext->OpenGL;

    
    #if 1
    app_window AppWindow;
    AppWindow.Width = WindowWidth;
    AppWindow.Height = WindowHeight;
    AppUpdateAndRender(Thread, &AppMemory, &Input, &AppWindow);
    #endif
    
    SDL_GL_SwapWindow(Window);
}

int main()
{
    int Width, Height;
    SDL_GLContext Context;
    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    Window = SDL_CreateWindow("randomWindow", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 768, SDL_WINDOW_OPENGL);
    Context = SDL_GL_CreateContext(Window);
    if (!Context) {
    }
    
    {
    }

    
    SDL_GL_SetSwapInterval(0);
        
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    SDL_GetWindowSize(Window, &WindowWidth, &WindowHeight);

     
    open_gl OpenGL;
    WebLoadOpenglFunctions(&OpenGL);
    thread_context ThreadContext = {};
    ThreadContext.RenderContext.OpenGL = &OpenGL;
    Thread = &ThreadContext;
    AppMemory = {};

    AppMemory.PlatformApi.GetAllFilesOfTypeBegin = WebGetAllFilesOfTypeBegin;
    AppMemory.PlatformApi.GetAllFilesOfTypeEnd = WebGetAllFilesOfTypeEnd;
    AppMemory.PlatformApi.OpenNextFile = WebOpenNextFile;
    AppMemory.PlatformApi.ReadDataFromFile = WebReadDataFromFile;
    AppMemory.PlatformApi.FileError = WebFileError;

    AppMemory.PlatformApi.ReadEntireFile = WebReadEntireFile;
    AppMemory.PlatformApi.FreeFileMemory = WebFreeFileMemory;
    AppMemory.PlatformApi.WriteEntireFile = WebWriteEntireFile;
    AppMemory.PlatformApi.AddWorkEntry = WebAddWorkEntry;

    AppMemory.PermanentStorageSize = Megabytes(64);
    AppMemory.TransientStorageSize = Megabytes(8);
    
    AppMemory.PermanentStorage = calloc(AppMemory.PermanentStorageSize, 1);
    AppMemory.TransientStorage = calloc(AppMemory.TransientStorageSize, 1);


    
    app_state *AppState = (app_state *)AppMemory.PermanentStorage;
    
    emscripten_set_main_loop(MainLoop, Fps, true);
}
         
