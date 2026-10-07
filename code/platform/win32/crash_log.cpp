/* Crash log: when the game crashes (an access violation, a divide by
   zero, a failed Assert's write to address 0), writes crash.txt in the
   folder the game runs from: the exception, the thread, and the call
   stack, first as module and offset, then as function, file and line, so
   a crash seen once can be found without a debugger. Symbols come from
   the .pdb files the build leaves next to the executable, through
   dbghelp.dll, which every Windows has; it is loaded only after a crash,
   and only once the raw stack is safely on disk. The game code runs from
   app_temp.dll, whose .pdb has a random name, so its frames often stay
   unnamed: look app_temp.dll+offset up in buildpp.map, the function
   with the largest address (less the preferred load address) under it. */

#include <dbghelp.h>

typedef BOOL WINAPI dbghelp_sym_initialize(HANDLE Process, PCSTR SearchPath, BOOL Invade);
typedef DWORD WINAPI dbghelp_sym_set_options(DWORD Options);
typedef BOOL WINAPI dbghelp_sym_from_addr(HANDLE Process, DWORD64 Address,
                                          PDWORD64 Displacement, PSYMBOL_INFO Symbol);
typedef BOOL WINAPI dbghelp_sym_get_line_from_addr64(HANDLE Process, DWORD64 Address,
                                                     PDWORD Displacement, PIMAGEHLP_LINE64 Line);

#define WIN32_CRASH_FRAMES 48

// NOTE(zoubir): appends one formatted line to the open file
internal void
Win32CrashWrite(HANDLE File, char *Format, ...)
{
    char Line[512];
    va_list Args;
    va_start(Args, Format);
    int Length = _vsnprintf_s(Line, sizeof(Line), _TRUNCATE, Format, Args);
    va_end(Args);
    if (Length < 0)
    {
        Length = (int)strlen(Line);
    }
    DWORD Written;
    WriteFile(File, Line, (DWORD)Length, &Written, 0);
}

// NOTE(zoubir): the file name of the module Address is in, and its base
internal char *
Win32CrashModule(void *Address, char *Buffer, u32 Size, u8 **Base)
{
    HMODULE Owner = 0;
    *Base = 0;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)Address, &Owner) &&
        GetModuleFileNameA(Owner, Buffer, Size))
    {
        *Base = (u8 *)Owner;
        char *Slash = strrchr(Buffer, '\\');
        return Slash ? Slash + 1 : Buffer;
    }
    return (char *)"?";
}

internal LONG WINAPI
Win32WriteCrashLog(EXCEPTION_POINTERS *Exception)
{
    HANDLE File = CreateFileA("crash.txt", GENERIC_WRITE, FILE_SHARE_READ, 0,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (File == INVALID_HANDLE_VALUE)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    EXCEPTION_RECORD *Record = Exception->ExceptionRecord;
    char Module[MAX_PATH];
    u8 *Base = 0;
    char *Name = Win32CrashModule(Record->ExceptionAddress, Module, sizeof(Module), &Base);
    Win32CrashWrite(File, "exception 0x%08X in %s+0x%llX, thread %u\r\n",
                    (u32)Record->ExceptionCode, Name,
                    (u64)((u8 *)Record->ExceptionAddress - Base), (u32)GetCurrentThreadId());
    if (Record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && Record->NumberParameters >= 2)
    {
        Win32CrashWrite(File, "%s address %p\r\n",
                        Record->ExceptionInformation[0] ? "writing" : "reading",
                        (void *)Record->ExceptionInformation[1]);
    }

    // NOTE(zoubir): walk the crashed thread's stack from the registers at
    // the crash, one function's unwind data at a time; a function with
    // none (a leaf) has its return address on top of the stack
    void *Frames[WIN32_CRASH_FRAMES];
    u32 Count = 0;
    CONTEXT Context = *Exception->ContextRecord;
    while(Count < WIN32_CRASH_FRAMES && Context.Rip)
    {
        Frames[Count++] = (void *)Context.Rip;
        DWORD64 ImageBase = 0;
        RUNTIME_FUNCTION *Function = RtlLookupFunctionEntry(Context.Rip, &ImageBase, 0);
        if (Function)
        {
            void *HandlerData = 0;
            DWORD64 EstablisherFrame = 0;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, ImageBase, Context.Rip, Function,
                             &Context, &HandlerData, &EstablisherFrame, 0);
        }
        else
        {
            Context.Rip = *(DWORD64 *)Context.Rsp;
            Context.Rsp += 8;
        }
    }
    for(u32 Index = 0; Index < Count; Index++)
    {
        Name = Win32CrashModule(Frames[Index], Module, sizeof(Module), &Base);
        Win32CrashWrite(File, "%2u %s+0x%llX\r\n", Index, Name,
                        (u64)((u8 *)Frames[Index] - Base));
    }
    FlushFileBuffers(File);

    HMODULE DbgHelp = LoadLibraryA("dbghelp.dll");
    if (DbgHelp)
    {
        dbghelp_sym_set_options *SymSetOptions =
            (dbghelp_sym_set_options *)GetProcAddress(DbgHelp, "SymSetOptions");
        dbghelp_sym_initialize *SymInitialize =
            (dbghelp_sym_initialize *)GetProcAddress(DbgHelp, "SymInitialize");
        dbghelp_sym_from_addr *SymFromAddr =
            (dbghelp_sym_from_addr *)GetProcAddress(DbgHelp, "SymFromAddr");
        dbghelp_sym_get_line_from_addr64 *SymGetLineFromAddr64 =
            (dbghelp_sym_get_line_from_addr64 *)GetProcAddress(DbgHelp, "SymGetLineFromAddr64");
        HANDLE Process = GetCurrentProcess();
        if (SymSetOptions)
        {
            SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
        }
        if (SymInitialize && SymFromAddr && SymInitialize(Process, 0, TRUE))
        {
            Win32CrashWrite(File, "\r\n");
            for(u32 Index = 0; Index < Count; Index++)
            {
                DWORD64 Address = (DWORD64)Frames[Index];
                char Function[256] = "?";
                char Where[MAX_PATH + 16] = "";
                u8 Buffer[sizeof(SYMBOL_INFO) + 256] = {};
                SYMBOL_INFO *Symbol = (SYMBOL_INFO *)Buffer;
                Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
                Symbol->MaxNameLen = 255;
                DWORD64 Displacement = 0;
                if (SymFromAddr(Process, Address, &Displacement, Symbol))
                {
                    _snprintf_s(Function, sizeof(Function), _TRUNCATE, "%s", Symbol->Name);
                }
                IMAGEHLP_LINE64 Line = {};
                Line.SizeOfStruct = sizeof(Line);
                DWORD LineDisplacement = 0;
                if (SymGetLineFromAddr64 &&
                    SymGetLineFromAddr64(Process, Address, &LineDisplacement, &Line))
                {
                    _snprintf_s(Where, sizeof(Where), _TRUNCATE, "  %s:%u", Line.FileName,
                                (u32)Line.LineNumber);
                }
                Win32CrashWrite(File, "%2u %s%s\r\n", Index, Function, Where);
            }
        }
    }
    CloseHandle(File);
    return EXCEPTION_CONTINUE_SEARCH;
}

internal void
Win32InstallCrashLog(void)
{
    SetUnhandledExceptionFilter(Win32WriteCrashLog);
}
