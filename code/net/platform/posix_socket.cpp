/* Linux (and other POSIX) version of socket.h. */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

internal bool32 NetSocketsStartup() { return true; }
internal void NetSocketsShutdown() {}

internal net_socket
NetOpenSocket(u16 Port)
{
    net_socket Result = {};
    int Handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (Handle < 0) return Result;

    sockaddr_in Bind = {};
    Bind.sin_family = AF_INET;
    Bind.sin_addr.s_addr = htonl(INADDR_ANY);
    Bind.sin_port = htons(Port);
    if (bind(Handle, (sockaddr *)&Bind, sizeof(Bind)) != 0 ||
        fcntl(Handle, F_SETFL, fcntl(Handle, F_GETFL, 0) | O_NONBLOCK) != 0)
    {
        close(Handle);
        return Result;
    }

    Result.Handle = (u64)Handle;
    Result.Open = true;
    return Result;
}

internal void
NetCloseSocket(net_socket *Socket)
{
    if (Socket->Open) close((int)Socket->Handle);
    *Socket = {};
}

internal u16
NetSocketPort(net_socket *Socket)
{
    sockaddr_in Address = {};
    socklen_t Length = sizeof(Address);
    if (!Socket->Open || getsockname((int)Socket->Handle, (sockaddr *)&Address, &Length) != 0) return 0;
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
    ssize_t Sent = sendto((int)Socket->Handle, Data, Size, 0, (sockaddr *)&Address, sizeof(Address));
    return Sent == (ssize_t)Size;
}

internal u32
NetReceiveFrom(net_socket *Socket, net_address *From, u8 *Buffer, u32 BufferSize)
{
    if (!Socket->Open) return 0;
    for (;;)
    {
        sockaddr_in Address = {};
        socklen_t Length = sizeof(Address);
        // MSG_TRUNC makes recvfrom return the real size, so oversized datagrams can be dropped.
        ssize_t Received = recvfrom((int)Socket->Handle, Buffer, BufferSize, MSG_TRUNC, (sockaddr *)&Address, &Length);
        if (Received < 0) return 0;
        if (Received == 0 || (size_t)Received > BufferSize) continue; // empty or oversized
        From->Ip = ntohl(Address.sin_addr.s_addr);
        From->Port = ntohs(Address.sin_port);
        return (u32)Received;
    }
}
