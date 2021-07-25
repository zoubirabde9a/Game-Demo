/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */

#include "windows.h"
#include "opengl.h"

internal void *
Win32GetGLFuncAddress(HMODULE OpenglDLL, char *name)
{
    void *Result = (void *)wglGetProcAddress(name);
  
  if(OpenglDLL &&
     (Result == 0 ||
     (Result == (void*)0x1) ||
     (Result == (void*)0x2) ||
     (Result == (void*)0x3) ||
      (Result == (void*)-1)))
  {
      Result = (void *)GetProcAddress(OpenglDLL, name);
      Assert(Result);
  }

  return Result;
}

internal void
Win32LoadOpenglFunctions(open_gl *OpenGL)
{
    HMODULE OpenglDLL = LoadLibraryA("opengl32.dll");

#define GetOpenglFunction(Name) OpenGL->Name = (type_##Name *)Win32GetGLFuncAddress(OpenglDLL, #Name)
    
#if 1
    GetOpenglFunction(glGenTextures);
    GetOpenglFunction(glDeleteTextures);
    GetOpenglFunction(glBindTexture);
    GetOpenglFunction(glTexImage2D);
    GetOpenglFunction(glTexParameteri);
    GetOpenglFunction(glEnable);
    GetOpenglFunction(glBlendFunc);
    GetOpenglFunction(glClear);
    GetOpenglFunction(glClearColor);
    GetOpenglFunction(glGetString);
#endif
    
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

#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_PIXEL_TYPE_ARB                      0x2013

#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_FULL_ACCELERATION_ARB               0x2027

#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB        0x20A9

typedef HGLRC WINAPI wgl_create_context_attribs_arb(HDC hDC, HGLRC hShareContext,
                                                    const int *attribList);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_iv_arb(HDC hdc,
                                                       int iPixelFormat,
                                                       int iLayerPlane,
                                                       UINT nAttributes,
                                                       const int *piAttributes,
                                                       int *piValues);

typedef BOOL WINAPI wgl_get_pixel_format_attrib_fv_arb(HDC hdc,
                                                       int iPixelFormat,
                                                       int iLayerPlane,
                                                       UINT nAttributes,
                                                       const int *piAttributes,
                                                       FLOAT *pfValues);

typedef BOOL WINAPI wgl_choose_pixel_format_arb(HDC hdc,
                                                const int *piAttribIList,
                                                const FLOAT *pfAttribFList,
                                                UINT nMaxFormats,
                                                int *piFormats,
                                                UINT *nNumFormats);
typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
typedef const char * WINAPI wgl_get_extensions_string_ext(void);


global_variable wgl_create_context_attribs_arb *wglCreateContextAttribsARB;
global_variable wgl_choose_pixel_format_arb *wglChoosePixelFormatARB;
global_variable wgl_swap_interval_ext *wglSwapIntervalEXT;
global_variable wgl_get_extensions_string_ext *wglGetExtensionsStringEXT;


internal void
Win32SetPixelFormat(open_gl *OpenGL, HDC WindowDC)
{
    int SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick = 0;
    if(wglChoosePixelFormatARB)
    {
        int IntAttribList[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE, // 0
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB, // 1
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE, // 2
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE, // 3
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, // 4
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE, // 5
            0,
        };
        
        if(!OpenGL->SupportsSRGBFramebuffer)
        {
            IntAttribList[10] = 0;
        }
        
        wglChoosePixelFormatARB(WindowDC, IntAttribList, 0, 1,
                                &SuggestedPixelFormatIndex, &ExtendedPick);
    }
    
    if(!ExtendedPick)
    {
        // TODO(casey): Hey Raymond Chen - what's the deal here?
        // Is cColorBits ACTUALLY supposed to exclude the alpha bits, like MSDN says, or not?
        PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
        DesiredPixelFormat.nVersion = 1;
        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        DesiredPixelFormat.cColorBits = 32;
        DesiredPixelFormat.cAlphaBits = 8;
        DesiredPixelFormat.cDepthBits = 24;
        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
        
        SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }
    
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    // NOTE(casey): Technically you do not need to call DescribePixelFormat here,
    // as SetPixelFormat doesn't actually need it to be filled out properly.
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

internal void
Win32LoadWGLExtensions(open_gl *OpenGL)
{
    WNDCLASSA WindowClass = {};
    
    WindowClass.lpfnWndProc = DefWindowProcA;
    WindowClass.hInstance = GetModuleHandle(0);
    WindowClass.lpszClassName = "HandmadeWGLLoader";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            WindowClass.hInstance,
            0);
        
        HDC WindowDC = GetDC(Window);
        Win32SetPixelFormat(OpenGL, WindowDC);
        HGLRC OpenGLRC = wglCreateContext(WindowDC);
        if(wglMakeCurrent(WindowDC, OpenGLRC))
        {
            wglChoosePixelFormatARB =
                (wgl_choose_pixel_format_arb *)wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB =
                (wgl_create_context_attribs_arb *)wglGetProcAddress("wglCreateContextAttribsARB");
            wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)wglGetProcAddress("wglGetExtensionsStringEXT");
                        
            wglMakeCurrent(0, 0);
        }
        
        wglDeleteContext(OpenGLRC);
        ReleaseDC(Window, WindowDC);
        DestroyWindow(Window);
    }
}

