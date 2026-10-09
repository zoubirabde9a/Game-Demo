/* Browser files and memory: the platform calls that read the files
   bundled into the page (web/build.bat packs asset_1.zas, shaders/,
   fonts/ and the hero sheets), memory, the work queue (run on the spot:
   the page has one thread), and the clock.

   The game saves its settings (server.txt, keys.txt) with fopen into the
   current folder, which in the browser lives in memory and is gone on
   reload. WebKeepSavedFiles puts them back from the browser's
   localStorage at startup and copies any file the game writes there
   back into localStorage a moment later. */

DEBUG_PLATFORM_READ_ENTIRE_FILE(WebReadEntireFile)
{
    debug_read_file_result Result = {};
    FILE *File = fopen(fileName, "rb");
    if (File)
    {
        fseek(File, 0, SEEK_END);
        long Size = ftell(File);
        fseek(File, 0, SEEK_SET);
        void *Memory = Size > 0 ? malloc((size_t)Size) : 0;
        if (Memory && fread(Memory, (size_t)Size, 1, File) == 1)
        {
            Result.Memory = Memory;
            Result.Size = (u32)Size;
        }
        else if (Memory)
        {
            free(Memory);
        }
        fclose(File);
    }
    return Result;
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(WebFreeFileMemory)
{
    free(memory);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(WebWriteEntireFile)
{
    bool32 Result = false;
    FILE *File = fopen(fileName, "wb");
    if (File)
    {
        Result = (fwrite(memory, memorySize, 1, File) == 1);
        fclose(File);
    }
    return Result;
}

internal PLATFORM_ALLOCATE_MEMORY(WebAllocateMemory)
{
    return calloc(1, Size);
}

internal PLATFORM_DEALLOCATE_MEMORY(WebDeallocateMemory)
{
    free(Ptr);
}

// NOTE(zoubir): no worker threads in the page; the work runs now
internal PLATFORM_ADD_WORK_ENTRY(WebAddWorkEntry)
{
    Callback(Data);
}

internal PLATFORM_WALL_SECONDS(WebWallSeconds)
{
    return emscripten_get_now() / 1000.0;
}

// NOTE(zoubir): the asset pack is the only file group the game opens
struct web_file_handle
{
    platform_file_handle H;
    FILE *Handle;
};

internal PLATFORM_GET_ALL_FILES_OF_TYPE_BEGIN(WebGetAllFilesOfTypeBegin)
{
    platform_file_group *Group = (platform_file_group *)calloc(1, sizeof(platform_file_group));
    Group->FileCount = 1;
    return Group;
}

internal PLATFORM_GET_ALL_FILES_OF_TYPE_END(WebGetAllFilesOfTypeEnd)
{
    free(FileGroup);
}

internal PLATFORM_OPEN_NEXT_FILE(WebOpenNextFile)
{
    web_file_handle *Result = (web_file_handle *)calloc(1, sizeof(web_file_handle));
    Result->Handle = fopen("asset_1.zas", "rb");
    Result->H.HasErrors = (Result->Handle == 0);
    return (platform_file_handle *)Result;
}

internal PLATFORM_FILE_ERROR(WebFileError)
{
    printf("file error: %s\n", Error);
    Handle->HasErrors = true;
}

internal PLATFORM_READ_DATA_FROM_FILE(WebReadDataFromFile)
{
    if (PlatformNoFileErrors(Source))
    {
        web_file_handle *Handle = (web_file_handle *)Source;
        fseek(Handle->Handle, (long)Offset, SEEK_SET);
        if (fread(Dest, (size_t)Size, 1, Handle->Handle) != 1)
        {
            WebFileError(Source, (char *)"read past the end of the asset pack");
        }
    }
}

internal void
WebFillPlatformApi(platform_api *Api)
{
    Api->GetAllFilesOfTypeBegin = WebGetAllFilesOfTypeBegin;
    Api->GetAllFilesOfTypeEnd = WebGetAllFilesOfTypeEnd;
    Api->OpenNextFile = WebOpenNextFile;
    Api->ReadDataFromFile = WebReadDataFromFile;
    Api->FileError = WebFileError;
    Api->AllocateMemory = WebAllocateMemory;
    Api->DeallocateMemory = WebDeallocateMemory;
    Api->ReadEntireFile = WebReadEntireFile;
    Api->FreeFileMemory = WebFreeFileMemory;
    Api->WriteEntireFile = WebWriteEntireFile;
    Api->AddWorkEntry = WebAddWorkEntry;
    Api->WallSeconds = WebWallSeconds;
}

// NOTE(zoubir): the files the game saves; anything else in the folder
// came with the page
EM_JS(void, WebKeepSavedFiles, (), {
    var Names = ["server.txt", "keys.txt"];
    var Prefix = "game-demo/";
    var Kept = {};
    for (var Index = 0; Index < Names.length; Index++)
    {
        var Name = Names[Index];
        try
        {
            var Text = localStorage.getItem(Prefix + Name);
            if (Text !== null) FS.writeFile(Name, Text);
            Kept[Name] = Text;
        }
        catch (Ignored) {}
    }
    setInterval(function()
    {
        for (var Index = 0; Index < Names.length; Index++)
        {
            var Name = Names[Index];
            try
            {
                var Text = FS.readFile(Name, {encoding: "utf8"});
                if (Text !== Kept[Name])
                {
                    localStorage.setItem(Prefix + Name, Text);
                    Kept[Name] = Text;
                }
            }
            catch (Ignored) {}
        }
    }, 1000);
});
