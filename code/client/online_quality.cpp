/* Connection quality, measured from the snapshots the client receives:
   round trip (how many of our input frames the server is behind, from the
   InputTick each snapshot echoes) and snapshot loss (snapshots that never
   arrived, from the gaps in their ticks). online.cpp feeds it every new
   snapshot; the HUD's connection indicator reads it. */

// NOTE(zoubir): the server sends a snapshot every this many ticks
// (SERVER_SNAPSHOT_INTERVAL in server/server.h, not in the client build)
#define ONLINE_SNAPSHOT_TICKS 3
#define ONLINE_INPUT_HZ 60.f
// NOTE(zoubir): loss is counted over this many server ticks, then restarts
#define ONLINE_LOSS_WINDOW_TICKS 120

struct online_quality
{
    bool32 HasSample;
    // NOTE(zoubir): smoothed, so one late snapshot does not make it jump
    float RoundTripMs;
    // NOTE(zoubir): 0..1, over the last full window
    float Loss;

    u32 LastSnapshotTick;
    u32 WindowStartTick;
    u32 WindowSnapshots;
};

internal void
ResetOnlineQuality(online_quality *Quality)
{
    *Quality = {};
}

internal void
RecordSnapshotQuality(online_quality *Quality, u32 SnapshotTick,
                      u32 InputTickNow, u32 InputTickApplied)
{
    float Frames = (InputTickNow > InputTickApplied) ?
        (float)(InputTickNow - InputTickApplied) : 0.f;
    float RoundTripMs = Frames * 1000.f / ONLINE_INPUT_HZ;
    if (!Quality->HasSample)
    {
        Quality->HasSample = true;
        Quality->RoundTripMs = RoundTripMs;
        Quality->WindowStartTick = SnapshotTick;
    }
    else
    {
        Quality->RoundTripMs += 0.1f * (RoundTripMs - Quality->RoundTripMs);
    }

    Quality->LastSnapshotTick = SnapshotTick;
    Quality->WindowSnapshots++;
    u32 Span = SnapshotTick - Quality->WindowStartTick;
    if (Span >= ONLINE_LOSS_WINDOW_TICKS)
    {
        float Expected = (float)Span / (float)ONLINE_SNAPSHOT_TICKS;
        float Received = (float)(Quality->WindowSnapshots - 1);
        Quality->Loss = Maximum(0.f, 1.f - Received / Expected);
        Quality->WindowStartTick = SnapshotTick;
        Quality->WindowSnapshots = 1;
    }
}
