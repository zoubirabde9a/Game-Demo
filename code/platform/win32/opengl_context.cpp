/* Timing and OpenGL: the wall clock, and creating the OpenGL context. */

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

// NOTE(zoubir): Platform.WallSeconds, for the game's frame timing
internal PLATFORM_WALL_SECONDS(Win32WallSeconds)
{
    double Result = (double)Win32GetWallClock().QuadPart / (double)GlobalPerCounterFrequency;
    return Result;
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
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
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
    
//    const GLubyte *version = glGetString(GL_VERSION);
    
//    GLenum error = glewInit();
//    Assert(error == GLEW_OK);
}
