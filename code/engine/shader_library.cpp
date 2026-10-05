/* Shader library: every shader program the game draws with, by shader_id
   (render.h), loaded from shaders/ in the folder the game runs from.

   A new effect is three steps: a shader_id line in render.h, a row in
   ShaderDefs below, and its .frag file in build/shaders/fx/. Then draw it
   with DrawShaderQuad (engine/render/shapes.cpp), alpha or additive.

   Files under fx/ are written once for both the desktop (GLSL 1.30) and
   the browser (GLSL ES 1.00): the loader puts a prelude in front that
   defines ATTRIBUTE, VARYING, TEXTURE and FragColor for each. Every
   program gets the same three attributes (vertexPosition, vertexColor,
   vertexUV) and may use two uniforms: P, the projection, and Time,
   seconds since launch.

   Developer builds look at the files twice a second and rebuild the ones
   that changed, so a shader can be tuned with the game running. A file
   that fails to compile keeps the last good program and puts the
   driver's message in ShaderLibraryError, which the screen shows
   (ui/shader_errors.cpp). */

#define SHADER_RELOAD_SECONDS 0.5f
#define SHADER_ERROR_SIZE 512

struct shader_def
{
    char *Name;
    char *Vertex;
    char *Fragment;
    u32 AttributeCount;
    // NOTE(zoubir): written in the shared dialect and given a prelude
    bool32 Shared;
};

global_variable shader_def ShaderDefs[Shader_Count] =
{
    {"texture", "shaders/texture_shading.vert", "shaders/texture_shading.frag", 3, false},
    {"line", "shaders/line_shading.vert", "shaders/line_shading.frag", 2, false},
    {"slot frame", "shaders/fx/quad.vert", "shaders/fx/slot_frame.frag", 3, true},
    {"cooldown sweep", "shaders/fx/quad.vert", "shaders/fx/cooldown_sweep.frag", 3, true},
    {"glow", "shaders/fx/quad.vert", "shaders/fx/glow.frag", 3, true},
    {"ring", "shaders/fx/quad.vert", "shaders/fx/ring.frag", 3, true},
    {"panel", "shaders/fx/quad.vert", "shaders/fx/panel.frag", 3, true},
    {"time warp", "shaders/fx/quad.vert", "shaders/fx/time_warp.frag", 3, true},
    {"time sigil", "shaders/fx/quad.vert", "shaders/fx/time_sigil.frag", 3, true},
    {"talent node", "shaders/fx/quad.vert", "shaders/fx/talent_node.frag", 3, true},
    {"xp bar", "shaders/fx/quad.vert", "shaders/fx/xp_bar.frag", 3, true},
    {"talent backdrop", "shaders/fx/quad.vert", "shaders/fx/talent_backdrop.frag", 3, true},
};

global_variable char *ShaderAttributes[] =
{
    "vertexPosition",
    "vertexColor",
    "vertexUV",
};

#if COMPILER_EMSCRIPTEN
global_variable char ShaderVertexPrelude[] =
    "#define ATTRIBUTE attribute\n"
    "#define VARYING varying\n";
global_variable char ShaderFragmentPrelude[] =
    "precision mediump float;\n"
    "#define VARYING varying\n"
    "#define TEXTURE texture2D\n"
    "#define FragColor gl_FragColor\n";
#else
global_variable char ShaderVertexPrelude[] =
    "#version 130\n"
    "#define ATTRIBUTE in\n"
    "#define VARYING out\n";
global_variable char ShaderFragmentPrelude[] =
    "#version 130\n"
    "#define VARYING in\n"
    "#define TEXTURE texture\n"
    "out vec4 FragColor;\n";
#endif

struct shader_library_state
{
    // NOTE(zoubir): hash of each program's two files when last built
    u32 Hash[Shader_Count];
    float SinceCheck;
    char Error[SHADER_ERROR_SIZE];
    u32 ErrorShader; // which program Error is about
};

// NOTE(zoubir): a global, not app state: it only caches what is on disk,
// and starting over after a code reload just rebuilds every shader once
global_variable shader_library_state ShaderLibrary;

inline u32
HashShaderBytes(u32 Hash, u8 *Bytes, u32 Size)
{
    for(u32 Index = 0; Index < Size; Index++)
    {
        Hash = (Hash ^ Bytes[Index]) * 16777619u;
    }
    return Hash;
}

