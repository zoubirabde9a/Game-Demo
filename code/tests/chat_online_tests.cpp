/* Chat online: lines said reach every player once each, in the same order
   for all, over a clean link and over one that drops, doubles and
   reorders packets; the relay drops repeats, empty lines and floods, and
   a player joining late hears only what is said after. Included by
   server_tests.cpp, which calls RunChatOnlineTests. */

#define CHAT_TEST_MAX_HEARD 16

struct chat_test_ear
{
    u32 Count;
    net_chat_line Lines[CHAT_TEST_MAX_HEARD];
};

internal void
ListenForChat(net_client *Client, chat_test_ear *Ear)
{
    net_chat_line Line;
    while (NetClientTakeChat(Client, &Line))
    {
        if (Ear->Count < CHAT_TEST_MAX_HEARD) Ear->Lines[Ear->Count++] = Line;
    }
}

internal void
TestChatReachesEveryone()
{
    static server Server;
    static net_client Ana, Ben;
    static lossy_link Link;
    static chat_test_ear AnaHeard, BenHeard;
    AnaHeard = {};
    BenHeard = {};
    Check(ServerStart(&Server, 0));
    ClearMonsters(&Server);
    // NOTE: Ben's link loses a third of the packets each way, doubles some
    // and holds some back up to 100 ms, so they also come out of order
    Check(LossyOpen(&Link, LocalServer(&Server), 33, 10, 6, 11));
    Check(NetClientConnect(&Ana, LocalServer(&Server), 501, SimContentId(), "Ana"));
    Check(NetClientConnect(&Ben, LossyAddress(&Link), 502, SimContentId()));

    #define CHAT_FRAME() do { NetClientUpdate(&Ana, 1.0f / SERVER_TICK_RATE, 0, 0, 0); \
                              NetClientUpdate(&Ben, 1.0f / SERVER_TICK_RATE, 0, 0, 0); \
                              LossyPump(&Link); ServerTick(&Server); LossyPump(&Link); \
                              ListenForChat(&Ana, &AnaHeard); ListenForChat(&Ben, &BenHeard); } while (0)
    for (int Frame = 0; Frame < 5 * SERVER_TICK_RATE &&
         (Ana.State != NetClient_Connected || Ben.State != NetClient_Connected); ++Frame) CHAT_FRAME();
    Check(Ana.State == NetClient_Connected && Ben.State == NetClient_Connected);

    Check(NetClientSay(&Ana, "hello"));
    Check(NetClientSay(&Ana, "   spaced out   "));
    Check(NetClientSay(&Ana, "third"));
    Check(NetClientSay(&Ben, "hi from the bad link"));
    for (int Frame = 0; Frame < 10 * SERVER_TICK_RATE && (AnaHeard.Count < 4 || BenHeard.Count < 4); ++Frame)
    {
        CHAT_FRAME();
    }
    // a moment more, so a doubled or resent packet would show as a repeat
    for (int Frame = 0; Frame < SERVER_TICK_RATE; ++Frame) CHAT_FRAME();
    #undef CHAT_FRAME

    printf("  chat over a bad link: %u dropped, %u duplicated\n", Link.Dropped, Link.Duplicated);
    Check(AnaHeard.Count == 4 && BenHeard.Count == 4);
    u32 FromAna = 0;
    for (u32 Index = 0; Index < AnaHeard.Count && Index < BenHeard.Count; ++Index)
    {
        net_chat_line *A = &AnaHeard.Lines[Index], *B = &BenHeard.Lines[Index];
        Check(A->Number == B->Number && strcmp(A->Text, B->Text) == 0 && A->Slot == B->Slot);
        if (A->Slot == Ana.PlayerIndex)
        {
            Check(strcmp(A->Name, "Ana") == 0);
            char *Expected[] = {(char *)"hello", (char *)"spaced out", (char *)"third"};
            if (FromAna < 3) Check(strcmp(A->Text, Expected[FromAna]) == 0);
            ++FromAna;
        }
        else
        {
            Check(A->Slot == Ben.PlayerIndex);
            char Plain[NET_NAME_SIZE];
            snprintf(Plain, sizeof(Plain), "Player %u", Ben.PlayerIndex + 1);
            Check(strcmp(A->Name, Plain) == 0);
            Check(strcmp(A->Text, "hi from the bad link") == 0);
        }
    }
    Check(FromAna == 3);
    Check(Ana.Chat.OutCount == 0 && Ben.Chat.OutCount == 0);

    NetClientDisconnect(&Ana);
    NetClientDisconnect(&Ben);
    NetCloseSocket(&Link.Socket);
    ServerStop(&Server);
}

