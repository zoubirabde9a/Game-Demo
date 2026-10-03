/* Worker threads: the work queue the asset loader uses, and big memory
   allocations. */

struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
};

struct platform_work_queue
{    
    HANDLE SemaphoreHandle;
    
    platform_work_queue_entry Entries[256];    
    u32 volatile CurrentEntryToRead;
    u32 volatile CurrentEntryToWrite;
};

//platform_work_queue *Queue, platform_work_queue_callback *CallBack, void *Data
PLATFORM_ADD_WORK_ENTRY(PlatformAddWorkEntry)
{
    // TODO(zoubir): maybe make this an if stement and do something
    // about it
    // TODO(zoubir): Handle the case where we are out
    // of entries
    u32 NextEntryToWrite = (Queue->CurrentEntryToWrite + 1) %
        ArrayCount(Queue->Entries);
    platform_work_queue_entry *Entry = &Queue->Entries[Queue->CurrentEntryToWrite];
    Entry->Callback = Callback;
    Entry->Data = Data;
    CompletePreviousWritesBeforeFutureWrites;
    Queue->CurrentEntryToWrite = NextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
    platform_work_queue *Queue =
        (platform_work_queue *)lpParameter;

    for(;;)
    {
        u32 CurrentEntryToRead = Queue->CurrentEntryToRead;
        u32 CurrentEntryToWrite = Queue->CurrentEntryToWrite;
        u32 NextEntryToRead = (CurrentEntryToRead + 1) %
                ArrayCount(Queue->Entries);
        if (CurrentEntryToWrite != CurrentEntryToRead)
        {
            u32 Index =
                InterlockedCompareExchange((LONG volatile *)&Queue->CurrentEntryToRead,
                                           NextEntryToRead,
                                           CurrentEntryToRead);
            if(Index == CurrentEntryToRead)
            {
                platform_work_queue_entry *Entry = &Queue->Entries[Index];
                Entry->Callback(Entry->Data);
            }

        }
        else
        {            
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }
    
}

internal void
Win32InitWorkQueue(platform_work_queue *Queue, u32 ThreadCount)
{
    Queue->SemaphoreHandle =
        CreateSemaphoreExA(0, 0, ThreadCount,
                          0, 0, SEMAPHORE_ALL_ACCESS);
    for(u32 ThreadIndex = 0;
        ThreadIndex < ThreadCount;
        ThreadIndex++)
    {
        DWORD ThreadID;
        HANDLE ThreadHandle =
            CreateThread(0, 0, ThreadProc, Queue, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    void *Result = VirtualAlloc(0, Size,
                                MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE);
    return Result;
}

PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
    if (Ptr)
    {
        VirtualFree(Ptr, 0, MEM_RELEASE);
    }
}