// NOTE(zoubir): builds program Index from its files. A failure keeps the
// program it had and writes the reason into ShaderLibrary.Error. With
// Force false, files whose bytes have not changed are left alone
internal bool32
LoadShaderProgram(render_context *RenderContext, u32 Index, bool32 Force)
{
    open_gl *OpenGL = RenderContext->OpenGL;
    shader_def *Def = &ShaderDefs[Index];
    debug_read_file_result Vertex = Platform.ReadEntireFile(Def->Vertex);
    debug_read_file_result Fragment = Platform.ReadEntireFile(Def->Fragment);
    bool32 Result = false;
    if (Vertex.Memory && Fragment.Memory)
    {
        u32 Hash = HashShaderBytes(2166136261u, (u8 *)Vertex.Memory, Vertex.Size);
        Hash = HashShaderBytes(Hash, (u8 *)Fragment.Memory, Fragment.Size);
        if (Force || Hash != ShaderLibrary.Hash[Index])
        {
            ShaderLibrary.Hash[Index] = Hash;
            char *VertexSources[2] = {Def->Shared ? ShaderVertexPrelude : (char *)"",
                                      (char *)Vertex.Memory};
            i32 VertexSizes[2] = {Def->Shared ? (i32)(sizeof(ShaderVertexPrelude) - 1) : 0,
                                  (i32)Vertex.Size};
            char *FragmentSources[2] = {Def->Shared ? ShaderFragmentPrelude : (char *)"",
                                        (char *)Fragment.Memory};
            i32 FragmentSizes[2] = {Def->Shared ? (i32)(sizeof(ShaderFragmentPrelude) - 1) : 0,
                                    (i32)Fragment.Size};
            char Error[SHADER_ERROR_SIZE];
            u32 ID = BuildProgram(OpenGL, 2, VertexSources, VertexSizes,
                                  FragmentSources, FragmentSizes,
                                  ShaderAttributes, Def->AttributeCount,
                                  Error, sizeof(Error));
            if (ID)
            {
                render_program *Program = &RenderContext->Programs[Index];
                // NOTE(zoubir): an effect that failed at startup shares the
                // sprite program; only delete an id nobody else holds
                bool32 OwnsOld = (Program->ID != 0);
                for(u32 Other = 0; Other < Shader_Count; Other++)
                {
                    if (Other != Index && RenderContext->Programs[Other].ID == Program->ID)
                    {
                        OwnsOld = false;
                    }
                }
                if (OwnsOld)
                {
                    OpenGL->glDeleteProgram(Program->ID);
                }
                Program->ID = ID;
                Program->NumAttrib = Def->AttributeCount;
                Program->ProjectionLocation = OpenGL->glGetUniformLocation(ID, "P");
                Program->TimeLocation = OpenGL->glGetUniformLocation(ID, "Time");
                if (ShaderLibrary.ErrorShader == Index)
                {
                    ShaderLibrary.Error[0] = 0;
                }
                // NOTE(zoubir): the program in use may have just been
                // replaced; send uniforms again on next use
                RenderContext->AProgramIsUsed = false;
                Result = true;
            }
            else
            {
                snprintf(ShaderLibrary.Error, sizeof(ShaderLibrary.Error),
                         "shader \"%s\": %s", Def->Name, Error);
                ShaderLibrary.ErrorShader = Index;
            }
        }
    }
    else if (Force)
    {
        snprintf(ShaderLibrary.Error, sizeof(ShaderLibrary.Error),
                 "shader \"%s\": cannot read %s", Def->Name,
                 Vertex.Memory ? Def->Fragment : Def->Vertex);
        ShaderLibrary.ErrorShader = Index;
    }
    if (Vertex.Memory)
    {
        Platform.FreeFileMemory(Vertex.Memory);
    }
    if (Fragment.Memory)
    {
        Platform.FreeFileMemory(Fragment.Memory);
    }
    RenderContext->TextureProgram = RenderContext->Programs[Shader_Texture];
    RenderContext->LineProgram = RenderContext->Programs[Shader_Line];
    return Result;
}

// NOTE(zoubir): at startup. The sprite and line programs must load; an
// effect that does not draws with the sprite program instead, so the game
// still runs (and says why on screen)
internal void
LoadShaderLibrary(render_context *RenderContext)
{
    ShaderLibrary = {};
    for(u32 Index = 0; Index < Shader_Count; Index++)
    {
        RenderContext->Programs[Index] = {};
        if (!LoadShaderProgram(RenderContext, Index, true))
        {
            Assert(Index != Shader_Texture && Index != Shader_Line);
            RenderContext->Programs[Index] = RenderContext->Programs[Shader_Texture];
        }
    }
}

// NOTE(zoubir): developer builds, every frame: rebuilds programs whose
// files changed on disk
internal void
UpdateShaderLibrary(render_context *RenderContext, float DeltaTime)
{
#if APP_DEV
    ShaderLibrary.SinceCheck += DeltaTime;
    if (ShaderLibrary.SinceCheck >= SHADER_RELOAD_SECONDS)
    {
        ShaderLibrary.SinceCheck = 0.f;
        for(u32 Index = 0; Index < Shader_Count; Index++)
        {
            LoadShaderProgram(RenderContext, Index, false);
        }
    }
#endif
}

// NOTE(zoubir): the last shader build failure, or 0 when all is well
inline char *
ShaderLibraryError()
{
    char *Result = ShaderLibrary.Error[0] ? ShaderLibrary.Error : 0;
    return Result;
}
