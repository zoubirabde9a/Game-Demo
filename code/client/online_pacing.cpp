/* Online pacing: when the client sends its inputs. The server applies one
   input per tick (server/input_queue.cpp), so the client makes one per
   tick too, NET_TICK_RATE a second, whatever its frame rate: a 144 Hz
   frame makes none or one, a 30 Hz frame two. Prediction steps the local
   player once per input, so it walks exactly the steps the server will.

   The two clocks drift and the link's delay wobbles, so the client nudges
   its own tick rate, a few percent at most, from what each snapshot says
   about the server's queue (InputBuffered): faster while the server
   sometimes runs out of inputs, slower while more than one or two are
   waiting, since each one waiting adds a tick of delay before the others
   see the player move. online.cpp calls AdvanceOnlinePacing every frame
   and NotePacingSnapshot on every new snapshot. */

#define ONLINE_TICK_SECONDS (1.f / (float)NET_TICK_RATE)
// NOTE(zoubir): ticks one frame may make; after a longer hitch the rest
// are skipped rather than sent in a burst
#define ONLINE_PACING_MAX_STEPS 4
// NOTE(zoubir): the tick rate moves by this share per input of difference
// from the target, at most ONLINE_PACING_MAX_ADJUST either way
#define ONLINE_PACING_GAIN 0.02f
#define ONLINE_PACING_MAX_ADJUST 0.05f
// NOTE(zoubir): the fewest inputs the server should have waiting, the
// lowest seen over a window of this many snapshots (about a second)
#define ONLINE_PACING_TARGET 1
#define ONLINE_PACING_WINDOW 20

struct online_pacing
{
    // NOTE(zoubir): seconds toward the next tick, 0..ONLINE_TICK_SECONDS
    float Clock;
    // NOTE(zoubir): 1 ticks at NET_TICK_RATE; 1.02 runs 2% faster
    float Rate;
    u32 WindowSnapshots;
    u32 WindowLowest;
    u32 LastWindowLowest;
};

internal void
ResetOnlinePacing(online_pacing *Pacing)
{
    *Pacing = {};
    Pacing->Rate = 1.f;
    Pacing->WindowLowest = 0xffffffff;
    Pacing->LastWindowLowest = ONLINE_PACING_TARGET;
}

// NOTE(zoubir): the ticks this frame makes
internal u32
AdvanceOnlinePacing(online_pacing *Pacing, float DeltaTime)
{
    if (Pacing->Rate <= 0.f)
    {
        ResetOnlinePacing(Pacing);
    }
    Pacing->Clock += DeltaTime * Pacing->Rate;
    u32 Result = 0;
    while (Pacing->Clock >= ONLINE_TICK_SECONDS && Result < ONLINE_PACING_MAX_STEPS)
    {
        Pacing->Clock -= ONLINE_TICK_SECONDS;
        Result++;
    }
    if (Pacing->Clock >= ONLINE_TICK_SECONDS)
    {
        Pacing->Clock = 0.f;
    }
    return Result;
}

// NOTE(zoubir): how far this frame stands between the last tick and the
// next, 0..1, for drawing the local player between its two newest steps
inline float
OnlinePacingBlend(online_pacing *Pacing)
{
    float Result = Pacing->Clock / ONLINE_TICK_SECONDS;
    return Minimum(1.f, Maximum(0.f, Result));
}

internal void
NotePacingSnapshot(online_pacing *Pacing, u32 InputBuffered)
{
    Pacing->WindowLowest = Minimum(Pacing->WindowLowest, InputBuffered);
    if (++Pacing->WindowSnapshots >= ONLINE_PACING_WINDOW)
    {
        Pacing->LastWindowLowest = Pacing->WindowLowest;
        Pacing->WindowLowest = 0xffffffff;
        Pacing->WindowSnapshots = 0;
    }
    // NOTE(zoubir): the lower of this window so far and the last one, so a
    // run-out speeds the ticks up at once and a calm second slows them down
    u32 Lowest = Minimum(Pacing->WindowLowest, Pacing->LastWindowLowest);
    float Error = (float)ONLINE_PACING_TARGET - (float)Lowest;
    float Adjust = ONLINE_PACING_GAIN * Error;
    Adjust = Maximum(-ONLINE_PACING_MAX_ADJUST, Minimum(ONLINE_PACING_MAX_ADJUST, Adjust));
    Pacing->Rate = 1.f + Adjust;
}
