/* Winsock version of socket.h. */

#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

internal bool32
NetSocketsStartup()
{
    WSADATA Data;
    return WSAStartup(MAKEWORD(2, 2), &Data) == 0;
}

internal void
NetSocketsShutdown()
{
    WSACleanup();
}

internal net_socket
NetOpenSocket(u16 Port)
{
    net_socket Result = {};
    SOCKET Handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (Handle == INVALID_SOCKET) return Result;

    sockaddr_in Bind = {};
    Bind.sin_family = AF_INET;
    Bind.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind.sin_port = htons(Port);
    u_long NonBlocking = 1;
    if (bind(Handle, (sockaddr *)&Bind, sizeof(Bind)) != 0 ||
        ioctlsocket(Handle, FIONBIO, &NonBlocking) != 0)
    {
        closesocket(Handle);
        return Result;
    }

    Result.Handle = (u64)Handle;
    Result.Open = true;
    return Result;
}

internal void
NetCloseSocket(net_socket *Socket)
{
    if (Socket->Open) closesocket((SOCKET)Socket->Handle);
    *Socket = {};
}

internal u16
NetSocketPort(net_socket *Socket)
{
    sockaddr_in Address = {};
    int Length = sizeof(Address);
    if (!Socket->Open || getsockname((SOCKET)Socket->Handle, (sockaddr *)&Address, &Length) != 0) return 0;
    return ntohs(Address.sin_port);
}

internal bool32
NetSendTo(net_socket *Socket, net_address To, u8 *Data, u32 Size)
{
    if (!Socket->Open) return false;
    sockaddr_in Address = {};
    Address.sin_family = AF_INET;
    Address.sin_addr.s_addr = htonl(To.Ip);
    Address.sin_port = htons(To.Port);
    int Sent = sendto((SOCKET)Socket->Handle, (char *)Data, (int)Size, 0, (sockaddr *)&Address, sizeof(Address));
    return Sent == (int)Size;
}

internal u32
NetReceiveFrom(net_socket *Socket, net_address *From, u8 *Buffer, u32 BufferSize)
{
    if (!Socket->Open) return 0;
    for (;;)
    {
        sockaddr_in Address = {};
        int Length = sizeof(Address);
        int Received = recvfrom((SOCKET)Socket->Handle, (char *)Buffer, (int)BufferSize, 0, (sockaddr *)&Address, &Length);
        if (Received > 0)
        {
            From->Ip = ntohl(Address.sin_addr.s_addr);
            From->Port = ntohs(Address.sin_port);
            return (u32)Received;
        }

        if (Received == 0) continue; // empty datagram

        // Windows reports an oversized datagram (WSAEMSGSIZE) and an ICMP
        // "port unreachable" from an earlier send (WSAECONNRESET) as errors
        // on the next receive. Neither means the socket is empty, so skip them.
        int Error = WSAGetLastError();
        if (Received < 0 && (Error == WSAEMSGSIZE || Error == WSAECONNRESET)) continue;
        return 0;
    }
}
