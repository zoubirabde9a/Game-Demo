#if !defined(RENDER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

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

#if 0
struct zi_header
{
    u8 Channels;
    int Width;
    int Height;
};

struct gl_texture
{
    u32 ID;
    u32 Width;
    u32 Height;
};
#endif


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
    RENDER_BATCH_TYPE_RECTANGLE
};

struct render_program
{
    u32 ID;
    u32 NumAttrib;
    mat4 *ProjectionMatrix;
};

struct render_batch
{
    render_vertex *Verticies;
    u32 VertexCount;
    u32 TextureID;
    float SortingValue;
    render_program Program;
    u32 Type;
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
        };
    };

    // for performance only
    bool32 AProgramIsUsed;
    u32 LastUsedProgramID;
};

#define RENDER_H
#endif
