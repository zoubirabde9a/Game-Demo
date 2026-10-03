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
#if 0
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#else
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
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
    // https://learnopengl.com/Advanced-OpenGL/Framebuffers
    // for frame buffer
    // https://stackoverflow.com/questions/12157646/how-to-render-offscreen-on-opengl
    // to render offscreern
    #if 0
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.dwFlags =
        PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    DesiredPixelFormat.cColorBits = 24;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
    HDC WindowDC = GetDC(Window);
    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
    
    HGLRC context = wglCreateContext(WindowDC);
    if (!wglMakeCurrent(WindowDC, context))
    {
        Assert(0);
    }
    ReleaseDC(Window, WindowDC);
#endif
    
//    const GLubyte *version = glGetString(GL_VERSION);
    
//    GLenum error = glewInit();
//    Assert(error == GLEW_OK);
    #if 0
    Win32LoadOpenGLFunctions();
    #endif

#if 0
    // Screen Frame Buffer
    {
        glGenFramebuffers(1, &AppState->ScreenFboID);
        glBindFramebuffer(GL_FRAMEBUFFER, AppState->ScreenFboID);
        Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
            
        glGenTextures(1, &AppState->ScreenFboTexture);
        glBindTexture(GL_TEXTURE_2D, AppState->ScreenFboTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, buffer->width, buffer->height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, AppState->ScreenFboTexture, 0);
    }
#endif
}
