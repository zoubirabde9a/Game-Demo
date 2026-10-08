/* Chat on the client's connection (client.h; packets in protocol/chat.h).
   NetClientSay queues a line for the server, which goes out again every
   NET_CHAT_RESEND seconds until the server confirms it; lines queue up to
   NET_CHAT_OUTBOX and go one at a time, in order. Lines the server relays
   wait in the inbox until NetClientTakeChat hands them over, oldest
   first; each is confirmed back to the server as it arrives. A
   reconnect starts both empty. */

#define NET_CHAT_OUTBOX 4
#define NET_CHAT_INBOX 16
#define NET_CHAT_RESEND 0.2f

struct net_client_chat
{
    u32 Heard;            // the newest line number received
    bool32 Confirm;       // lines came since the last Chat packet: say which we hold
    u16 LastSayId;
    u32 OutCount;         // Out[0] is the line being sent
    u16 OutIds[NET_CHAT_OUTBOX];
    char Out[NET_CHAT_OUTBOX][NET_CHAT_SIZE];
    float ResendIn;
    u32 InCount;          // oldest first
    net_chat_line In[NET_CHAT_INBOX];
};

struct net_client;

// Queues Text to be said. False when not connected, the text is empty, or
// NET_CHAT_OUTBOX lines are still waiting.
internal bool32 NetClientSay(net_client *Client, const char *Text);

// Moves the oldest line received into *Out; false when none is waiting.
internal bool32 NetClientTakeChat(net_client *Client, net_chat_line *Out);
