/* OpenGL setup: BuildProgram compiles a vertex and a fragment shader and
   links them, binding attributes to locations 0, 1, 2 in order, and
   returns 0 with the driver's message in Error on failure.
   AppInitOpenGL runs once at startup: it turns on alpha blending, loads
   the shader library and creates the vertex array and buffer laid out as
   render_vertex (position, color, UV). */
// NOTE(zoubir): compiles one stage from Count source strings (a prelude
// and the file); false with the driver's message in Error when it fails
internal bool32
CompileShader(open_gl *OpenGL, u32 ShaderID, u32 Count, char **Sources,
              i32 *Sizes, char *Error, u32 ErrorSize)
{
    OpenGL->glShaderSource(ShaderID, (GLsizei)Count, (GLchar **)Sources, Sizes);
    OpenGL->glCompileShader(ShaderID);
    i32 Success = 0;
    OpenGL->glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Success);
    if (Success == GL_FALSE)
    {
        GLsizei Length = 0;
        OpenGL->glGetShaderInfoLog(ShaderID, (GLsizei)ErrorSize, &Length, Error);
    }
    return Success != GL_FALSE;
}

// NOTE(zoubir): a linked program from vertex and fragment sources (each
// Count strings), the attributes bound to locations 0, 1, 2 in order.
// Returns 0, with the reason in Error, when either stage or the link fails
internal u32
BuildProgram(open_gl *OpenGL, u32 Count, char **VertexSources, i32 *VertexSizes,
             char **FragmentSources, i32 *FragmentSizes,
             char **Attributes, u32 AttributeCount,
             char *Error, u32 ErrorSize)
{
    Error[0] = 0;
    u32 Vertex = OpenGL->glCreateShader(GL_VERTEX_SHADER);
    u32 Fragment = OpenGL->glCreateShader(GL_FRAGMENT_SHADER);
    u32 Program = 0;
    if (CompileShader(OpenGL, Vertex, Count, VertexSources, VertexSizes,
                      Error, ErrorSize) &&
        CompileShader(OpenGL, Fragment, Count, FragmentSources, FragmentSizes,
                      Error, ErrorSize))
    {
        Program = OpenGL->glCreateProgram();
        for(u32 Index = 0; Index < AttributeCount; Index++)
        {
            OpenGL->glBindAttribLocation(Program, Index, Attributes[Index]);
        }
        OpenGL->glAttachShader(Program, Vertex);
        OpenGL->glAttachShader(Program, Fragment);
        OpenGL->glLinkProgram(Program);
        i32 Linked = 0;
        OpenGL->glGetProgramiv(Program, GL_LINK_STATUS, &Linked);
        OpenGL->glDetachShader(Program, Vertex);
        OpenGL->glDetachShader(Program, Fragment);
        if (Linked == GL_FALSE)
        {
            GLsizei Length = 0;
            OpenGL->glGetProgramInfoLog(Program, (GLsizei)ErrorSize, &Length, Error);
            OpenGL->glDeleteProgram(Program);
            Program = 0;
        }
    }
    OpenGL->glDeleteShader(Vertex);
    OpenGL->glDeleteShader(Fragment);
    return Program;
}

internal void
AppInitOpenGL(memory_arena *TransientArena, app_state *AppState,
              thread_context *Thread, app_memory *Memory)
{
    render_context* RenderContext = &Thread->RenderContext;
    open_gl *OpenGL = RenderContext->OpenGL;

    OpenGL->glEnable(GL_BLEND);
    OpenGL->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    LoadShaderLibrary(RenderContext);

    OpenGL->glGenVertexArrays(1, &RenderContext->VAO);

    //G enerate the VBO if it isn't already generated
    OpenGL->glGenBuffers(1, &RenderContext->VBO);
    //glm::mat4 projectionMatrix = camera.getCameraMatrix();
      
    OpenGL->glBindVertexArray(RenderContext->VAO);
    OpenGL->glBindBuffer(GL_ARRAY_BUFFER, RenderContext->VBO);

    //Tell opengl what attribute arrays we need
    OpenGL->glEnableVertexAttribArray(0);
    OpenGL->glEnableVertexAttribArray(1);
    OpenGL->glEnableVertexAttribArray(2);

//This is the position attribute pointer
    OpenGL->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(render_vertex),
                                  (void*)OffsetOf(render_vertex, X));
    //This is the color attribute pointer
    OpenGL->glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(render_vertex),
                                  (void*)OffsetOf(render_vertex, R));
    //This is the UV attribute pointer
    OpenGL->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(render_vertex),
                                  (void*)OffsetOf(render_vertex, U));
    
}
