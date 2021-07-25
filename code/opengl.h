#if !defined(OPENGL_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define OPENGL_H
#include <stddef.h>
//#include "windows.h"
//#include "gl/gl.h"

typedef u32 GLbitfield;
typedef float GLclampf;
typedef unsigned char GLubyte;
typedef u32 GLuint;
typedef unsigned int GLenum;
typedef i32 GLint;
typedef i32 GLsizei;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef void GLvoid;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

#ifndef GL_VERSION
#define GL_VERSION                        7938
#define GL_SRC_ALPHA                      770
#define GL_ONE_MINUS_SRC_ALPHA            771
#define GL_COLOR_BUFFER_BIT               16384
#define GL_DEPTH_BUFFER_BIT               256
#define GL_TEXTURE_2D                     3553
#define GL_FLOAT                          5126
#define GL_UNSIGNED_BYTE                  5121
#define GL_TEXTURE_WRAP_S                 10242
#define GL_TEXTURE_WRAP_T                 10243
#define GL_TEXTURE_MAG_FILTER             10240
#define GL_TEXTURE_MIN_FILTER             10241
#define GL_LINEAR_MIPMAP_LINEAR           9987
#define GL_NEAREST                        9728
#define GL_CLAMP_TO_EDGE                  33071
#define GL_ALPHA                          6406
#define GL_BLEND                          0x0BE2
#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_TRIANGLES                      4
#define GL_LINE_LOOP                      2
#define GL_LINEAR                         9729
#define GL_REPEAT                         10497
#define GL_RGBA                           6408
#define GL_RGB                            6407
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_COMPILE_STATUS                 0x8B81
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_LINK_STATUS                    0x8B82
#endif

typedef void type_glDrawArrays(GLenum Mode,
                                 GLint First,
                                 GLsizei Count);
typedef GLuint type_glCreateProgram(void);
typedef void type_glGenerateMipmap(GLenum Target);
typedef void type_glGenVertexArrays(GLsizei N,
                                    GLuint *Arrays);
typedef void type_glGenBuffers(GLsizei N,
                               GLuint *Buffers);
typedef void type_glVertexAttribPointer(GLuint Index,
                                        GLint Size,
                                        GLenum Type,
                                        GLboolean Normalized,
                                        GLsizei Stride,
                                        const void *Pointer);
typedef void type_glUseProgram(GLuint Program);
typedef void type_glEnableVertexAttribArray(GLuint Index);
typedef GLint type_glGetUniformLocation(GLuint Program,
                                   const GLchar *Name);
typedef void type_glUniformMatrix4fv(GLint Location,
                                GLsizei Count,
                                GLboolean transpose,
                                const GLfloat *Value);
typedef void type_glDisableVertexAttribArray(GLuint Index);
typedef void type_glBindBuffer(GLenum Target,
                               GLuint Buffer);
typedef void type_glBufferData(GLenum Target,
                               GLsizeiptr Size,
                               const void *Data,
                               GLenum Usage);

typedef void type_glBufferSubData(GLenum Target,
                               GLintptr Offset,
                               GLsizeiptr Size,
                               const void *Data);

typedef void type_glBindVertexArray(GLuint Array);
typedef void type_glShaderSource(GLuint Shader,
                                 GLsizei Count,
                                 const GLchar *const *String,
                                 const GLint *Length);
typedef void type_glCompileShader(GLuint Shader);
typedef void type_glGetShaderiv(GLuint Shader,
                                GLenum PName,
                                GLint *Params);
typedef void type_glGetShaderInfoLog(GLuint Shader,
                                     GLsizei MaxLength,
                                     GLsizei *Length,
                                     GLchar *InfoLog);
typedef void type_glDeleteShader(GLuint Shader);
typedef void type_glUniform1i(GLint Location,
                              GLint V0);

typedef GLuint type_glCreateShader(GLenum ShaderType);
typedef void type_glBindAttribLocation(GLuint Program,
                                       GLuint Index,
                                       const GLchar *Name);
typedef void type_glAttachShader(GLuint Program,
                                 GLuint Shader);
typedef void type_glLinkProgram(GLuint Program);
typedef void type_glGetProgramiv(GLuint Program,
                                 GLenum PName,
                                 GLint *Params);
typedef void type_glGetProgramInfoLog(GLuint Program,
                                      GLsizei MaxLength,
                                      GLsizei *Length,
                                      GLchar *InfoLog);
typedef void type_glDeleteProgram(GLuint Program);
typedef void type_glDetachShader(GLuint Program,
                                 GLuint Shader);

#if 1
typedef void type_glGenTextures(GLsizei N,
                                GLuint* Texutres);
typedef void type_glDeleteTextures(GLsizei N,
                                GLuint* Texutres);
typedef void type_glBindTexture(GLenum Target,
                                GLuint Texutre);
typedef void type_glTexImage2D(GLenum Target,
                               GLint Level,
                               GLint internalFormat,
                               GLsizei Width, GLsizei Height,
                               GLint Border, GLenum Format,
                               GLenum Type, const GLvoid* Pixels);
typedef void type_glTexParameteri(GLenum Target,
                                  GLenum Pname,
                                  GLint Param);
typedef void type_glEnable(GLenum Cap);
typedef void type_glBlendFunc(GLenum SFactor,
                              GLenum DFactor);
typedef void type_glClear(GLbitfield Mask);
typedef void type_glClearColor(GLclampf Red,
                               GLclampf Green,
                               GLclampf Blue,
                               GLclampf Alpha);
typedef const GLubyte *type_glGetString(GLenum Name);
#endif

#define OpenGLFunction(Name) type_##Name *Name


struct open_gl
{
    bool32 SupportsSRGBFramebuffer;
    
#if 1
    OpenGLFunction(glGenTextures);
    OpenGLFunction(glDeleteTextures);
    OpenGLFunction(glBindTexture);
    OpenGLFunction(glTexImage2D);
    OpenGLFunction(glTexParameteri);
    OpenGLFunction(glEnable);
    OpenGLFunction(glBlendFunc);
    OpenGLFunction(glClear);
    OpenGLFunction(glClearColor);
    OpenGLFunction(glGetString);
#endif
    
    OpenGLFunction(glDrawArrays);
    OpenGLFunction(glCreateProgram);
    OpenGLFunction(glGenerateMipmap);
    OpenGLFunction(glGenVertexArrays);
    OpenGLFunction(glBindVertexArray);
    OpenGLFunction(glGenBuffers);
    OpenGLFunction(glVertexAttribPointer); 
    OpenGLFunction(glUseProgram);
    OpenGLFunction(glEnableVertexAttribArray);
    OpenGLFunction(glGetUniformLocation);
    OpenGLFunction(glUniformMatrix4fv);    
    OpenGLFunction(glDisableVertexAttribArray);    
    OpenGLFunction(glBindBuffer);
    OpenGLFunction(glBufferData);
    OpenGLFunction(glBufferSubData);
    OpenGLFunction(glShaderSource);
    OpenGLFunction(glCompileShader);
    OpenGLFunction(glGetShaderiv);
    OpenGLFunction(glGetShaderInfoLog);
    OpenGLFunction(glDeleteShader);
    OpenGLFunction(glUniform1i);
    OpenGLFunction(glCreateShader);
    OpenGLFunction(glBindAttribLocation);
    OpenGLFunction(glAttachShader);
    OpenGLFunction(glLinkProgram);
    OpenGLFunction(glGetProgramiv);
    OpenGLFunction(glGetProgramInfoLog);
    OpenGLFunction(glDeleteProgram);
    OpenGLFunction(glDetachShader);

};
#endif
