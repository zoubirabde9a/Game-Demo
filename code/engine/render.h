#if !defined(RENDER_H)
/* Render types: the vertex, batch, shader program and render_context
   that every draw goes through, plus the RGBA8 color helpers. shader_id
   lists every shader the game draws with; a new effect needs a row in
   ShaderDefs, a line here and its .frag file. The functions live in
   render.cpp and the files it includes. */

struct app_memory;

struct ColorRGBA8
{
    union{
        struct {
            u8 R;
            u8 G;
            u8 B;
            u8 A;
        };
        u32 ColorU32;
    };
};

#define RGBA8_BLACK (0xFF000000)
#define RGBA8_WHITE (0xFFFFFFFF)
#define RGBA8_RED (0xFF0000FF)
#define RGBA8_GREEN (0xFF00FF00)
#define RGBA8_BLUE (0xFFFF0000)
#define RGBA8_YELLOW (0xFF00FFFF)

struct render_vertex
{
    float X, Y, Z;
    union
    {
        u32 Color;
        struct
        {
            u8 R, G, B, A;
        };
        
    };
    float U, V;
};

enum render_batch_type
{
    RENDER_BATCH_TYPE_TEXTURE,
    RENDER_BATCH_TYPE_RECTANGLE,
    RENDER_BATCH_TYPE_FILLED_RECTANGLE
};

struct render_program
{
    u32 ID;
    u32 NumAttrib;
    mat4 *ProjectionMatrix;
    // NOTE(zoubir): uniform locations, -1 when the shader has none
    i32 ProjectionLocation;
    i32 TimeLocation;
};

// NOTE(zoubir): every shader program the game draws with, one row each in
// ShaderDefs (engine/render/shader_library.cpp), which says where its
// files are. A new effect is a row there, a line here and its .frag file
enum shader_id
{
    Shader_Texture,
    Shader_Line,
    Shader_SlotFrame,     // ability slot: rounded frame, ready glow and sheen
    Shader_CooldownSweep, // ability slot: the dark clock sweep over the icon
    Shader_Glow,          // soft round light, usually drawn additive
    Shader_Ring,          // thin bright ring, usually drawn additive
    Shader_Panel,         // rounded translucent panel with a lit top edge
    Shader_TimeWarp,      // the time rewinds' post-process over the world
    Shader_TimeSigil,     // a clock face of light under a rewind's caster
    Shader_TalentNode,    // a talent's round medallion: locked, open or full
    Shader_XpBar,         // the experience bar's track and liquid fill
    Shader_TalentBackdrop, // a talent branch's glowing column
    Shader_TalentArc,     // a progress ring filling clockwise from the top
    Shader_ScreenEdge,    // vignette and hurt tint over the whole screen
    Shader_WorldGrade,    // glow and colour grade over the world, every frame
    Shader_RoundRect,     // small raised rounded part: bar, button, field, chip
    Shader_RoundOutline,  // the rim of a rounded part, for focus
    Shader_GroundCrack,   // broken ground a Launch or slam leaves, client/ground_cracks.cpp
    Shader_WaterSurface,  // moving light on water, client/ground/water_surface.cpp
    Shader_Count
};

// NOTE(zoubir): how a batch mixes with what is under it
enum render_blend
{
    RenderBlend_Alpha,    // ordinary see-through
    RenderBlend_Additive, // adds light: glows, flashes, sparks
};

struct render_batch
{
    render_vertex *Verticies;
    u32 VertexCount;
    u32 TextureID;
    float SortingValue;
    render_program Program;
    u32 Type;
    u32 Blend; // render_blend
};

enum render_order_type
{
    RENDER_ORDER_NO_ORDER,
    RENDER_ORDER_BACK_TO_FRONT,
    RENDER_ORDER_FRONT_TO_BACK,
    RENDER_ORDER_COUNT
};

enum renderer_type
{
    RENDERER_TYPE_DEFAULT,
    RENDERER_TYPE_BATCH,
    RENDERER_TYPE_COUNT
};

struct render_context
{
    open_gl *OpenGL;
#if APP_DEV
    bool32 Began;
#endif
    struct memory_arena *Arena;
    render_program TextureProgram;
    render_program LineProgram;
    // NOTE(zoubir): all programs by shader_id; TextureProgram and
    // LineProgram are copies of the first two
    render_program Programs[Shader_Count];
    // NOTE(zoubir): seconds since the game started, the Time uniform
    float Time;
    
    render_vertex *AllocatedVerticies;
    u32 AllocatedVertexCount;
    u32 VertexCount;
    u32 VBO;
    u32 VAO;
    u32 OrderType;
    u32 RendererType;
    union
    {
        // Default Renderer
        struct
        {
            u32 Texture;
            render_program Program;
        };
        // Batch Renderer
        struct
        {
            render_batch *AllocatedBatches;
            render_vertex *TemporaryVerticies;
            u32 AllocatedBatchCount;
            u32 BatchCount;
            // NOTE(zoubir): between BeginBatch and EndBatch; the open
            // batch sits at AllocatedBatches[BatchCount]
            bool32 BatchOpen;
        };
    };

    // for performance only
    bool32 AProgramIsUsed;
    u32 LastUsedProgramID;
    mat4 *LastProjection;
    u32 CurrentBlend;
};

// NOTE(zoubir): engine/shader_library.cpp
internal void LoadShaderLibrary(render_context *RenderContext);
internal void UpdateShaderLibrary(render_context *RenderContext, float DeltaTime);

#define RENDER_H
#endif
