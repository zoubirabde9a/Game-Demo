/* Chat on the client's connection, declared in client_chat.h. Included by
   client.cpp after NetClientSend, which it uses. */

// NOTE(zoubir): the probe and the bot tool build this without the engine
// and its string helpers
internal void
NetCopyChatText(char *Out, const char *Text)
{
    u32 Length = 0;
    for (; Text[Length] && Length + 1 < NET_CHAT_SIZE; ++Length) Out[Length] = Text[Length];
    Out[Length] = 0;
}

internal bool32
NetClientSay(net_client *Client, const char *Text)
{
    net_client_chat *Chat = &Client->Chat;
    if (Client->State != NetClient_Connected || !Text || !Text[0] ||
        Chat->OutCount >= NET_CHAT_OUTBOX)
    {
        return false;
    }
    // NOTE(zoubir): 0 means "nothing said" on the wire, so the count skips it
    if (++Chat->LastSayId == 0) Chat->LastSayId = 1;
    NetCopyChatText(Chat->Out[Chat->OutCount], Text);
    Chat->OutIds[Chat->OutCount++] = Chat->LastSayId;
    if (Chat->OutCount == 1) Chat->ResendIn = 0.f;
    return true;
}

internal bool32
NetClientTakeChat(net_client *Client, net_chat_line *Out)
{
    net_client_chat *Chat = &Client->Chat;
    if (Chat->InCount == 0) return false;
    *Out = Chat->In[0];
    for (u32 Index = 1; Index < Chat->InCount; ++Index) Chat->In[Index - 1] = Chat->In[Index];
    --Chat->InCount;
    return true;
}

// A ChatLines packet: keeps the lines newer than any held (the server
// sends them in order, so an older one is a resend), and drops the
// outgoing lines the server says it took.
internal void
NetClientChatHandle(net_client *Client, net_chat_lines *Lines)
{
    net_client_chat *Chat = &Client->Chat;
    for (u32 Index = 0; Index < Lines->Count; ++Index)
    {
        net_chat_line *Line = &Lines->Lines[Index];
        if (Line->Number <= Chat->Heard) continue;
        Chat->Heard = Line->Number;
        if (Chat->InCount == NET_CHAT_INBOX)
        {
            net_chat_line Dropped;
            NetClientTakeChat(Client, &Dropped);
        }
        Chat->In[Chat->InCount++] = *Line;
    }
    if (Lines->Count) Chat->Confirm = true;

    u32 Taken = 0;
    while (Taken < Chat->OutCount && !NetSequenceNewer(Chat->OutIds[Taken], Lines->SaidId)) ++Taken;
    if (Taken)
    {
        for (u32 Index = Taken; Index < Chat->OutCount; ++Index)
        {
            Chat->OutIds[Index - Taken] = Chat->OutIds[Index];
            NetCopyChatText(Chat->Out[Index - Taken], Chat->Out[Index]);
        }
        Chat->OutCount -= Taken;
        Chat->ResendIn = 0.f;
    }
}

// Once per poll while connected: sends the line waiting (when its resend
// is due) and confirms the lines that came.
internal void
NetClientChatPoll(net_client *Client, float Dt)
{
    net_client_chat *Chat = &Client->Chat;
    Chat->ResendIn -= Dt;
    bool32 Saying = Chat->OutCount > 0 && Chat->ResendIn <= 0.f;
    if (!Saying && !Chat->Confirm) return;
    net_packet Out = {};
    Out.ChatSay.Heard = Chat->Heard;
    if (Chat->OutCount)
    {
        Out.ChatSay.SayId = Chat->OutIds[0];
        NetCopyChatText(Out.ChatSay.Text, Chat->Out[0]);
        if (Saying) Chat->ResendIn = NET_CHAT_RESEND;
    }
    NetClientSend(Client, &Out, NetPacket_Chat);
    Chat->Confirm = false;
}
