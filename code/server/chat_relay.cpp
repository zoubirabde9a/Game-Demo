/* Chat relay: a line one player says reaches every connected player
   (packets in net/protocol/chat.h). The server numbers each line, keeps
   the last SERVER_CHAT_HISTORY, and sends each client the lines after the
   newest one it has confirmed, again every SERVER_CHAT_RESEND seconds
   until it confirms them, so a lost packet only delays a line. A client
   that falls further behind than the history skips to the oldest line
   kept. A player joining hears only what is said after they join.

   Each player may say SERVER_CHAT_BURST lines at once and then one every
   1 / SERVER_CHAT_PER_SECOND seconds; lines past that are dropped (still
   confirmed, so the client stops resending them). Chat never touches the
   game, so replays and the world hash know nothing of it. */

#define SERVER_CHAT_HISTORY 32
#define SERVER_CHAT_RESEND 0.2f
#define SERVER_CHAT_BURST 4.f
#define SERVER_CHAT_PER_SECOND 0.5f

struct server_chat
{
    u32 Newest; // the newest line's number, 0 before the first
    net_chat_line History[SERVER_CHAT_HISTORY]; // line N at N % SERVER_CHAT_HISTORY
    u32 Heard[NET_MAX_CLIENTS];      // the newest line each client confirmed
    u16 SaidId[NET_MAX_CLIENTS];     // the newest SayId taken from each client
    bool32 AckOwed[NET_MAX_CLIENTS]; // a Chat packet came that wants a SaidId back
    float ResendIn[NET_MAX_CLIENTS];
    float Allowance[NET_MAX_CLIENTS]; // lines each player may say right now
};

internal void
ChatJoined(server_chat *Chat, u32 Slot)
{
    Chat->Heard[Slot] = Chat->Newest;
    Chat->SaidId[Slot] = 0;
    Chat->AckOwed[Slot] = false;
    Chat->ResendIn[Slot] = 0.f;
    Chat->Allowance[Slot] = SERVER_CHAT_BURST;
}

// NOTE(zoubir): Text without the spaces at either end; empty when
// nothing else is left
internal void
ChatTrim(char *Out, u32 OutSize, char *Text)
{
    while (*Text == ' ') ++Text;
    u32 Length = 0;
    for (; Text[Length] && Length + 1 < OutSize; ++Length) Out[Length] = Text[Length];
    while (Length > 0 && Out[Length - 1] == ' ') --Length;
    Out[Length] = 0;
}

// Takes a Chat packet from the client in Slot, whose player is called
// Name. Returns the line it added for everyone, or 0 (only a confirmation,
// a line already taken, an empty line, or one over the player's allowance).
internal net_chat_line *
ChatReceive(server_chat *Chat, u32 Slot, net_chat_say *Say, char *Name)
{
    if (Say->Heard > Chat->Heard[Slot] && Say->Heard <= Chat->Newest)
    {
        Chat->Heard[Slot] = Say->Heard;
    }
    if (Say->SayId == 0) return 0;
    Chat->AckOwed[Slot] = true;
    if (Chat->SaidId[Slot] != 0 && !NetSequenceNewer(Say->SayId, Chat->SaidId[Slot])) return 0;
    Chat->SaidId[Slot] = Say->SayId;

    char Text[NET_CHAT_SIZE];
    ChatTrim(Text, sizeof(Text), Say->Text);
    if (!Text[0] || Chat->Allowance[Slot] < 1.f) return 0;
    Chat->Allowance[Slot] -= 1.f;

    ++Chat->Newest;
    net_chat_line *Line = &Chat->History[Chat->Newest % SERVER_CHAT_HISTORY];
    *Line = {};
    Line->Number = Chat->Newest;
    Line->Slot = (u8)Slot;
    snprintf(Line->Name, sizeof(Line->Name), "%s", Name);
    snprintf(Line->Text, sizeof(Line->Text), "%s", Text);
    // NOTE(zoubir): everyone gets it this tick, not at their next resend
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index) Chat->ResendIn[Index] = 0.f;
    return Line;
}

// Once per tick: refills the allowances and runs the resend clocks.
internal void
ChatAdvance(server_chat *Chat, float Dt)
{
    for (u32 Slot = 0; Slot < NET_MAX_CLIENTS; ++Slot)
    {
        Chat->Allowance[Slot] += Dt * SERVER_CHAT_PER_SECOND;
        if (Chat->Allowance[Slot] > SERVER_CHAT_BURST) Chat->Allowance[Slot] = SERVER_CHAT_BURST;
        Chat->ResendIn[Slot] -= Dt;
    }
}

// Fills Out with what the client in Slot should get now and returns true,
// or returns false when nothing is due: it has every line, or the lines
// it lacks went out less than SERVER_CHAT_RESEND seconds ago.
internal bool32
ChatPacketFor(server_chat *Chat, u32 Slot, net_chat_lines *Out)
{
    u32 Oldest = Chat->Newest >= SERVER_CHAT_HISTORY ? Chat->Newest - SERVER_CHAT_HISTORY + 1 : 1;
    u32 First = Chat->Heard[Slot] + 1;
    if (First < Oldest) First = Oldest;
    bool32 Missing = First <= Chat->Newest;
    if (!Chat->AckOwed[Slot] && !(Missing && Chat->ResendIn[Slot] <= 0.f)) return false;

    *Out = {};
    Out->SaidId = Chat->SaidId[Slot];
    for (u32 Number = First; Number <= Chat->Newest && Out->Count < NET_CHAT_MAX_LINES; ++Number)
    {
        Out->Lines[Out->Count++] = Chat->History[Number % SERVER_CHAT_HISTORY];
    }
    Chat->AckOwed[Slot] = false;
    if (Missing) Chat->ResendIn[Slot] = SERVER_CHAT_RESEND;
    return true;
}
