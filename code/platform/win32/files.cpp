/* Files: path helpers, reading and writing whole files, and the
   asset-pack file API the game reads through platform_api. */

void
CatStrings(size_t SourceALength, char *SourceA,
           size_t SourceBLength, char *SourceB,
           size_t DestCount, char *Dest)
{
    for(size_t i = 0;
        i < SourceALength;
        i++)
    {
        *Dest++ = SourceA[i];
    }

    for(size_t i = 0;
        i < SourceBLength;
        i++)
    {
        *Dest++ = SourceB[i];
    }
    *Dest = 0;
}

internal void
Win32GetExecutableFileName(win32_state *state)
{
    DWORD sizeOfFileName =
        GetModuleFileName(0, state->executableFilePath, sizeof(state->executableFilePath));
    char *onePastLastSlash = state->executableFilePath;
    for(char *scan = onePastLastSlash;
        *scan;
        scan++)
    {
        if (*scan == '\\')
        {
            onePastLastSlash = scan + 1;
        }      
    }
    state->executableFileName = onePastLastSlash;
}

internal size_t
StringLength(char *string)
{
    size_t length = 0;
    while(*string++)
    {
        length++;
    }
    return length;
}

internal void
Win32BuildExecutablePathFileName(win32_state *state, char *fileName,
                                 size_t destLength, char *dest)
{
    CatStrings(state->executableFileName - state->executableFilePath, state->executableFilePath,
               StringLength(fileName), fileName,
               destLength, dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result result = {};
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_READ,
                                    FILE_SHARE_READ, 0,
                                    OPEN_EXISTING, 0, 0);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        // NOTE(zoubir): a missing file is a normal answer (the font
        // loader tries several paths); callers check result.Memory
        return result;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize))
    {
        Assert(0);
        return result;
    }

    u32 fileSize32 = safeTruncateU32(fileSize.QuadPart);
    result.Memory = VirtualAlloc(0, fileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!result.Memory)
    {
        Assert(0);
        return result;
    }

    DWORD bytesRead;
    if (ReadFile(fileHandle, result.Memory, fileSize32, &bytesRead, 0) &&
        bytesRead == fileSize32)
    {
        result.Size = fileSize32;
    }
    else
    {
        DEBUGPlatformFreeFileMemory(result.Memory);
        result.Memory = 0;
    }

    CloseHandle(fileHandle);
    return result;                               
}

inline void *
Win32AllocateSize(u32 Size)
{
     void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
//     ZeroSize(Result, Size);
     return Result;
}

#define Win32AllocateStruct(Type) (Type *)(Win32AllocateSize(sizeof(Type)))
inline void
Win32Free(void *Memory)
{
    VirtualFree(Memory, 0, MEM_RELEASE);
}

struct win32_platform_file_handle
{
    platform_file_handle H;
    HANDLE Handle;
};

struct win32_platform_file_group
{
    platform_file_group H;
    HANDLE FindHandle;
    WIN32_FIND_DATAA FindData;
};

internal
PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(Win32GetAllFilesOfTypeBegin)
{
    win32_platform_file_group *Win32FileGroup =
        Win32AllocateStruct(win32_platform_file_group);

    char *TypeAt = Type;
    char WildCard[32] = "*.";
    for(u32 WildCardIndex = 2;
        WildCardIndex < sizeof(WildCard);
        WildCardIndex++)
    {
        WildCard[WildCardIndex] = *TypeAt;
        if (*TypeAt == '\0')
        {
            break;
        }
        TypeAt++;
    }
    WildCard[sizeof(WildCard) - 1] = '\0';

    Win32FileGroup->H.FileCount = 0;

    WIN32_FIND_DATA FindData;
    HANDLE FindHandle = FindFirstFileA(WildCard, &FindData);
    while(FindHandle != INVALID_HANDLE_VALUE)
    {
        Win32FileGroup->H.FileCount++;

        if (!FindNextFileA(FindHandle, &FindData))
        {
            break;
        }
    }
    FindClose(FindHandle);

    Win32FileGroup->FindHandle = FindFirstFileA(WildCard, &Win32FileGroup->FindData);
    
    return (platform_file_group *)Win32FileGroup;
}

internal
PLATFORM_GET_ALL_FILES_OF_TYPE_END(Win32GetAllFilesOfTypeEnd)
{
    win32_platform_file_group *Win32FileGroup =
        (win32_platform_file_group *)(FileGroup);
    if(Win32FileGroup)
    {
        FindClose(Win32FileGroup->FindHandle);

        Win32Free(Win32FileGroup);
    }

}

internal
PLATFORM_OPEN_NEXT_FILE(Win32OpenNextFile)
{
    win32_platform_file_group *Win32FileGroup =
        (win32_platform_file_group *)FileGroup;
    win32_platform_file_handle *Result = 0;

    if (Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE)
    {
        Result = Win32AllocateStruct(win32_platform_file_handle);
        if(Result)
        {
            char *FileName = Win32FileGroup->FindData.cFileName;
            Result->Handle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            Result->H.HasErrors = (Result->Handle == INVALID_HANDLE_VALUE);
        }
        
        if(!FindNextFileA(Win32FileGroup->FindHandle, &Win32FileGroup->FindData))
        {
            FindClose(Win32FileGroup->FindHandle);
            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
        }        
    }

    return (platform_file_handle *)Result;
}

internal
PLATFORM_FILE_ERROR(Win32FileError)
{
#if HANDMADE_INTERNAL
    OutputDebugString("WIN32 FILE ERROR: ");
    OutputDebugString(Message);
    OutputDebugString("\n");
#endif
    
    Handle->HasErrors = true;
    
}

internal
PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
    if(PlatformNoFileErrors(Source))
    {
        win32_platform_file_handle *Handle =
            (win32_platform_file_handle *)Source;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);
    
        u32 FileSize32 = (u32)(Size);

        Assert(FileSize32 == Size) ;
        
        DWORD BytesRead;
        if(ReadFile(Handle->Handle, Dest, FileSize32, &BytesRead, &Overlapped) &&
           (FileSize32 == BytesRead))
        {
            // NOTE(Zoubir): File read succeeded!
        }
        else
        {
            Win32FileError(&Handle->H, "Read file failed.");
        }
    }
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 result = false;
    HANDLE fileHandle = CreateFileA(fileName, GENERIC_WRITE,
                                    0, 0,
                                    CREATE_ALWAYS, 0, 0);
    if (fileHandle == INVALID_HANDLE_VALUE)
        return result;

    DWORD bytesWritten;
    if (WriteFile(fileHandle, memory, memorySize, &bytesWritten, 0))
    {
        result = bytesWritten == memorySize;
    }
    CloseHandle(fileHandle);
    return result;         
}
                             

HRESULT WINAPI
DirectSoundCreate(LPCGUID pcGuidDevice,
                  LPDIRECTSOUND *ppDS,
                  LPUNKNOWN pUnkOuter);

void*
PlatformLoadFile(char *fileName)
{
    return 0;
}
