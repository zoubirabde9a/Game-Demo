/* SHA-256 through Windows' own crypto library (bcrypt), so the launcher
   can check that every file it downloads or reuses is the one the
   manifest names. Hashes are written as 64 lowercase hex digits, the
   same text sha256sum prints on the server. */

struct sha256
{
    BCRYPT_ALG_HANDLE Algorithm;
    BCRYPT_HASH_HANDLE Hash;
};

internal bool32
Sha256Begin(sha256 *State)
{
    *State = {};
    if (BCryptOpenAlgorithmProvider(&State->Algorithm, BCRYPT_SHA256_ALGORITHM, 0, 0) != 0)
    {
        return false;
    }
    if (BCryptCreateHash(State->Algorithm, &State->Hash, 0, 0, 0, 0, 0) != 0)
    {
        BCryptCloseAlgorithmProvider(State->Algorithm, 0);
        State->Algorithm = 0;
        return false;
    }
    return true;
}

internal void
Sha256Add(sha256 *State, void *Data, u32 Size)
{
    BCryptHashData(State->Hash, (PUCHAR)Data, Size, 0);
}

// NOTE(zoubir): writes the hex digest into Hex and frees the state
internal void
Sha256End(sha256 *State, char *Hex)
{
    u8 Digest[32];
    BCryptFinishHash(State->Hash, Digest, sizeof(Digest), 0);
    BCryptDestroyHash(State->Hash);
    BCryptCloseAlgorithmProvider(State->Algorithm, 0);
    *State = {};

    char *Digits = "0123456789abcdef";
    for(u32 Index = 0; Index < 32; ++Index)
    {
        Hex[Index*2 + 0] = Digits[Digest[Index] >> 4];
        Hex[Index*2 + 1] = Digits[Digest[Index] & 15];
    }
    Hex[64] = 0;
}

// NOTE(zoubir): the hash and size of a file on disk; false when it
// cannot be read
internal bool32
Sha256File(wchar_t *Path, char *Hex, u64 *Size)
{
    HANDLE File = CreateFileW(Path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                              0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (File == INVALID_HANDLE_VALUE)
    {
        return false;
    }
    sha256 State;
    bool32 Result = Sha256Begin(&State);
    u64 Total = 0;
    static u8 Buffer[64*1024];
    while (Result)
    {
        DWORD Read = 0;
        if (!ReadFile(File, Buffer, sizeof(Buffer), &Read, 0))
        {
            Result = false;
        }
        else if (Read == 0)
        {
            break;
        }
        else
        {
            Sha256Add(&State, Buffer, Read);
            Total += Read;
        }
    }
    CloseHandle(File);
    if (State.Hash)
    {
        Sha256End(&State, Hex);
    }
    *Size = Total;
    return Result;
}
