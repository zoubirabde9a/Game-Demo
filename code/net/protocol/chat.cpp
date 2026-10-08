/* Chat packets as bytes (protocol.cpp, layout in protocol/chat.h). Text and
   names go as a length byte and that many bytes (NetName), and reading
   keeps printable ASCII only, so whatever comes off the net is safe to
   draw and to log. */

internal bool32
NetSerializeChatSay(net_stream *S, net_chat_say *Say)
{
    NetU32(S, &Say->Heard);
    NetU16(S, &Say->SayId);
    NetName(S, Say->Text, NET_CHAT_SIZE);
    return true;
}

internal bool32
NetSerializeChatLines(net_stream *S, net_chat_lines *Lines)
{
    NetU16(S, &Lines->SaidId);
    NetU8(S, &Lines->Count);
    if (Lines->Count > NET_CHAT_MAX_LINES) return false;
    for (u32 Index = 0; Index < Lines->Count; ++Index)
    {
        net_chat_line *Line = &Lines->Lines[Index];
        NetU32(S, &Line->Number);
        NetU8(S, &Line->Slot);
        NetName(S, Line->Name, NET_NAME_SIZE);
        NetName(S, Line->Text, NET_CHAT_SIZE);
    }
    return true;
}
