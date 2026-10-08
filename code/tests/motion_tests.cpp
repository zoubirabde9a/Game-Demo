/* Motion tests: how smoothly a real client draws moving players while
   online. The client runs at a monitor's frame rate (about 144 a second,
   each frame a little uneven) while the server ticks at its own 60, over
   a link that delays each packet by a random few milliseconds. The local
   player walks back and forth; a second player, on its own connection,
   walks up and down, out of its way, so the first sees it as a replica.

   Every frame the test reads where each of the two is drawn and works out
   the speed it appears to move at. Walking only ever changes that speed
   by the run-up or the stop rate (RunSpeed over RunUpSeconds); anything
   much bigger in one frame is a jolt the eye sees: a correction snapping
   in, a replica stopping to wait for a late snapshot. Included by
   server_tests.cpp, which calls RunMotionTests. */

struct motion_track
{
    bool32 HasLast;
    bool32 HasSpeed;
    v2 Last;
    v2 LastSpeed;
    u32 Frames;
    u32 Jolts;
    float WorstChange;
    float SumChangeSq;
};

// NOTE: one frame of a drawn position; a change of apparent speed bigger
// than Allowed (units per second, in one frame) is a jolt
internal void
TrackMotion(motion_track *Track, v2 Position, float DeltaTime, float Allowed)
{
    if (Track->HasLast)
    {
        v2 Speed = (1.0f / DeltaTime) * (Position - Track->Last);
        if (Track->HasSpeed)
        {
            float Change = Length(Speed - Track->LastSpeed);
            Track->Frames++;
            Track->SumChangeSq += Change * Change;
            if (Change > Track->WorstChange) Track->WorstChange = Change;
            if (Change > Allowed) Track->Jolts++;
        }
        Track->LastSpeed = Speed;
        Track->HasSpeed = true;
    }
    Track->Last = Position;
    Track->HasLast = true;
}

internal void
ResetMotionTrack(motion_track *Track)
{
    *Track = {};
}

struct motion_result
{
    bool32 Joined;
    motion_track Local;
    motion_track Remote;
    // NOTE: the camera's own position, before it is snapped to pixels
    motion_track Camera;
    v2 LocalEnd;
    u32 Corrections;
    float CorrectedDistance;
    float LocalTravel;
    float RemoteTravel;
};

internal void
PrintMotionTrack(const char *Name, motion_track *Track)
{
    float Rms = Track->Frames ? sqrtf(Track->SumChangeSq / (float)Track->Frames) : 0.0f;
    printf("    %-7s %4u frames, %3u jolts, worst speed change %5.0f/frame, typical %4.0f\n",
           Name, Track->Frames, Track->Jolts, Track->WorstChange, Rms);
}

