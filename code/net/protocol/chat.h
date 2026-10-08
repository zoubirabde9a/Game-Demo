/* Chat on the wire (protocol.h): a player's line to the server, and the
   lines the server relays to every player. Snapshots are never resent, so
   chat travels in its own packets, and both directions are resent until
   the other end says it has them.

   Client to server, NetPacket_Chat: Heard is the newest line number the
   client holds, so the server stops resending up to it; SayId counts the
   client's own lines (0: nothing to say, only Heard), and the client sends
   the same SayId and Text again until a ChatLines echoes it in SaidId.

   Server to client, NetPacket_ChatLines: the lines after the client's
   Heard, oldest first, each numbered (one count for the whole server,
   from 1) and carrying the sender's name, so a client that has not seen
   a name in a snapshot yet still shows who spoke.

   The two packets came after the GDMn layout without changing any packet
   before them, so a build without chat drops them as malformed and plays
   on; the protocol id stayed. */

#define NET_CHAT_SIZE 96        // one line, 95 characters plus the terminator
#define NET_CHAT_MAX_LINES 8    // lines in one ChatLines packet (8 full ones fit in 1000 bytes)

struct net_chat_say
{
    u32 Heard;
    u16 SayId;
    char Text[NET_CHAT_SIZE];
};

struct net_chat_line
{
    u32 Number;
    u8 Slot;                  // the sender's player slot
    char Name[NET_NAME_SIZE];
    char Text[NET_CHAT_SIZE];
};

struct net_chat_lines
{
    u16 SaidId;               // the newest SayId the server took from this client
    u8 Count;
    net_chat_line Lines[NET_CHAT_MAX_LINES];
};
