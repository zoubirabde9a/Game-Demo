/* Sound files (sounds.cpp): a recorded sound effect read from
   sfx/<name>.wav in the folder the game runs from, used in place of its
   made-in-code recipe. The files are CC0 recordings from Kenney's packs
   (build/sfx/CREDITS.txt says which), cut and levelled with ffmpeg into
   the one format read here: WAV, 16-bit PCM, one channel, SFX_RATE
   samples a second. A missing file, or one in any other format, leaves
   the recipe's sound, so a build without the folder (the browser one)
   still has every sound. */

#define SFX_FILE_MAX_SECONDS 4.f

inline u32
SfxFileU32(u8 *At)
{
    u32 Result = (u32)At[0] | ((u32)At[1] << 8) | ((u32)At[2] << 16) | ((u32)At[3] << 24);
    return Result;
}

inline u32
SfxFileU16(u8 *At)
{
    u32 Result = (u32)At[0] | ((u32)At[1] << 8);
    return Result;
}

inline bool32
SfxFileTag(u8 *At, char *Tag)
{
    bool32 Result = At[0] == (u8)Tag[0] && At[1] == (u8)Tag[1] &&
        At[2] == (u8)Tag[2] && At[3] == (u8)Tag[3];
    return Result;
}

// NOTE(zoubir): the samples of sfx/<Name>.wav, copied into Permanent, and
// how many in *Count; 0 when there is no such file or it is not the
// format above
internal i16 *
LoadSoundFile(char *Name, memory_arena *Permanent, u32 *Count)
{
    char Path[128];
    snprintf(Path, sizeof(Path), "sfx/%s.wav", Name);
    debug_read_file_result File = Platform.ReadEntireFile(Path);
    i16 *Result = 0;
    if (!File.Memory)
    {
        return Result;
    }
    u8 *Bytes = (u8 *)File.Memory;
    u32 Size = File.Size;
    bool32 Format = false;
    if (Size >= 12 && SfxFileTag(Bytes, "RIFF") && SfxFileTag(Bytes + 8, "WAVE"))
    {
        u32 At = 12;
        while (At + 8 <= Size)
        {
            u32 ChunkSize = SfxFileU32(Bytes + At + 4);
            u8 *Chunk = Bytes + At + 8;
            if (ChunkSize > Size - At - 8)
            {
                break;
            }
            if (SfxFileTag(Bytes + At, "fmt ") && ChunkSize >= 16)
            {
                Format = SfxFileU16(Chunk) == 1 && SfxFileU16(Chunk + 2) == 1 &&
                    SfxFileU32(Chunk + 4) == SFX_RATE && SfxFileU16(Chunk + 14) == 16;
            }
            else if (SfxFileTag(Bytes + At, "data") && Format)
            {
                u32 Samples = ChunkSize / 2;
                if (Samples > 0 && Samples <= (u32)(SFX_FILE_MAX_SECONDS * SFX_RATE))
                {
                    Result = AllocateArray(Permanent, Samples, i16);
                    for(u32 Index = 0; Index < Samples; Index++)
                    {
                        Result[Index] = (i16)SfxFileU16(Chunk + 2 * Index);
                    }
                    *Count = Samples;
                }
                break;
            }
            // NOTE(zoubir): chunks are padded to an even size
            At += 8 + ChunkSize + (ChunkSize & 1);
        }
    }
    Platform.FreeFileMemory(File.Memory);
    return Result;
}
