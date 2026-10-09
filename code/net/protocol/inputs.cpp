/* Inputs on the wire (protocol.cpp, net_input_batch in protocol.h). An
   input packet carries every input the server has not confirmed, up to
   NET_MAX_INPUTS_PER_PACKET (about half a second), so a run of lost
   packets on a bad link is covered by the next one that arrives
   (net/client.cpp). The newest input goes whole; each older one is a byte
   saying which fields differ from the input after it, then only those,
   so a full batch of held keys costs about 70 bytes. */

internal void
NetSerializeInput(net_stream *S, net_input *Input)
{
    NetU32(S, &Input->Tick);
    NetU32(S, &Input->Buttons);
    NetUnitFloat(S, &Input->AimX);
    NetUnitFloat(S, &Input->AimY);
    NetU16(S, &Input->Target);
    NetU8(S, &Input->Role);
}

// NOTE(zoubir): an input older than Newer, sent as what differs from it;
// its tick is always the one before Newer's
#define NET_INPUT_NEW_BUTTONS 1
#define NET_INPUT_NEW_AIM 2
#define NET_INPUT_NEW_TARGET 4
#define NET_INPUT_NEW_ROLE 8

// NOTE(zoubir): an aim as NetUnitFloat sends it, so two aims that arrive
// the same compare the same
inline i32
NetUnitSteps(float Value)
{
    float V = Value;
    if (!(V >= -1.0f)) V = -1.0f;
    if (V > 1.0f) V = 1.0f;
    i32 Result = (i32)(V * 32767.0f + (V < 0 ? -0.5f : 0.5f));
    return Result;
}

internal bool32
NetSerializeOlderInput(net_stream *S, net_input *Input, net_input *Newer)
{
    u8 Changed = 0;
    if (S->Writing)
    {
        Changed = (u8)((Input->Buttons != Newer->Buttons ? NET_INPUT_NEW_BUTTONS : 0) |
                       (NetUnitSteps(Input->AimX) != NetUnitSteps(Newer->AimX) ||
                        NetUnitSteps(Input->AimY) != NetUnitSteps(Newer->AimY) ?
                        NET_INPUT_NEW_AIM : 0) |
                       (Input->Target != Newer->Target ? NET_INPUT_NEW_TARGET : 0) |
                       (Input->Role != Newer->Role ? NET_INPUT_NEW_ROLE : 0));
    }
    NetU8(S, &Changed);
    if (Changed > 15) return false;
    if (!S->Writing)
    {
        *Input = *Newer;
    }
    Input->Tick = Newer->Tick - 1;
    if (Changed & NET_INPUT_NEW_BUTTONS) NetU32(S, &Input->Buttons);
    if (Changed & NET_INPUT_NEW_AIM)
    {
        NetUnitFloat(S, &Input->AimX);
        NetUnitFloat(S, &Input->AimY);
    }
    if (Changed & NET_INPUT_NEW_TARGET) NetU16(S, &Input->Target);
    if (Changed & NET_INPUT_NEW_ROLE) NetU8(S, &Input->Role);
    return true;
}
