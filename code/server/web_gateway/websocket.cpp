/* WebSocket pieces for the web gateway (RFC 6455), with no state of their
   own so the tests can check them alone: the SHA-1 and base64 that turn a
   browser's Sec-WebSocket-Key into the accept key, reading the opening
   HTTP request, and reading and writing frames. A browser's frames are
   masked and a server's are not; the gateway takes whole binary frames up
   to WS_MAX_PAYLOAD and nothing fragmented. */

#define WS_MAX_PAYLOAD 4096

internal u32
WsRotateLeft(u32 Value, u32 Bits)
{
    return (Value << Bits) | (Value >> (32 - Bits));
}

// NOTE(zoubir): Out gets the 20-byte SHA-1 of Data
internal void
WsSha1(const u8 *Data, u32 Size, u8 *Out)
{
    u32 H[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    u64 BitCount = (u64)Size * 8;
    u32 Total = ((Size + 8) / 64 + 1) * 64;
    for (u32 Block = 0; Block < Total; Block += 64)
    {
        u32 W[80];
        for (u32 Index = 0; Index < 16; Index++)
        {
            u32 Word = 0;
            for (u32 Byte = 0; Byte < 4; Byte++)
            {
                u32 At = Block + Index * 4 + Byte;
                u8 Value = 0;
                if (At < Size) Value = Data[At];
                else if (At == Size) Value = 0x80;
                else if (At >= Total - 8) Value = (u8)(BitCount >> (8 * (Total - 1 - At)));
                Word = (Word << 8) | Value;
            }
            W[Index] = Word;
        }
        for (u32 Index = 16; Index < 80; Index++)
        {
            W[Index] = WsRotateLeft(W[Index - 3] ^ W[Index - 8] ^ W[Index - 14] ^ W[Index - 16], 1);
        }
        u32 A = H[0], B = H[1], C = H[2], D = H[3], E = H[4];
        for (u32 Index = 0; Index < 80; Index++)
        {
            u32 F, K;
            if (Index < 20) { F = (B & C) | (~B & D); K = 0x5A827999; }
            else if (Index < 40) { F = B ^ C ^ D; K = 0x6ED9EBA1; }
            else if (Index < 60) { F = (B & C) | (B & D) | (C & D); K = 0x8F1BBCDC; }
            else { F = B ^ C ^ D; K = 0xCA62C1D6; }
            u32 Next = WsRotateLeft(A, 5) + F + E + K + W[Index];
            E = D; D = C; C = WsRotateLeft(B, 30); B = A; A = Next;
        }
        H[0] += A; H[1] += B; H[2] += C; H[3] += D; H[4] += E;
    }
    for (u32 Index = 0; Index < 20; Index++)
    {
        Out[Index] = (u8)(H[Index / 4] >> (24 - 8 * (Index % 4)));
    }
}

// NOTE(zoubir): writes Size bytes as base64 with padding, and a 0
internal void
WsBase64(const u8 *Data, u32 Size, char *Out)
{
    const char *Digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    u32 At = 0;
    for (u32 Index = 0; Index < Size; Index += 3)
    {
        u32 Bits = (u32)Data[Index] << 16;
        if (Index + 1 < Size) Bits |= (u32)Data[Index + 1] << 8;
        if (Index + 2 < Size) Bits |= Data[Index + 2];
        Out[At++] = Digits[(Bits >> 18) & 63];
        Out[At++] = Digits[(Bits >> 12) & 63];
        Out[At++] = (Index + 1 < Size) ? Digits[(Bits >> 6) & 63] : '=';
        Out[At++] = (Index + 2 < Size) ? Digits[Bits & 63] : '=';
    }
    Out[At] = 0;
}

// NOTE(zoubir): the Sec-WebSocket-Accept answer to a Sec-WebSocket-Key;
// Out needs 29 bytes
internal void
WsAcceptKey(const char *Key, char *Out)
{
    char Joined[128];
    snprintf(Joined, sizeof(Joined), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", Key);
    u8 Hash[20];
    WsSha1((const u8 *)Joined, (u32)strlen(Joined), Hash);
    WsBase64(Hash, 20, Out);
}

struct ws_request
{
    char Method[8];
    char Path[256];
    char Key[64];      // Sec-WebSocket-Key, "" when not asking for a WebSocket
    bool32 Upgrade;    // Upgrade: websocket
};

// NOTE(zoubir): copies the header value after Name into Out when Line is
// that header, names compared without case
internal bool32
WsHeaderValue(const char *Line, const char *Name, char *Out, u32 OutSize)
{
    u32 Length = (u32)strlen(Name);
    for (u32 Index = 0; Index < Length; Index++)
    {
        if (tolower((unsigned char)Line[Index]) != tolower((unsigned char)Name[Index])) return false;
    }
    if (Line[Length] != ':') return false;
    const char *Value = Line + Length + 1;
    while (*Value == ' ' || *Value == '\t') Value++;
    u32 Count = 0;
    while (Value[Count] && Value[Count] != '\r' && Value[Count] != '\n' && Count + 1 < OutSize)
    {
        Out[Count] = Value[Count];
        Count++;
    }
    Out[Count] = 0;
    return true;
}

// NOTE(zoubir): reads the request line and headers of Text, a complete
// request ending in a blank line
internal bool32
WsParseRequest(const char *Text, ws_request *Out)
{
    *Out = {};
    const char *At = Text;
    u32 Length = 0;
    while (*At && *At != ' ' && Length + 1 < sizeof(Out->Method)) Out->Method[Length++] = *At++;
    if (*At++ != ' ') return false;
    Length = 0;
    while (*At && *At != ' ' && *At != '\r' && Length + 1 < sizeof(Out->Path)) Out->Path[Length++] = *At++;
    if (*At != ' ' || !Length) return false;
    for (const char *Line = strchr(Text, '\n'); Line; Line = strchr(Line, '\n'))
    {
        Line++;
        char Value[128];
        WsHeaderValue(Line, "Sec-WebSocket-Key", Out->Key, sizeof(Out->Key));
        if (WsHeaderValue(Line, "Upgrade", Value, sizeof(Value)))
        {
            for (char *C = Value; *C; C++) *C = (char)tolower((unsigned char)*C);
            Out->Upgrade = (strstr(Value, "websocket") != 0);
        }
    }
    return true;
}

enum ws_read_result
{
    WsRead_NeedMore,
    WsRead_Frame,
    WsRead_Bad, // too large, fragmented or unmasked: close the connection
};

struct ws_frame
{
    u8 Opcode; // 1 text, 2 binary, 8 close, 9 ping, 10 pong
    u32 Size;
    u8 Payload[WS_MAX_PAYLOAD];
};

// NOTE(zoubir): reads one frame from the front of Data; *Used is how many
// bytes it took
internal ws_read_result
WsReadFrame(const u8 *Data, u32 Size, ws_frame *Frame, u32 *Used)
{
    if (Size < 2) return WsRead_NeedMore;
    bool32 Final = (Data[0] & 0x80) != 0;
    u8 Opcode = Data[0] & 0x0f;
    bool32 Masked = (Data[1] & 0x80) != 0;
    u64 Length = Data[1] & 0x7f;
    u32 At = 2;
    if (Length == 126)
    {
        if (Size < 4) return WsRead_NeedMore;
        Length = ((u32)Data[2] << 8) | Data[3];
        At = 4;
    }
    else if (Length == 127)
    {
        if (Size < 10) return WsRead_NeedMore;
        Length = 0;
        for (u32 Index = 0; Index < 8; Index++) Length = (Length << 8) | Data[2 + Index];
        At = 10;
    }
    if (!Final || !Masked || Opcode == 0 || Length > WS_MAX_PAYLOAD) return WsRead_Bad;
    if (Size < At + 4 + Length) return WsRead_NeedMore;
    const u8 *Mask = Data + At;
    At += 4;
    for (u32 Index = 0; Index < (u32)Length; Index++)
    {
        Frame->Payload[Index] = Data[At + Index] ^ Mask[Index & 3];
    }
    Frame->Opcode = Opcode;
    Frame->Size = (u32)Length;
    *Used = At + (u32)Length;
    return WsRead_Frame;
}

// NOTE(zoubir): a server frame (unmasked) into Out, which needs Size + 4
// bytes; returns its length
internal u32
WsWriteFrame(u8 Opcode, const u8 *Payload, u32 Size, u8 *Out)
{
    u32 At = 0;
    Out[At++] = (u8)(0x80 | Opcode);
    if (Size < 126)
    {
        Out[At++] = (u8)Size;
    }
    else
    {
        Out[At++] = 126;
        Out[At++] = (u8)(Size >> 8);
        Out[At++] = (u8)Size;
    }
    if (Size) memcpy(Out + At, Payload, Size);
    return At + Size;
}
