/* Browser OpenGL: WebGL 2 through Emscripten's OpenGL ES 3 functions,
   handed to the game in the same open_gl table the Windows layer fills
   (engine/opengl.h). */

#define WebGetOpenglFunction(Name) OpenGL->Name = (type_##Name *)Name

// NOTE(zoubir): the game hands its shaders over in pieces (prelude,
// library, file). With 8-byte pointers (webuild.bat) Emscripten's
// glShaderSource reads that list of pieces wrong, so they are joined here
// and passed as one string.
internal void
WebShaderSource(GLuint Shader, GLsizei Count, const GLchar *const *Strings, const GLint *Lengths)
{
    size_t Total = 1;
    for (GLsizei Index = 0; Index < Count; Index++)
    {
        Total += (Lengths && Lengths[Index] >= 0) ? (size_t)Lengths[Index] : strlen(Strings[Index]);
    }
    char *Joined = (char *)malloc(Total);
    if (!Joined) return;
    size_t At = 0;
    for (GLsizei Index = 0; Index < Count; Index++)
    {
        size_t Size = (Lengths && Lengths[Index] >= 0) ? (size_t)Lengths[Index] : strlen(Strings[Index]);
        memcpy(Joined + At, Strings[Index], Size);
        At += Size;
    }
    Joined[At] = 0;
    const GLchar *One = Joined;
    glShaderSource(Shader, 1, &One, 0);
    free(Joined);
}

internal void
WebLoadOpenglFunctions(open_gl *OpenGL)
{
    WebGetOpenglFunction(glAttachShader);
    WebGetOpenglFunction(glBindAttribLocation);
    WebGetOpenglFunction(glBindBuffer);
    WebGetOpenglFunction(glBindFramebuffer);
    WebGetOpenglFunction(glBindTexture);
    WebGetOpenglFunction(glBindVertexArray);
    WebGetOpenglFunction(glBlendFunc);
    WebGetOpenglFunction(glBufferData);
    WebGetOpenglFunction(glBufferSubData);
    WebGetOpenglFunction(glCheckFramebufferStatus);
    WebGetOpenglFunction(glClear);
    WebGetOpenglFunction(glClearColor);
    WebGetOpenglFunction(glCompileShader);
    WebGetOpenglFunction(glCreateProgram);
    WebGetOpenglFunction(glCreateShader);
    WebGetOpenglFunction(glDeleteProgram);
    WebGetOpenglFunction(glDeleteShader);
    WebGetOpenglFunction(glDeleteTextures);
    WebGetOpenglFunction(glDetachShader);
    WebGetOpenglFunction(glDisableVertexAttribArray);
    WebGetOpenglFunction(glDrawArrays);
    WebGetOpenglFunction(glEnable);
    WebGetOpenglFunction(glEnableVertexAttribArray);
    WebGetOpenglFunction(glFramebufferTexture2D);
    WebGetOpenglFunction(glGenBuffers);
    WebGetOpenglFunction(glGenFramebuffers);
    WebGetOpenglFunction(glGenTextures);
    WebGetOpenglFunction(glGenVertexArrays);
    WebGetOpenglFunction(glGenerateMipmap);
    WebGetOpenglFunction(glGetIntegerv);
    WebGetOpenglFunction(glGetProgramInfoLog);
    WebGetOpenglFunction(glGetProgramiv);
    WebGetOpenglFunction(glGetShaderInfoLog);
    WebGetOpenglFunction(glGetShaderiv);
    WebGetOpenglFunction(glGetString);
    WebGetOpenglFunction(glGetUniformLocation);
    WebGetOpenglFunction(glLinkProgram);
    OpenGL->glShaderSource = WebShaderSource;
    WebGetOpenglFunction(glTexImage2D);
    WebGetOpenglFunction(glTexParameteri);
    WebGetOpenglFunction(glUniform1f);
    WebGetOpenglFunction(glUniform1i);
    WebGetOpenglFunction(glUniform2f);
    WebGetOpenglFunction(glUniform4fv);
    WebGetOpenglFunction(glUniformMatrix4fv);
    WebGetOpenglFunction(glUseProgram);
    WebGetOpenglFunction(glVertexAttribPointer);
}
