#if !defined(NET_SOCKET_H)
#define NET_SOCKET_H
/* Non-blocking UDP sockets on Windows and Linux. The server and the client
   each open one socket, send packets written by NetWritePacket, and poll
   NetReceiveFrom every tick until it reports nothing waiting.

   The Windows and Linux code lives in platform/. Including socket.cpp
   picks the right one. On Windows it links ws2_32.lib by itself. */

#include "address.h"

struct net_socket
{
    u64 Handle;
    bool32 Open;
};

// Call once at startup and shutdown. Does nothing on Linux.
internal bool32 NetSocketsStartup();
internal void NetSocketsShutdown();

// Port 0 lets the OS pick one, which is what a client wants.
internal net_socket NetOpenSocket(u16 Port);
internal void NetCloseSocket(net_socket *Socket);

// The port the socket is bound to, useful after opening on port 0.
internal u16 NetSocketPort(net_socket *Socket);

internal bool32 NetSendTo(net_socket *Socket, net_address To, u8 *Data, u32 Size);

// Returns the size of the datagram read into Buffer, or 0 if none is waiting.
// Datagrams larger than BufferSize are dropped.
internal u32 NetReceiveFrom(net_socket *Socket, net_address *From, u8 *Buffer, u32 BufferSize);

#endif
