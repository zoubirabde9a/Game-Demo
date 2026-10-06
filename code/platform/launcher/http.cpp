/* Downloads over HTTP(S) with WinHTTP, which ships with Windows and uses
   the system's certificates and proxy settings. One call fetches one URL
   into memory (the manifest) or into a file (a game file), hashing the
   bytes as they arrive so the caller can check them. */

global_variable HINTERNET HttpSession;

internal bool32
HttpStart()
{
    HttpSession = WinHttpOpen(L"GameDemoLauncher/1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!HttpSession)
    {
        // NOTE(zoubir): automatic proxy needs Windows 8.1; older ones
        // take the proxy set with netsh
        HttpSession = WinHttpOpen(L"GameDemoLauncher/1", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    }
    if (HttpSession)
    {
        // NOTE(zoubir): resolve, connect, send, receive, in milliseconds
        WinHttpSetTimeouts(HttpSession, 10000, 10000, 30000, 30000);
    }
    return HttpSession != 0;
}

// NOTE(zoubir): where the body of a response goes. Memory gets a 0 after
// the last byte; Done, when set, grows as bytes arrive (the progress bar)
struct http_target
{
    char *Memory;
    u32 MemorySize;
    u32 MemoryUsed;
    HANDLE File;
    sha256 *Hash;
    launcher_status *Progress;
};

internal void
LauncherAddProgress(launcher_status *Status, u64 Bytes)
{
    EnterCriticalSection(&Status->Lock);
    Status->Done += Bytes;
    LeaveCriticalSection(&Status->Lock);
}

// NOTE(zoubir): GETs Url into Target. False on any network error, a
// status other than 200, or a body that does not fit in Memory
internal bool32
HttpGet(char *Url, http_target *Target)
{
    wchar_t WideUrl[LAUNCHER_PATH_COUNT];
    MultiByteToWideChar(CP_UTF8, 0, Url, -1, WideUrl, LAUNCHER_PATH_COUNT);

    wchar_t Host[256];
    wchar_t Path[LAUNCHER_PATH_COUNT];
    URL_COMPONENTS Parts = {};
    Parts.dwStructSize = sizeof(Parts);
    Parts.lpszHostName = Host;
    Parts.dwHostNameLength = ArrayCount(Host);
    Parts.lpszUrlPath = Path;
    Parts.dwUrlPathLength = ArrayCount(Path);
    if (!WinHttpCrackUrl(WideUrl, 0, 0, &Parts))
    {
        return false;
    }

    bool32 Result = false;
    HINTERNET Connection = WinHttpConnect(HttpSession, Host, Parts.nPort, 0);
    HINTERNET Request = 0;
    if (Connection)
    {
        DWORD Flags = WINHTTP_FLAG_REFRESH;
        if (Parts.nScheme == INTERNET_SCHEME_HTTPS)
        {
            Flags |= WINHTTP_FLAG_SECURE;
        }
        Request = WinHttpOpenRequest(Connection, L"GET", Path, 0, WINHTTP_NO_REFERER,
                                     WINHTTP_DEFAULT_ACCEPT_TYPES, Flags);
    }
    if (Request &&
        WinHttpSendRequest(Request, L"Cache-Control: no-cache\r\n", (DWORD)-1L,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(Request, 0))
    {
        DWORD StatusCode = 0;
        DWORD StatusSize = sizeof(StatusCode);
        WinHttpQueryHeaders(Request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &StatusCode, &StatusSize,
                            WINHTTP_NO_HEADER_INDEX);
        Result = (StatusCode == 200);

        static u8 Buffer[64*1024];
        while (Result)
        {
            DWORD Read = 0;
            if (!WinHttpReadData(Request, Buffer, sizeof(Buffer), &Read))
            {
                Result = false;
                break;
            }
            if (Read == 0)
            {
                break;
            }
            if (Target->Hash)
            {
                Sha256Add(Target->Hash, Buffer, Read);
            }
            if (Target->File)
            {
                DWORD Written = 0;
                if (!WriteFile(Target->File, Buffer, Read, &Written, 0) || Written != Read)
                {
                    Result = false;
                }
            }
            if (Target->Memory)
            {
                if (Target->MemoryUsed + Read + 1 > Target->MemorySize)
                {
                    Result = false;
                }
                else
                {
                    memcpy(Target->Memory + Target->MemoryUsed, Buffer, Read);
                    Target->MemoryUsed += Read;
                    Target->Memory[Target->MemoryUsed] = 0;
                }
            }
            if (Target->Progress)
            {
                LauncherAddProgress(Target->Progress, Read);
            }
        }
    }

    if (Request)
    {
        WinHttpCloseHandle(Request);
    }
    if (Connection)
    {
        WinHttpCloseHandle(Connection);
    }
    return Result;
}

// NOTE(zoubir): downloads <base>/files/<hash> to Path and keeps it only
// if its bytes hash to Hash and add up to Size
internal bool32
HttpDownloadFile(launcher_paths *Paths, char *Hash, u64 Size, wchar_t *Path,
                 launcher_status *Progress)
{
    char Url[LAUNCHER_PATH_COUNT];
    _snprintf_s(Url, sizeof(Url), _TRUNCATE, "%sfiles/%s", Paths->BaseUrl, Hash);

    HANDLE File = CreateFileW(Path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, 0);
    if (File == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    sha256 State;
    bool32 Result = Sha256Begin(&State);
    http_target Target = {};
    Target.File = File;
    Target.Hash = &State;
    Target.Progress = Progress;
    if (Result)
    {
        Result = HttpGet(Url, &Target);
    }
    LARGE_INTEGER Written = {};
    GetFileSizeEx(File, &Written);
    CloseHandle(File);

    if (State.Hash)
    {
        char Got[LAUNCHER_HASH_SIZE];
        Sha256End(&State, Got);
        Result = Result && strcmp(Got, Hash) == 0 && (u64)Written.QuadPart == Size;
    }
    if (!Result)
    {
        DeleteFileW(Path);
    }
    return Result;
}
