/* Input queues: each client's inputs wait here between arriving and being
   applied, and the server applies one per tick. A client sends one input
   per tick of its own (NET_TICK_RATE), so a player moves exactly as many
   steps on the server as its client predicted, whatever the client's
   frame rate and however unevenly the packets arrive. Applying every
   input the moment it arrived made two packets landing in one tick count
   as one step and a late one as a repeat, and each of those came back to
   the player as a correction.

   A tick with no input waiting holds the player still (GameHoldPlayer)
   rather than repeating the last input: the client never steps without
   an input, so the server must not either.

   Inputs that come too fast pile up; past SERVER_INPUT_MAX_WAITING the
   oldest are applied together in one tick, so a burst after a stall does
   not leave the player that much further behind for good. Snapshots tell
   each client how many of its inputs are waiting (InputBuffered), and the
   client speeds up or slows down its ticks a little to keep one or two
   there (client/online_pacing.cpp). Included by server.cpp. */

#define SERVER_INPUT_QUEUE_SIZE 32
// NOTE(zoubir): more waiting than this and the extra ones are applied at
// once, down to SERVER_INPUT_KEEP_WAITING
#define SERVER_INPUT_MAX_WAITING 6
#define SERVER_INPUT_KEEP_WAITING 2

struct server_input_queue
{
    net_input Inputs[SERVER_INPUT_QUEUE_SIZE];
    u32 First;
    u32 Count;
};

internal void
ClearInputQueue(server_input_queue *Queue)
{
    Queue->First = 0;
    Queue->Count = 0;
}

// NOTE(zoubir): inputs arrive in tick order (connections.cpp keeps only
// ticks newer than any seen); a full queue drops its oldest
internal void
PushInput(server_input_queue *Queue, net_input *Input)
{
    if (Queue->Count == SERVER_INPUT_QUEUE_SIZE)
    {
        Queue->First = (Queue->First + 1) % SERVER_INPUT_QUEUE_SIZE;
        Queue->Count--;
    }
    Queue->Inputs[(Queue->First + Queue->Count) % SERVER_INPUT_QUEUE_SIZE] = *Input;
    Queue->Count++;
}

internal net_input
PopInput(server_input_queue *Queue)
{
    net_input Result = Queue->Inputs[Queue->First];
    Queue->First = (Queue->First + 1) % SERVER_INPUT_QUEUE_SIZE;
    Queue->Count--;
    return Result;
}

// NOTE(zoubir): once a tick, before the game steps. Connected has a bit
// set per slot a client holds; such a slot with nothing waiting is held
// for the tick (GameHoldPlayer), others (bots) are left alone
internal void
ApplyQueuedInputs(server_game *Game, server_input_queue *Queues, u32 SlotCount,
                  u32 Connected)
{
    for (u32 Slot = 0; Slot < SlotCount; ++Slot)
    {
        server_input_queue *Queue = &Queues[Slot];
        if (!Queue->Count && (Connected & (1u << Slot)))
        {
            GameHoldPlayer(Game, Slot);
            continue;
        }
        u32 Take = Queue->Count ? 1 : 0;
        if (Queue->Count > SERVER_INPUT_MAX_WAITING)
        {
            Take = Queue->Count - SERVER_INPUT_KEEP_WAITING;
        }
        for (u32 Index = 0; Index < Take; ++Index)
        {
            net_input Input = PopInput(Queue);
            GameApplyInput(Game, Slot, &Input);
        }
    }
}
