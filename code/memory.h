#if !defined(MEMORY_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Zoubir $
   ======================================================================== */

#define MEMORY_H

typedef size_t memory_index;

struct memory_arena
{
    u8 *Base;
    memory_index Size;
    memory_index Used;
    u32 TempCount;
};

struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
};

inline void
InitializeArena(memory_arena *Arena,
                      memory_index *Base, memory_index Size)
{
    Arena->Base = (u8 *)Base;
    Arena->Size = Size;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

inline memory_index
GetAlignmentOffset(memory_arena *Arena, memory_index Alignment)
{
    memory_index Offset = 0;
    memory_index Ptr = (memory_index)Arena->Base + Arena->Used;
    memory_index Mask = Alignment - 1;
    if (Ptr & Mask)
    {
        Offset = Alignment - (Ptr & Mask);
    }
    
    return Offset;
}

inline memory_index
GetArenaSizeRemaining(memory_arena *Arena, memory_index Alignment)
{
    memory_index SizeRemaining;
    SizeRemaining = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));
    return SizeRemaining;
}

inline void *
AllocateSize(memory_arena *Arena, memory_index PrefSize, memory_index Alignment = 4)
{
    memory_index Size = PrefSize;

    memory_index AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;

    Assert(Arena->Size >=(Arena->Used + Size));
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;

    return Result;
}

#define AllocateStruct(Arena, Type, ...) (Type *)AllocateSize(Arena, sizeof(Type), ##__VA_ARGS__)
#define AllocateArray(Arena, Count, Type, ...) (Type *)AllocateSize(Arena, (Count) * sizeof(Type), ##__VA_ARGS__)

inline char *
AllocateString(memory_arena *Arena, char *Source)
{
    memory_index StringSize = strlen(Source) + 1;
    char *Dest = AllocateArray(Arena, StringSize, char);
    memcpy(Dest, Source, StringSize);
    return Dest;
}


inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;

    Result.Arena = Arena;
    Result.Used = Arena->Used;
    Arena->TempCount++;

    return Result;
}

inline void
ZeroSize(void *Ptr, memory_index Size)
{
    #if 0
    u8 *BytePtr = (u8 *)Ptr;
    while(Size--)
    {
        *BytePtr++ = 0;
    }
    #endif
    memset(Ptr, 0, Size);
}
#define ZeroArray(Array, Count, Type) ZeroSize(Array, (Count) * sizeof(Type))

inline void
CopyString(char *Dest, memory_index DestSize, char *Src)
{
    while(*Src)
    {        
        *Dest = *Src++;
        DestSize--;
        if (DestSize == 0)
        {
            break;
        }
    }
}

inline void
EndTemporaryMemory(temporary_memory TempMemory)
{
    memory_arena *Arena = TempMemory.Arena;
    Assert(Arena->Used >= TempMemory.Used);
    Assert(Arena->TempCount > 0);

    memory_index MemoryFreed = Arena->Used - TempMemory.Used;
    Arena->Used = TempMemory.Used;    
    Arena->TempCount--;
    ZeroSize(Arena->Base + Arena->Used, MemoryFreed);
}

inline void
SubArena(memory_arena *Dest, memory_arena *Src,
         memory_index Size, memory_index Alignment = 16)
{
    Dest->Size = Size;
    Dest->Base = (u8 *)AllocateSize(Src, Size, Alignment);
    Dest->Used = 0;
    Dest->TempCount = 0;
}

#endif
