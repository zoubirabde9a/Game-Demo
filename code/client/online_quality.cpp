/* Connection quality, measured from the snapshots the client receives:
   round trip (how many of our input frames the server is behind, from the
   InputTick each snapshot echoes) and snapshot loss (snapshots that never
   arrived, from the gaps in their ticks). online.cpp feeds it every new
   snapshot; the HUD's connection indicator reads it. */

// NOTE(zoubir): loss is counted over this many server ticks, then restarts
#define ONLINE_LOSS_WINDOW_TICKS 120

struct online_quality
{
    bool32 HasSample;
    // NOTE(zoubir): smoothed, so one late snapshot does not make it jump
    float RoundTripMs;
    // NOTE(zoubir): 0..1, over the last full window
    float Loss;
    // NOTE(zoubir): average seconds between the inputs we send: one a
    // server tick (online_pacing.cpp). It was one a frame once, 7 ms at
    // 144 fps, and counting those as 16.7 ms made the round trip 2.4 times
    // too long
    float FrameSeconds;

    u32 LastSnapshotTick;
    // NOTE(zoubir): server ticks between snapshots, learned as the smallest
    // gap seen (a lost snapshot only makes a gap bigger), so the client
    // needs no copy of the server's SERVER_SNAPSHOT_INTERVAL; 0 = not yet
    u32 SnapshotStep;
    u32 WindowStartTick;
    u32 WindowSnapshots;
};

internal void
ResetOnlineQuality(online_quality *Quality)
{
    *Quality = {};
}

// NOTE(zoubir): every input sent, with the time it covers
internal void
NoteOnlineFrame(online_quality *Quality, float DeltaTime)
{
    if (DeltaTime <= 0.f) return;
    Quality->FrameSeconds = (Quality->FrameSeconds == 0.f) ? DeltaTime :
        Quality->FrameSeconds + 0.05f * (DeltaTime - Quality->FrameSeconds);
}

internal void
RecordSnapshotQuality(online_quality *Quality, u32 SnapshotTick,
                      u32 InputTickNow, u32 InputTickApplied)
{
    float Frames = (InputTickNow > InputTickApplied) ?
        (float)(InputTickNow - InputTickApplied) : 0.f;
    float FrameSeconds = (Quality->FrameSeconds > 0.f) ? Quality->FrameSeconds : 1.f / 60.f;
    float RoundTripMs = Frames * FrameSeconds * 1000.f;
    if (Quality->HasSample && SnapshotTick > Quality->LastSnapshotTick)
    {
        u32 Gap = SnapshotTick - Quality->LastSnapshotTick;
        if (Quality->SnapshotStep == 0 || Gap < Quality->SnapshotStep)
        {
            Quality->SnapshotStep = Gap;
        }
    }
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
    if (Span >= ONLINE_LOSS_WINDOW_TICKS && Quality->SnapshotStep)
    {
        float Expected = (float)Span / (float)Quality->SnapshotStep;
        float Received = (float)(Quality->WindowSnapshots - 1);
        Quality->Loss = Maximum(0.f, 1.f - Received / Expected);
        Quality->WindowStartTick = SnapshotTick;
        Quality->WindowSnapshots = 1;
    }
}
