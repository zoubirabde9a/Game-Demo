#if !defined(NET_ADDRESS_H)
#define NET_ADDRESS_H
/* An IPv4 address and port, shared by the socket layer and the client table. */

#include "../app_defs.h"

struct net_address
{
    u32 Ip;   // host byte order: 127.0.0.1 is 0x7f000001
    u16 Port;
};

inline bool32
NetAddressEqual(net_address A, net_address B)
{
    return A.Ip == B.Ip && A.Port == B.Port;
}

// Parses "a.b.c.d:port". Returns false on anything else.
inline bool32
NetParseAddress(const char *Text, net_address *Out)
{
    u32 Parts[5] = {};
    u32 PartCount = 0;
    bool32 HasDigit = false;
    for (const char *At = Text;; ++At)
    {
        char C = *At;
        if (C >= '0' && C <= '9')
        {
            Parts[PartCount] = Parts[PartCount] * 10 + (u32)(C - '0');
            if (Parts[PartCount] > 65535) return false;
            HasDigit = true;
        }
        else if ((C == '.' && PartCount < 3) || (C == ':' && PartCount == 3) || (C == 0 && PartCount == 4))
        {
            if (!HasDigit) return false;
            HasDigit = false;
            ++PartCount;
            if (C == 0) break;
        }
        else return false;
    }

    for (u32 Index = 0; Index < 4; ++Index)
    {
        if (Parts[Index] > 255) return false;
    }
    Out->Ip = (Parts[0] << 24) | (Parts[1] << 16) | (Parts[2] << 8) | Parts[3];
    Out->Port = (u16)Parts[4];
    return true;
}

#endif