// NOTE: MaxDelayMs is the most a packet waits on the link either way;
// each waits a random amount up to it, so packets also arrive out of order
internal motion_result
PlayMotion(float FrameRate, u32 MaxDelayMs, u32 DropPercent, u32 Seed,
           u32 EdgePattern = 0, bool32 Offline = false, u32 MapId = MapId_Arena)
{
    motion_result Result = {};
    static server Server;
    static lossy_link Link;
    static net_client Mover;
    Check(ServerStart(&Server, 0, MapId));
    ClearMonsters(&Server);

    app_state *Client = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    memory_arena Arena, Constants;
    InitializeArena(&Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    InitSimulation(Client, &Arena, &Constants);
    AddPlayerToSlot(Client, &Client->World, &Arena, 0, PlayerSpawnPosition(&Client->World, 0));
    Client->RewindFx = (rewind_fx *)calloc(1, sizeof(rewind_fx));
    SetEnvironment(ONLINE_ADDRESS_ENV, "");
    Client->Online = StartOnlineSession(&Arena);
    online_session *Online = Client->Online;

    float FrameSeconds = 1.0f / FrameRate;
    u32 DelayFrames = (u32)((float)MaxDelayMs * 0.001f / FrameSeconds + 0.5f);
    Check(LossyOpen(&Link, LocalServer(&Server), DropPercent, 0, DelayFrames, Seed));
    char Address[32];
    net_address LinkAddress = LossyAddress(&Link);
    snprintf(Address, sizeof(Address), "127.0.0.1:%u", LinkAddress.Port);
    if (!Offline) Check(OnlineConnect(Online, Address, "Watcher"));
    Check(NetClientConnect(&Mover, LocalServer(&Server), 4321, SimContentId(), "Mover"));

    // NOTE: walking changes speed by at most RunSpeed / StopSeconds a
    // second, and the server steps it a tick at a time; a jolt is a change
    // in one frame twice the biggest one walking makes in a tick
    float Allowed = 2.0f * Maximum(FrameSeconds, 1.0f / SERVER_TICK_RATE) *
        PlayerStats.RunSpeed / Minimum(PlayerStats.RunUpSeconds, PlayerStats.StopSeconds);
    float ServerClock = 0.0f;
    u32 Random = Seed * 747796405u + 1;
    app_input Input = {};
    float Seconds = 0.0f;
    float TotalSeconds = EdgePattern ? 16.0f : 8.0f;
    float MeasureFrom = EdgePattern ? 9.0f : 2.0f;
    app_window Window = {};
    Window.Width = 1920;
    Window.Height = 1080;
    v2 LocalStart = {}, RemoteStart = {};
    bool32 Measuring = false;
    while (Seconds < TotalSeconds)
    {
        // NOTE: a frame within 10% of the nominal length either way, as a
        // real game's frames are never exactly even
        Random ^= Random << 13; Random ^= Random >> 17; Random ^= Random << 5;
        float Wobble = 0.9f + 0.2f * (float)(Random % 1000) / 1000.0f;
        float DeltaTime = FrameSeconds * Wobble;
        Seconds += DeltaTime;
        Input.DeltaTime = DeltaTime;

        // NOTE: the local player walks right then left, a second each way;
        // the mover walks away first, then down and up, 0.8 s each way, so
        // the two never touch (a bump is a real disagreement, not a network one)
        bool32 Right = ((int)Seconds % 2) == 0;
        Input.ButtonD.EndedDown = Right;
        Input.ButtonQ.EndedDown = !Right;
        // NOTE: edge patterns run up and left into the corner for 9 s,
        // then 1: keep pushing; 2: slide right and back along the top
        // edge; 3: step down off the top edge and back into it
        // NOTE: pattern 4 wanders: a new one of the eight directions every
        // 0.7 s, over whatever ground the map has there
        if (EdgePattern == 4)
        {
            u32 Way = ((u32)(Seconds / 0.7f) * 3) % 8;
            Input.ButtonD.EndedDown = Way == 0 || Way == 1 || Way == 7;
            Input.ButtonQ.EndedDown = Way == 3 || Way == 4 || Way == 5;
            Input.ButtonS.EndedDown = Way == 1 || Way == 2 || Way == 3;
            Input.ButtonZ.EndedDown = Way == 5 || Way == 6 || Way == 7;
        }
        else if (EdgePattern)
        {
            bool32 Settle = Seconds < 9.0f || EdgePattern == 1;
            float Phase = Seconds - 9.0f;
            Input.ButtonQ.EndedDown = Settle || (EdgePattern == 2 && (int)(Phase / 1.2f) % 2 == 1);
            Input.ButtonD.EndedDown = !Settle && EdgePattern == 2 && (int)(Phase / 1.2f) % 2 == 0;
            Input.ButtonZ.EndedDown = Settle || EdgePattern == 2 ||
                (EdgePattern == 3 && fmodf(Phase, 1.0f) > 0.35f);
            Input.ButtonS.EndedDown = !Settle && EdgePattern == 3 && fmodf(Phase, 1.0f) <= 0.35f;
        }

        UpdateOnlineSession(Online, &Input);
        LossyPump(&Link);
        ServerClock += DeltaTime;
        while (ServerClock >= 1.0f / SERVER_TICK_RATE)
        {
            ServerClock -= 1.0f / SERVER_TICK_RATE;
            u32 MoverButtons = (u32)((int)(Seconds / 0.8f) % 2) ? NetButton_Up : NetButton_Down;
            if (Seconds < 1.5f) MoverButtons = NetButton_Down;
            // NOTE: a wander crosses the spawn, so the mover leaves it
            if (EdgePattern == 4) MoverButtons = NetButton_Left | NetButton_Up;
            NetClientUpdate(&Mover, 1.0f / SERVER_TICK_RATE, MoverButtons, 0, 0);
            ServerTick(&Server);
        }
        LossyPump(&Link);
        if (Offline)
        {
            // NOTE: walking only; the keyboard reader also needs the cast
            // targeting the game's startup makes
            player_input Walk = {};
            Walk.Move.X = Input.ButtonD.EndedDown ? 1.f : (Input.ButtonQ.EndedDown ? -1.f : 0.f);
            Walk.Move.Y = Input.ButtonS.EndedDown ? 1.f : (Input.ButtonZ.EndedDown ? -1.f : 0.f);
            Client->Players[Client->LocalPlayerIndex].Input = Walk;
        }
        RunWorldTick(Client, &Arena, DeltaTime);

        if ((!Offline && !IsOnline(Online)) || Mover.State != NetClient_Connected) continue;
        Result.Joined = true;
        world_entity *Local = GetLocalPlayer(Client);
        world_entity *Remote = Offline ? Local : Client->Players[Mover.PlayerIndex].Entity;
        if (!Local || !Remote || !Remote->IsPresent) continue;
        if (Seconds >= MeasureFrom && !Measuring)
        {
            Measuring = true;
            LocalStart = Local->Position.XY;
            RemoteStart = Remote->Position.XY;
        }
        if (Measuring)
        {
            TrackMotion(&Result.Local, Local->Position.XY, DeltaTime, Allowed);
            TrackMotion(&Result.Remote, Remote->Position.XY, DeltaTime, Allowed);
            app_window View = GetWorldView(Client, &Window);
            UpdateCamera(Client, &View, &Input, false);
            TrackMotion(&Result.Camera, Client->CameraOffset.XY, DeltaTime, Allowed);
            Result.LocalTravel += Length(Local->Position.XY - LocalStart);
            Result.RemoteTravel += Length(Remote->Position.XY - RemoteStart);
            LocalStart = Local->Position.XY;
            RemoteStart = Remote->Position.XY;
        }
    }

    if (GetLocalPlayer(Client)) Result.LocalEnd = GetLocalPlayer(Client)->Position.XY;
    Result.Corrections = Online->Prediction.Corrections;
    Result.CorrectedDistance = Online->Prediction.CorrectedDistance;
    NetClientDisconnect(&Mover);
    OnlineDisconnect(Online);
    NetCloseSocket(&Link.Socket);
    ServerStop(&Server);
    free(Client->RewindFx);
    free(Arena.Base);
    free(Constants.Base);
    free(Client);
    return Result;
}

// The local player and a remote one move without jolts at a high frame
// rate over a clean link, over a jittery one, and at 60 frames a second.
internal void
TestOnlineMotionIsSmooth()
{
    struct motion_case { const char *Name; float FrameRate; u32 DelayMs; u32 Drop; };
    motion_case Cases[] =
    {
        {"144 fps, clean link", 144.0f, 0, 0},
        {"144 fps, 0-30 ms jitter", 144.0f, 30, 0},
        {"60 fps, 0-30 ms jitter", 60.0f, 30, 0},
        {"144 fps, jitter and 5% loss", 144.0f, 30, 5},
    };
    for (u32 Index = 0; Index < ArrayCount(Cases); ++Index)
    {
        motion_case *Case = &Cases[Index];
        motion_result Result = PlayMotion(Case->FrameRate, Case->DelayMs, Case->Drop, 11 + Index);
        printf("  motion, %s: local walked %.0f, remote %.0f\n", Case->Name,
               Result.LocalTravel, Result.RemoteTravel);
        PrintMotionTrack("local", &Result.Local);
        PrintMotionTrack("remote", &Result.Remote);
        Check(Result.Joined);
        Check(Result.LocalTravel > 500.0f);
        Check(Result.RemoteTravel > 500.0f);
        // NOTE: before inputs went out once a tick and replicas were drawn
        // between buffered snapshots, a quarter of the local player's
        // frames and a tenth of the remote one's were jolts
        Check(Result.Local.Jolts * 100 <= Result.Local.Frames);
        Check(Result.Remote.Jolts * 100 <= Result.Remote.Frames);
    }
}

// The local player runs into the arena's corner, then pushes into it,
// slides along the top edge, or steps off the raised edge and back, over
// a jittery, lossy link. The server must never have to correct it, and it
// must move as smoothly as the same walk offline. Snapshots used to send
// its position rounded to 1/8 unit; replayed from there, it went round
// the wall's corners differently from the server, and was pulled back
// 20 times a second all along the edges.
internal void
TestOnlineEdgesAreSmooth()
{
    const char *Names[] = {"", "pushing into the corner", "sliding along the edge",
                           "stepping off the edge and back"};
    for (u32 Pattern = 1; Pattern <= 3; ++Pattern)
    {
        motion_result Online = PlayMotion(144.0f, 30, 5, 40 + Pattern, Pattern);
        motion_result Offline = PlayMotion(144.0f, 0, 0, 40 + Pattern, Pattern, true);
        printf("  edge, %s: %u corrections from the server (%.1f units)\n",
               Names[Pattern], Online.Corrections, Online.CorrectedDistance);
        PrintMotionTrack("online", &Online.Local);
        PrintMotionTrack("offline", &Offline.Local);
        PrintMotionTrack("camera", &Online.Camera);
        Check(Online.Joined);
        Check(Online.Corrections <= 2);
        Check(Online.Local.Jolts <= Offline.Local.Jolts + 3);
        Check(Online.Camera.Jolts <= Offline.Local.Jolts + 3);
    }
}

// The local player wanders over every map, mud, water, snow and ice
// included. Prediction must walk each kind of ground as the server does:
// it once walked everything at stone-floor speed.
internal void
TestOnlineGroundPredictsExactly()
{
    for (u32 MapId = 0; MapId < MapId_Count; ++MapId)
    {
        motion_result Online = PlayMotion(144.0f, 30, 5, 60 + MapId, 4, false, MapId);
        printf("  wandering on map %u: %u corrections from the server (%.1f units), walked %.0f\n",
               MapId, Online.Corrections, Online.CorrectedDistance, Online.LocalTravel);
        Check(Online.Joined);
        Check(Online.Corrections <= 3);
    }
}

internal void
RunMotionTests()
{
    GROUP(TestOnlineMotionIsSmooth());
    GROUP(TestOnlineEdgesAreSmooth());
    GROUP(TestOnlineGroundPredictsExactly());
}
