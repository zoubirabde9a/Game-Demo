/* Browser version of socket.h. A web page cannot send UDP, so every
   server the game talks to gets a WebSocket to the web gateway
   (server/web_gateway/), which passes each message on to that server as
   one UDP datagram and sends the replies back the same way. One message
   is one packet, so the rest of code/net sees datagrams, as on the
   desktop. Packets sent before the WebSocket opens wait in a short queue
   (a few are enough: the client resends what matters); a WebSocket that
   closed is opened again by a later send, at most once a second.

   The gateway is at the page's own host, at /net/<server>:<port>, over
   wss on an https page. Adding ?gateway=ws://host:port to the page's
   address points it elsewhere. A page cannot look a host name up, so
   NetLookupHost gives each name a stand-in address in 240.0.0.0/4, a
   block no real server uses, and the gateway looks the name up itself. */

#include <emscripten/emscripten.h>

#define WEB_NET_NAMED_HOST 0xf0000000u

// NOTE(zoubir): the JavaScript side keeps the sockets in Module.gameNet
EM_JS(int, WebNetOpen, (), {
    var Net = Module.gameNet || (Module.gameNet = {Next: 1, Sockets: {}, Hosts: []});
    var Id = Net.Next++;
    Net.Sockets[Id] = {Links: {}, Inbox: []};
    return Id;
});

EM_JS(void, WebNetClose, (int Id), {
    var Net = Module.gameNet;
    var Socket = Net && Net.Sockets[Id];
    if (!Socket) return;
    for (var Key in Socket.Links)
    {
        try { Socket.Links[Key].close(); } catch (Ignored) {}
    }
    delete Net.Sockets[Id];
});

EM_JS(int, WebNetHostIndex, (const char *Host), {
    Host = Number(Host);
    var Net = Module.gameNet || (Module.gameNet = {Next: 1, Sockets: {}, Hosts: []});
    var Name = UTF8ToString(Host).toLowerCase();
    var Index = Net.Hosts.indexOf(Name);
    if (Index < 0)
    {
        Index = Net.Hosts.length;
        Net.Hosts.push(Name);
    }
    return Index;
});

EM_JS(int, WebNetSend, (int Id, unsigned Ip, int Port, const unsigned char *Data, int Size), {
    Data = Number(Data);
    var Net = Module.gameNet;
    var Socket = Net && Net.Sockets[Id];
    if (!Socket) return 0;
    Ip = Ip >>> 0;
    var Key = Ip + ":" + Port;
    var Link = Socket.Links[Key];
    var Now = Date.now();
    if (!Link || (Link.readyState > 1 && Now - Link.OpenedAt > 1000))
    {
        var Server = ((Ip >>> 28) == 15) ? Net.Hosts[Ip & 0xffffff] :
            [Ip >>> 24, (Ip >>> 16) & 255, (Ip >>> 8) & 255, Ip & 255].join(".");
        var Gateway = new URLSearchParams(location.search).get("gateway");
        if (!Gateway)
        {
            Gateway = location.host ?
                (location.protocol == "https:" ? "wss://" : "ws://") + location.host :
                "ws://127.0.0.1:27080";
        }
        try
        {
            Link = new WebSocket(Gateway + "/net/" + Server + ":" + Port);
        }
        catch (Failed)
        {
            return 0;
        }
        Link.binaryType = "arraybuffer";
        Link.OpenedAt = Now;
        Link.Pending = [];
        Link.onopen = function()
        {
            for (var Index = 0; Index < Link.Pending.length; Index++) Link.send(Link.Pending[Index]);
            Link.Pending = [];
        };
        Link.onmessage = function(Event)
        {
            if (typeof Event.data == "string" || Socket.Inbox.length >= 256) return;
            Socket.Inbox.push({Ip: Ip, Port: Port, Data: new Uint8Array(Event.data)});
        };
        Socket.Links[Key] = Link;
    }
    var Bytes = HEAPU8.slice(Data, Data + Size);
    if (Link.readyState == 1) Link.send(Bytes);
    else if (Link.readyState == 0 && Link.Pending.length < 8) Link.Pending.push(Bytes);
    return 1;
});

EM_JS(int, WebNetReceive, (int Id, unsigned *Ip, unsigned short *Port, unsigned char *Buffer, int BufferSize), {
    Ip = Number(Ip); Port = Number(Port); Buffer = Number(Buffer);
    var Net = Module.gameNet;
    var Socket = Net && Net.Sockets[Id];
    if (!Socket) return 0;
    while (Socket.Inbox.length)
    {
        var Message = Socket.Inbox.shift();
        if (Message.Data.length == 0 || Message.Data.length > BufferSize) continue;
        HEAPU8.set(Message.Data, Buffer);
        HEAPU32[Ip >> 2] = Message.Ip;
        HEAPU16[Port >> 1] = Message.Port;
        return Message.Data.length;
    }
    return 0;
});

internal bool32 NetSocketsStartup() { return true; }
internal void NetSocketsShutdown() {}

internal net_socket
NetOpenSocket(u16 Port)
{
    net_socket Result = {};
    Result.Handle = (u64)WebNetOpen();
    Result.Open = true;
    return Result;
}

internal void
NetCloseSocket(net_socket *Socket)
{
    if (Socket->Open) WebNetClose((int)Socket->Handle);
    *Socket = {};
}

// NOTE(zoubir): a page has no port of its own; any nonzero answer will do
internal u16
NetSocketPort(net_socket *Socket)
{
    return Socket->Open ? 1 : 0;
}

internal bool32
NetLookupHost(const char *Host, u32 *Ip)
{
    *Ip = WEB_NET_NAMED_HOST | (u32)WebNetHostIndex(Host);
    return true;
}

internal bool32
NetSendTo(net_socket *Socket, net_address To, u8 *Data, u32 Size)
{
    if (!Socket->Open) return false;
    return WebNetSend((int)Socket->Handle, To.Ip, To.Port, Data, (int)Size) != 0;
}

internal u32
NetReceiveFrom(net_socket *Socket, net_address *From, u8 *Buffer, u32 BufferSize)
{
    if (!Socket->Open) return 0;
    u32 Ip = 0;
    u16 Port = 0;
    int Size = WebNetReceive((int)Socket->Handle, &Ip, &Port, Buffer, (int)BufferSize);
    if (Size <= 0) return 0;
    From->Ip = Ip;
    From->Port = Port;
    return (u32)Size;
}