internal void
TestChatRelayDropsRepeatsAndFloods()
{
    static server_chat Chat;
    Chat = {};
    ChatJoined(&Chat, 0);
    net_chat_say Say = {};
    Say.SayId = 1;
    snprintf(Say.Text, sizeof(Say.Text), "%s", "first");
    Check(ChatReceive(&Chat, 0, &Say, (char *)"Ana") != 0);
    // the same line resent (its confirmation was lost) is not said twice
    Check(ChatReceive(&Chat, 0, &Say, (char *)"Ana") == 0);
    Check(Chat.Newest == 1);
    // only spaces say nothing, but it is still confirmed
    Say.SayId = 2;
    snprintf(Say.Text, sizeof(Say.Text), "%s", "    ");
    Check(ChatReceive(&Chat, 0, &Say, (char *)"Ana") == 0);
    net_chat_lines Out;
    Check(ChatPacketFor(&Chat, 0, &Out));
    Check(Out.SaidId == 2 && Out.Count == 1);

    // a burst of lines: the allowance lets SERVER_CHAT_BURST through, and
    // after that one each 1 / SERVER_CHAT_PER_SECOND seconds
    u32 Said = 0;
    for (u16 Id = 3; Id < 13; ++Id)
    {
        Say.SayId = Id;
        snprintf(Say.Text, sizeof(Say.Text), "line %u", Id);
        if (ChatReceive(&Chat, 0, &Say, (char *)"Ana")) ++Said;
    }
    Check(Said == (u32)SERVER_CHAT_BURST - 1);
    for (int Tick = 0; Tick < (int)(SERVER_TICK_RATE / SERVER_CHAT_PER_SECOND) + 1; ++Tick)
    {
        ChatAdvance(&Chat, 1.0f / SERVER_TICK_RATE);
    }
    Say.SayId = 13;
    Check(ChatReceive(&Chat, 0, &Say, (char *)"Ana") != 0);

    // a player joining now hears none of that, only what comes after
    ChatJoined(&Chat, 1);
    Check(!ChatPacketFor(&Chat, 1, &Out));
    net_chat_say Hi = {};
    Hi.SayId = 1;
    snprintf(Hi.Text, sizeof(Hi.Text), "%s", "hi");
    net_chat_line *Line = ChatReceive(&Chat, 1, &Hi, (char *)"Ben");
    Check(Line != 0);
    Check(ChatPacketFor(&Chat, 1, &Out));
    Check(Out.Count == 1 && strcmp(Out.Lines[0].Text, "hi") == 0 && Out.Lines[0].Slot == 1);
    // until it confirms, the line goes again after SERVER_CHAT_RESEND
    Check(!ChatPacketFor(&Chat, 1, &Out));
    for (int Tick = 0; Tick < (int)(SERVER_CHAT_RESEND * SERVER_TICK_RATE) + 1; ++Tick)
    {
        ChatAdvance(&Chat, 1.0f / SERVER_TICK_RATE);
    }
    Check(ChatPacketFor(&Chat, 1, &Out) && Out.Count == 1);
    net_chat_say Heard = {};
    Heard.Heard = Line->Number;
    ChatReceive(&Chat, 1, &Heard, (char *)"Ben");
    for (int Tick = 0; Tick < SERVER_TICK_RATE; ++Tick) ChatAdvance(&Chat, 1.0f / SERVER_TICK_RATE);
    Check(!ChatPacketFor(&Chat, 1, &Out));
}

internal void
RunChatOnlineTests()
{
    TestChatRelayDropsRepeatsAndFloods();
    TestChatReachesEveryone();
}
