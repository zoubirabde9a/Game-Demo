/* TCP for the web gateway: a listening socket, accepting, non-blocking
   reads and writes, and waiting on many sockets at once with poll
   (WSAPoll on Windows). The UDP side uses code/net/socket.h, which also
   starts Winsock. Handles are u64, like net_socket's. */

#if defined(_WIN32)
typedef WSAPOLLFD tcp_poll_entry;
#define TcpPoll(Entries, Count, TimeoutMs) WSAPoll(Entries, (ULONG)(Count), TimeoutMs)
#define TCP_INVALID ((u64)INVALID_SOCKET)
#define TcpHandle(Handle) ((SOCKET)(Handle))
#else
#include <poll.h>
#include <errno.h>
#include <netinet/tcp.h>
typedef struct pollfd tcp_poll_entry;
#define TcpPoll(Entries, Count, TimeoutMs) poll(Entries, (nfds_t)(Count), TimeoutMs)
#define TCP_INVALID ((u64)-1)
#define TcpHandle(Handle) ((int)(Handle))
#endif

internal void
TcpClose(u64 Handle)
{
    if (Handle == TCP_INVALID) return;
#if defined(_WIN32)
    closesocket((SOCKET)Handle);
#else
    close((int)Handle);
#endif
}

internal bool32
TcpMakeNonBlocking(u64 Handle)
{
#if defined(_WIN32)
    u_long NonBlocking = 1;
    return ioctlsocket((SOCKET)Handle, FIONBIO, &NonBlocking) == 0;
#else
    int Flags = fcntl((int)Handle, F_GETFL, 0);
    return Flags >= 0 && fcntl((int)Handle, F_SETFL, Flags | O_NONBLOCK) == 0;
#endif
}

// NOTE(zoubir): Loopback true listens on 127.0.0.1 only
internal u64
TcpListen(u16 Port, bool32 Loopback)
{
#if defined(_WIN32)
    u64 Handle = (u64)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    u64 Handle = (u64)(i64)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
    if (Handle == TCP_INVALID) return TCP_INVALID;
    int Yes = 1;
    setsockopt(TcpHandle(Handle), SOL_SOCKET, SO_REUSEADDR, (const char *)&Yes, sizeof(Yes));
    sockaddr_in Address = {};
    Address.sin_family = AF_INET;
    Address.sin_addr.s_addr = htonl(Loopback ? INADDR_LOOPBACK : INADDR_ANY);
    Address.sin_port = htons(Port);
    if (bind(TcpHandle(Handle), (sockaddr *)&Address, sizeof(Address)) != 0 ||
        listen(TcpHandle(Handle), 64) != 0 || !TcpMakeNonBlocking(Handle))
    {
        TcpClose(Handle);
        return TCP_INVALID;
    }
    return Handle;
}

internal u64
TcpAccept(u64 Listener)
{
#if defined(_WIN32)
    SOCKET Accepted = accept((SOCKET)Listener, 0, 0);
    if (Accepted == INVALID_SOCKET) return TCP_INVALID;
    u64 Handle = (u64)Accepted;
#else
    int Accepted = accept((int)Listener, 0, 0);
    if (Accepted < 0) return TCP_INVALID;
    u64 Handle = (u64)Accepted;
#endif
    // NOTE(zoubir): every packet goes out the moment it is written
    int Yes = 1;
    setsockopt(TcpHandle(Handle), IPPROTO_TCP, TCP_NODELAY, (const char *)&Yes, sizeof(Yes));
    if (!TcpMakeNonBlocking(Handle))
    {
        TcpClose(Handle);
        return TCP_INVALID;
    }
    return Handle;
}

// NOTE(zoubir): >0 bytes read, 0 nothing waiting, -1 closed or failed
internal int
TcpRead(u64 Handle, u8 *Buffer, u32 Size)
{
#if defined(_WIN32)
    int Got = recv((SOCKET)Handle, (char *)Buffer, (int)Size, 0);
    if (Got > 0) return Got;
    if (Got < 0 && WSAGetLastError() == WSAEWOULDBLOCK) return 0;
    return -1;
#else
    ssize_t Got = recv((int)Handle, Buffer, Size, 0);
    if (Got > 0) return (int)Got;
    if (Got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) return 0;
    return -1;
#endif
}

// NOTE(zoubir): >=0 bytes written (0 when the socket is full), -1 failed
internal int
TcpWrite(u64 Handle, const u8 *Data, u32 Size)
{
#if defined(_WIN32)
    int Sent = send((SOCKET)Handle, (const char *)Data, (int)Size, 0);
    if (Sent >= 0) return Sent;
    return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
#else
    ssize_t Sent = send((int)Handle, Data, Size, MSG_NOSIGNAL);
    if (Sent >= 0) return (int)Sent;
    return (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) ? 0 : -1;
#endif
}

// NOTE(zoubir): a TCP connection to 127.0.0.1:Port, for the tests; it
// blocks until connected
internal u64
TcpConnectLoopback(u16 Port)
{
#if defined(_WIN32)
    u64 Handle = (u64)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
    u64 Handle = (u64)(i64)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
    if (Handle == TCP_INVALID) return TCP_INVALID;
    sockaddr_in Address = {};
    Address.sin_family = AF_INET;
    Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    Address.sin_port = htons(Port);
    if (connect(TcpHandle(Handle), (sockaddr *)&Address, sizeof(Address)) != 0 || !TcpMakeNonBlocking(Handle))
    {
        TcpClose(Handle);
        return TCP_INVALID;
    }
    return Handle;
}
