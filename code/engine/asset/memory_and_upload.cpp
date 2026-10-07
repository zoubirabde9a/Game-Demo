/* Asset memory and texture upload: the work item a load thread gets, taking
   and returning asset memory, and the queue of decoded textures waiting
   for the main thread to hand them to OpenGL. */

struct load_asset_work
{
    opengl_texture_queue *Queue;
    assets *Assets;
    asset_id ID;
};

inline void *
AcquireAssetMemory(assets *Assets, memory_index Size)
{
    void *Result = Platform.AllocateMemory(Size);
    if (Result)
    {
        Assets->TotalMemoryUsed += Size;
    }
    return Result;
}

inline void
ReleaseAssetMemory(assets *Assets,
                              memory_index Size, void *Memory)
{
    Platform.DeallocateMemory(Memory);
    if (Memory)
    {
        Assets->TotalMemoryUsed -= Size;
    }
}                              
                                  

internal void
AddOpenglTextureToQueue(opengl_texture_queue *Queue,
                        assets *Assets,
                        asset_id ID,
                        u32 TextureSize,
                        void *TextureMemory, u32 Width,
                        u32 Height, i32 ImageFormat,
                        u32 Flags)
{
    opengl_texture_queue_entry NewEntry = {};
    NewEntry.ID = ID;
    NewEntry.Assets = Assets;
    NewEntry.TextureSize = TextureSize;
    NewEntry.TextureMemory = TextureMemory;
    NewEntry.Width = Width;
    NewEntry.Height = Height;
    NewEntry.ImageFormat = ImageFormat;
    NewEntry.Flags = Flags;

    bool32 DoneOperation = 0;
    while(DoneOperation == 0)
    {
        DoneOperation = 
            AtomicCompareExchangeU32(&Queue->AThreadIsAlreadyWritingAnEntry, 1, 0) == 0;
        if (DoneOperation)
        {        
            u32 NextEntryToWrite =
                (Queue->CurrentEntryToWrite + 1) %
                ArrayCount(Queue->Entries);
            opengl_texture_queue_entry *Entry =
                &Queue->Entries[Queue->CurrentEntryToWrite];
            *Entry = NewEntry;
            CompletePreviousWritesBeforeFutureWrites;
            Queue->CurrentEntryToWrite = NextEntryToWrite;
            Queue->AThreadIsAlreadyWritingAnEntry = 0;            
        }
    }
       
}



// NOTE(zoubir): sends pixels to a new OpenGL texture; the caller keeps the
// memory (LoadOpenglTexture releases asset memory after)
internal loaded_texture
UploadOpenglTexture(open_gl *OpenGL, u32 Width, u32 Height, GLenum ImageFormat,
                    void *TextureMemory, u32 Flags)
{
    loaded_texture Texture;
    Texture.Width = Width;
    Texture.Height = Height;
    
    OpenGL->glGenTextures(1, &Texture.ID);
    OpenGL->glBindTexture(GL_TEXTURE_2D, Texture.ID);

    OpenGL->glTexImage2D(GL_TEXTURE_2D, 0, ImageFormat,
                 Width, Height, 0,
                 ImageFormat, GL_UNSIGNED_BYTE, TextureMemory);

    //Set some texture parameters
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if ((Flags & TEXTURE_SOFT_FILTER) == TEXTURE_SOFT_FILTER) {
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else {
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        OpenGL->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    
    OpenGL->glGenerateMipmap(GL_TEXTURE_2D);
    OpenGL->glBindTexture(GL_TEXTURE_2D, 0);

    return Texture;
}

// NOTE(zoubir): uploads asset memory (from Platform.AllocateMemory, see
// AcquireAssetMemory) and gives it back
internal loaded_texture
LoadOpenglTexture(assets *Assets,
                  open_gl *OpenGL, u32 Width, u32 Height,
                  GLenum ImageFormat, memory_index MemorySize,
                  void *TextureMemory, u32 Flags)
{
    loaded_texture Texture = UploadOpenglTexture(OpenGL, Width, Height, ImageFormat,
                                                 TextureMemory, Flags);
    //IMPORTANT memory leak with texture memory
    ReleaseAssetMemory(Assets, MemorySize, TextureMemory);
    return Texture;
}
