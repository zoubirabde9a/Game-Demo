/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: zoubir $
   ======================================================================== */
internal bool32
CompileShader(open_gl *OpenGL,
              memory_arena *Arena,
              void* source, size_t size, char* name, u32 id)
{

    GLint sizeGL = (GLint)size;
    GLchar *sourceGL = (GLchar *)source;
    //tell opengl that we want to use fileContents as the contents of the shader file
    OpenGL->glShaderSource(id, 1, &sourceGL, &sizeGL);
    //compile the shader
    OpenGL->glCompileShader(id);
    //check for errors
    i32 success = 0;
    OpenGL->glGetShaderiv(id, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE)
    {
        i32 maxLength = 0;
        OpenGL->glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

        //The maxLength includes the NULL character
        temporary_memory TempMem = BeginTemporaryMemory(Arena);
        char* errorLog = AllocateArray(Arena, (u32)maxLength, char);
        OpenGL->glGetShaderInfoLog(id, maxLength, &maxLength, errorLog);

        
        //TODO(zoubir): Add logging
        EndTemporaryMemory(TempMem);
        Assert(0);

        OpenGL->glDeleteShader(id); //Don't leak the shader.
    }
    return success == GL_FALSE;
}

internal void
InitProgram(open_gl *OpenGL, memory_arena *Arena,
            render_program *Program, char *VertexShaderFileName,
            char *FragmentShaderFileName, char **Attributes, u32 NumAttrib)
{
    Program->ID = OpenGL->glCreateProgram();
    u32 VertexShaderID =
        OpenGL->glCreateShader(GL_VERTEX_SHADER);
    if (VertexShaderID == 0) {
        // Error Creating VertexShader
        // TODO(zoubir): Add Logging
        Assert(0);
    }

    u32 FragmentShaderID =
        OpenGL->glCreateShader(GL_FRAGMENT_SHADER);
    if (FragmentShaderID == 0) {
        // Error Creating FragmentShader
        // TODO(zoubir): Add Logging
        Assert(0);
    }
    debug_read_file_result VertexFileResult =
        Platform.ReadEntireFile(VertexShaderFileName);
    Assert(VertexFileResult.Memory);
    debug_read_file_result FragmentFileResult =
        Platform.ReadEntireFile(FragmentShaderFileName);
    Assert(FragmentFileResult.Memory);

    // Compile Shaders
    CompileShader(OpenGL, Arena, VertexFileResult.Memory,
                  VertexFileResult.Size, "Vertex Shader", VertexShaderID);
    CompileShader(OpenGL, Arena, FragmentFileResult.Memory,
                  FragmentFileResult.Size, "Fragment Shader", FragmentShaderID);

    
    Program->NumAttrib = NumAttrib;
    for(u32 CurrentAttribute = 0;
        CurrentAttribute < NumAttrib;
        CurrentAttribute++)
    {
        OpenGL->glBindAttribLocation(Program->ID, CurrentAttribute,
                             Attributes[CurrentAttribute]);
    }

    //Attach shaders to program
    OpenGL->glAttachShader(Program->ID, VertexShaderID);
    OpenGL->glAttachShader(Program->ID, FragmentShaderID);
        
    //Link program
    OpenGL->glLinkProgram(Program->ID);
        
    //Note the different functions here: glGetProgram* instead of glGetShader*.
    int isLinked = 0;
    OpenGL->glGetProgramiv(Program->ID, GL_LINK_STATUS, (int *)&isLinked);
    if (isLinked == GL_FALSE)
    {
        int32_t maxLength = 0;
        OpenGL->glGetProgramiv(Program->ID, GL_INFO_LOG_LENGTH, &maxLength);

        //The maxLength includes the NULL character
        temporary_memory TempMem = BeginTemporaryMemory(Arena);
        char* errorLog =
            AllocateArray(Arena, (u32)maxLength, char);
        OpenGL->glGetProgramInfoLog(Program->ID, maxLength, &maxLength, errorLog);

        // TODO(zoubir): Add Logging
        EndTemporaryMemory(TempMem);
        Assert(0);
        //We don't need the program anymore.
        OpenGL->glDeleteProgram(Program->ID);
        //Don't leak shaders either.
        OpenGL->glDeleteShader(VertexShaderID);
        OpenGL->glDeleteShader(FragmentShaderID);
    }

    //Always detach shaders after a successful link.
    OpenGL->glDetachShader(Program->ID, VertexShaderID);
    OpenGL->glDetachShader(Program->ID, FragmentShaderID);
    OpenGL->glDeleteShader(VertexShaderID);
    OpenGL->glDeleteShader(FragmentShaderID);
}

internal void
AppInitOpenGL(memory_arena *TransientArena, app_state *AppState,
              thread_context *Thread, app_memory *Memory)
{
    render_context* RenderContext = &Thread->RenderContext;
    open_gl *OpenGL = RenderContext->OpenGL;

    render_program *TextureProgram = &RenderContext->TextureProgram;
    render_program *LineProgram = &RenderContext->LineProgram;
    OpenGL->glEnable(GL_BLEND);
    OpenGL->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char *TextureProgramAttributes[] =
    {
        "vertexPosition",
        "vertexColor",
        "vertexUV",
    };
    char *LineProgramAttributes[] =
    {
        "vertexPosition",
        "vertexColor"
    };
    
    InitProgram(OpenGL, TransientArena, TextureProgram,
                "shaders/texture_shading.vert", "shaders/texture_shading.frag",
                TextureProgramAttributes,
                ArrayCount(TextureProgramAttributes));
    InitProgram(OpenGL, TransientArena, LineProgram,
                "shaders/line_shading.vert", "shaders/line_shading.frag",
                LineProgramAttributes,
                ArrayCount(LineProgramAttributes));

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
    
#if 0    
    u32 TextureProgram = RenderContext->TextureProgram;
    OpenGL->glUseProgram(TextureProgram);
    OpenGL->glEnableVertexAttribArray(0);
    OpenGL->glEnableVertexAttribArray(1);
    OpenGL->glEnableVertexAttribArray(2);
        
    GLint textureUniform = OpenGL->glGetUniformLocation(TextureProgram, "textureSampler");
    OpenGL->glUniform1i(textureUniform, 0);
    glActiveTexture(GL_TEXTURE0);
#endif
    
}
