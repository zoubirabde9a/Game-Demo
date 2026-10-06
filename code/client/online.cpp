/* Online session: when a server address is set, the client connects to
   the dedicated server, sends the keys held once per server tick
   (online_pacing.cpp), and keeps the
   newest snapshot in Online->Client.Snapshot; RunWorldTick then draws the
   world from it instead of simulating. With no address it stays offline
   and runs its own simulation.

   The address and name come from the environment or server.txt
   (online_config.cpp), else the first server in server_list.cpp. An
   address may be a DNS name; it is looked up when connecting and the
   answer reused by automatic reconnects. The connect screen (ui/connect_screen.cpp) calls
   OnlineConnect and OnlineDisconnect. When a connection ends for a reason
   worth retrying (server restarted or full, link lost, no answer) the
   session keeps reconnecting by itself, waiting longer each time up to
   ONLINE_RECONNECT_MAX_WAIT, until it is back or the player leaves. The
   world plays offline meanwhile. The browser build has no UDP and is
   always offline. */

// NOTE(zoubir): the first retry waits this long, each later one this
// much more, never more than the max
#define ONLINE_RECONNECT_STEP 2.f
#define ONLINE_RECONNECT_MAX_WAIT 15.f

// NOTE(zoubir): what the connect screen shows and offers
enum online_phase
{
    OnlinePhase_Offline,  // never tried, or left by choice
    OnlinePhase_Joining,
    OnlinePhase_Joined,
    OnlinePhase_Ended,    // the connection failed or dropped; can retry
};

struct online_session
{
    bool32 Enabled;
    // NOTE(zoubir): the last address typed could not be read or looked up
    bool32 BadAddress;
    char AddressText[64];
    // NOTE(zoubir): what AddressText resolved to, kept for reconnects
    char ResolvedText[64];
    net_address Resolved;
    char NameText[NET_NAME_SIZE];
    // NOTE(zoubir): set by OnlineConnect, cleared when the player leaves;
    // while set, a dropped connection is retried
    bool32 KeepTrying;
    // NOTE(zoubir): automatic reconnects since the last successful join,
    // and seconds until the next one
    u32 Reconnects;
    float ReconnectIn;
    // NOTE(zoubir): local copies of the server's entities, client/replicas.cpp
    replica_table Replicas;
    // NOTE(zoubir): inputs the server has not applied yet, client/prediction.cpp
    prediction_history Prediction;
    // NOTE(zoubir): round trip and loss, client/online_quality.cpp
    online_quality Quality;
    // NOTE(zoubir): when inputs go out, one per tick, client/online_pacing.cpp
    online_pacing Pacing;
    // NOTE(zoubir): buttons held on any frame since the last tick, so a tap
    // shorter than a tick still reaches the server; and the ticks this
    // frame made, for prediction
    u32 HeldSinceTick;
    u32 NewTicks;
#if !COMPILER_EMSCRIPTEN
    net_client Client;
#endif
};


#if !COMPILER_EMSCRIPTEN

// NOTE(zoubir): the held keys, in the network's button bits. The server
// turns new presses into actions itself.
internal u32
NetButtonsFromKeyboard(app_input *Input)
{
    u32 Result = 0;
    if (LayoutKey(Input, 'Q')->EndedDown) Result |= NetButton_Left;
    if (Input->ButtonD.EndedDown) Result |= NetButton_Right;
    if (LayoutKey(Input, 'Z')->EndedDown) Result |= NetButton_Up;
    if (Input->ButtonS.EndedDown) Result |= NetButton_Down;
    Result |= ActionButtonsFromKeys(Input, false) << PLAYER_BUTTON_NET_SHIFT;
    return Result;
}


// NOTE(zoubir): leaves the server (if any) and goes back to the local game
internal void
OnlineDisconnect(online_session *Online)
{
    Online->KeepTrying = false;
    if (Online->Enabled)
    {
        NetClientDisconnect(&Online->Client);
    }
    Online->Enabled = false;
}

// NOTE(zoubir): drops any current connection and starts connecting to
// Address as Name; false, with BadAddress set, when Address does not parse
internal bool32
OnlineConnect(online_session *Online, char *Address, char *Name)
{
    OnlineDisconnect(Online);
    Online->Reconnects = 0;
    Online->KeepTrying = true;
    ResetOnlineQuality(&Online->Quality);
    ResetOnlinePacing(&Online->Pacing);
    Online->HeldSinceTick = 0;
    Online->NewTicks = 0;
    if (Address != Online->AddressText)
    {
        CopyString(Online->AddressText, sizeof(Online->AddressText), Address);
    }
    if (Name != Online->NameText)
    {
        CopyString(Online->NameText, sizeof(Online->NameText), Name);
    }
    net_address Server = Online->Resolved;
    bool32 Started = NetSocketsStartup();
    bool32 Known = Online->ResolvedText[0] &&
        StringsMatchIgnoringCase(Online->ResolvedText, Online->AddressText);
    if (Started && !Known)
    {
        Known = NetResolveServer(Online->AddressText, &Server);
        if (Known)
        {
            Online->Resolved = Server;
            CopyString(Online->ResolvedText, sizeof(Online->ResolvedText),
                       Online->AddressText);
        }
    }
    Online->BadAddress = Started && !Known;
    if (Started && Known)
    {
        u32 Salt = (u32)time(0) ^ (u32)(size_t)Online ^ Online->Client.Salt;
        Online->Enabled = NetClientConnect(&Online->Client, Server, Salt,
                                           SimContentId(), Online->NameText);
    }
    return Online->Enabled;
}

// NOTE(zoubir): a connection that ended on its own and could come back:
// not left by choice, not turned away for running another version
internal bool32
WillReconnect(online_session *Online)
{
    net_client_end Reason = Online->Client.EndReason;
    bool32 Result = Online->Enabled && Online->KeepTrying &&
        Online->Client.State == NetClient_Disconnected &&
        (Reason == NetEnd_ServerClosed || Reason == NetEnd_LostConnection ||
         Reason == NetEnd_NoAnswer || Reason == NetEnd_ServerFull);
    return Result;
}

// NOTE(zoubir): joins the configured server, or DefaultAddress when none
// is configured (the game passes the live server; tests pass nothing and
// stay offline). An address of "offline" means stay offline
internal online_session *
StartOnlineSession(memory_arena *Arena, char *DefaultAddress = 0)
{
    online_session *Online = AllocateStruct(Arena, online_session);
    ZeroSize(Online, sizeof(*Online));
    if (!ReadOnlineConfig(Online->AddressText, sizeof(Online->AddressText),
                          Online->NameText, sizeof(Online->NameText)) &&
        DefaultAddress)
    {
        CopyString(Online->AddressText, sizeof(Online->AddressText),
                   DefaultAddress);
    }
    if (StringsMatchIgnoringCase(Online->AddressText, ONLINE_OFFLINE_WORD))
    {
        Online->AddressText[0] = 0;
    }
    if (Online->AddressText[0])
    {
        OnlineConnect(Online, Online->AddressText, Online->NameText);
    }
    return Online;
}

// NOTE(zoubir): KeysToUi while a screen takes the keyboard: the player
// holds nothing. Aim is the unit vector toward the cursor (zero: no change).
internal void
UpdateOnlineSession(online_session *Online, app_input *Input,
                    bool32 KeysToUi = false, v2 Aim = {}, u32 LearnBits = 0)
{
    if (Online && Online->Enabled)
    {
        // NOTE(zoubir): LearnBits is the talent field
        // (client/talent_requests.cpp), sent even while a screen has the keys
        u32 Held = KeysToUi ? 0 : NetButtonsFromKeyboard(Input);
        NetClientPoll(&Online->Client, Input->DeltaTime);
        Online->NewTicks = 0;
        if (Online->Client.State == NetClient_Connected)
        {
            Online->Reconnects = 0;
            Online->ReconnectIn = 0.f;
            Online->HeldSinceTick |= Held;
            u32 Ticks = AdvanceOnlinePacing(&Online->Pacing, Input->DeltaTime);
            for(u32 Index = 0; Index < Ticks; Index++)
            {
                u32 Buttons = (Index == 0 ? Online->HeldSinceTick : Held) | LearnBits;
                u32 Tick = NetClientQueueInput(&Online->Client, Buttons, Aim.X, Aim.Y);
                NoteOnlineFrame(&Online->Quality, ONLINE_TICK_SECONDS);
                RecordPredictedInput(&Online->Prediction, Tick, Buttons,
                                     ONLINE_TICK_SECONDS, Aim);
            }
            if (Ticks)
            {
                Online->HeldSinceTick = 0;
            }
            Online->NewTicks = Ticks;
            NetClientFlushInputs(&Online->Client);
        }
        else if (WillReconnect(Online))
        {
            if (Online->ReconnectIn <= 0.f)
            {
                Online->ReconnectIn =
                    Minimum(ONLINE_RECONNECT_MAX_WAIT,
                            ONLINE_RECONNECT_STEP * (float)(Online->Reconnects + 1));
            }
            Online->ReconnectIn -= Input->DeltaTime;
            if (Online->ReconnectIn <= 0.f)
            {
                u32 Tries = Online->Reconnects + 1;
                OnlineConnect(Online, Online->AddressText, Online->NameText);
                Online->Reconnects = Tries;
                Online->ReconnectIn = 0.f;
            }
        }
    }
}

// NOTE(zoubir): true while the server, not the local simulation, owns the
// world
inline bool32
IsOnline(online_session *Online)
{
    bool32 Result = Online && Online->Enabled &&
        Online->Client.State == NetClient_Connected;
    return Result;
}

// NOTE(zoubir): 0 unless joined and measured
internal online_quality *
GetOnlineQuality(online_session *Online)
{
    online_quality *Result = (IsOnline(Online) && Online->Quality.HasSample) ?
        &Online->Quality : 0;
    return Result;
}

// NOTE(zoubir): seconds since the server was last heard from, while joined
inline float
GetOnlineSilence(online_session *Online)
{
    float Result = IsOnline(Online) ? Online->Client.SecondsSinceHeard : 0.f;
    return Result;
}

internal online_phase
GetOnlinePhase(online_session *Online)
{
    online_phase Result = OnlinePhase_Offline;
    if (Online && Online->Enabled)
    {
        switch (Online->Client.State)
        {
            case NetClient_Connecting: Result = OnlinePhase_Joining; break;
            case NetClient_Connected: Result = OnlinePhase_Joined; break;
            case NetClient_Disconnected: Result = OnlinePhase_Ended; break;
        }
    }
    return Result;
}

// NOTE(zoubir): what to call the server: its own name once joined, else
// its name in the server list, else the address as typed
internal char *
GetOnlineServerName(online_session *Online)
{
    char *Result = (char *)"";
    if (Online)
    {
        server_entry *Entry = FindServerByAddress(Online->AddressText);
        Result = Entry ? Entry->Name : Online->AddressText;
        if (IsOnline(Online) && Online->Client.ServerName[0])
        {
            Result = Online->Client.ServerName;
        }
    }
    return Result;
}

internal void
GetOnlineStatusText(online_session *Online, char *Out, u32 OutSize)
{
    Out[0] = 0;
    if (Online && Online->BadAddress)
    {
        snprintf(Out, OutSize, "Offline: cannot find server %s", Online->AddressText);
        return;
    }
    if (!Online || !Online->Enabled)
    {
        return;
    }
    net_client *Client = &Online->Client;
    switch (Client->State)
    {
        case NetClient_Connecting:
        {
            snprintf(Out, OutSize, "Connecting to %s", GetOnlineServerName(Online));
        } break;
        case NetClient_Connected:
        {
            if (Online->NameText[0])
            {
                snprintf(Out, OutSize, "Online at %s as %s",
                         GetOnlineServerName(Online), Online->NameText);
            }
            else
            {
                snprintf(Out, OutSize, "Online at %s as Player %u",
                         GetOnlineServerName(Online), Client->PlayerIndex + 1);
            }
        } break;
        case NetClient_Disconnected:
        {
            if (WillReconnect(Online))
            {
                snprintf(Out, OutSize, "%s, reconnecting in %.0f s",
                         Client->EndReason == NetEnd_ServerFull ?
                         "Server full" : (Client->EndReason == NetEnd_NoAnswer ?
                                          "Server not answering" : "Connection lost"),
                         Maximum(1.f, Online->ReconnectIn + 0.5f));
                return;
            }
            char *Reasons[] = {"disconnected", "no answer from server",
                               "server full", "server closed",
                               "lost connection", "left",
                               "server runs a different version"};
            u32 Reason = (u32)Client->EndReason;
            snprintf(Out, OutSize, "Offline: %s",
                     Reason < ArrayCount(Reasons) ? Reasons[Reason] :
                     "disconnected");
        } break;
    }
}

// NOTE(zoubir): in client/rewind_fx/rewind_fx.cpp, included later
internal void ReadRewindsFromSnapshot(app_state *AppState, replica_table *Replicas,
                                      net_snapshot *Snapshot);

// NOTE(zoubir): the frame's one world update. Online the server's
// snapshot drives the world; offline the local simulation does. Switches
// between the two when the connection comes up or ends.
internal void
RunWorldTick(app_state *AppState, memory_arena *Arena, float DeltaTime)
{
    online_session *Online = AppState->Online;
    if (IsOnline(Online) && Online->Client.HasSnapshot)
    {
        net_snapshot *Snapshot = &Online->Client.Snapshot;
        // NOTE(zoubir): joining a server on another map, or a new round
        // there on the next map: build its ground first, and the replicas
        // again from this snapshot. Terrain is never sent, both sides
        // generate it from the id
        u32 MapId = Snapshot->MapId;
        if (MapId < MapId_Count && MapId != AppState->World.MapId)
        {
            RebuildWorldForMap(AppState, Arena, MapId);
            ZeroSize(&Online->Replicas, sizeof(Online->Replicas));
            Online->Prediction = {};
        }
        bool32 NewSnapshot = !Online->Replicas.Active ||
            Snapshot->Tick != Online->Replicas.LastAppliedTick;
        if (NewSnapshot)
        {
            // NOTE(zoubir): inputs waiting on the server already arrived,
            // so they are not part of the round trip
            RecordSnapshotQuality(&Online->Quality, Snapshot->Tick,
                                  Online->Client.InputTick,
                                  Snapshot->InputTick + Snapshot->InputBuffered);
            NotePacingSnapshot(&Online->Pacing, Snapshot->InputBuffered);
        }
        SyncReplicas(AppState, Arena, &Online->Replicas, Snapshot, DeltaTime,
                     Online->Client.PlayerIndex);
        // NOTE(zoubir): the server's sounds go where the local game's go;
        // PlaySimEvents plays them after this tick
        for(u32 Index = 0; NewSnapshot && Index < Snapshot->SoundCount; Index++)
        {
            if (Snapshot->Sounds[Index] < AssetType_Count)
            {
                EmitSound(&AppState->Events,
                          (asset_type_id)Snapshot->Sounds[Index], V3(0.f));
            }
        }
        for(u32 Index = 0; NewSnapshot && Index < Snapshot->KillCount; Index++)
        {
            net_kill *Kill = &Snapshot->Kills[Index];
            EmitKill(&AppState->Events, Kill->Killer, Kill->Victim,
                     Kill->KillerMonster);
        }
        for(u32 Index = 0; NewSnapshot && Index < Snapshot->BurstCount; Index++)
        {
            net_burst *Burst = &Snapshot->Bursts[Index];
            if (!IsBurstPredictedHere(Burst->Kind, Burst->Slot,
                                      Online->Client.PlayerIndex))
            {
                EmitBurst(&AppState->Events, (sim_burst)Burst->Kind,
                          Burst->Slot, V3(Burst->X, Burst->Y, Burst->Z),
                          (float)Burst->Angle * (Pi32 / 128.f));
            }
        }
        // NOTE(zoubir): before prediction, which leaves a player a time
        // rewind froze where the server has it (client/rewind_fx/)
        if (NewSnapshot)
        {
            ReadRewindsFromSnapshot(AppState, &Online->Replicas, Snapshot);
        }
        PredictLocalPlayer(AppState, Arena, &Online->Prediction, NewSnapshot,
                           Snapshot->InputTick, Online->NewTicks,
                           OnlinePacingBlend(&Online->Pacing), DeltaTime);
        return;
    }
    if (Online && Online->Replicas.Active)
    {
        LeaveReplicaWorld(AppState, Arena, &Online->Replicas);
        Online->Prediction = {};
        // NOTE(zoubir): the history was of the world before the server's
        ResetTimeRewind(AppState->Rewind);
    }
    SimulateTick(AppState, Arena, DeltaTime);
}

#else

internal void
RunWorldTick(app_state *AppState, memory_arena *Arena, float DeltaTime)
{
    SimulateTick(AppState, Arena, DeltaTime);
}

internal online_session *
StartOnlineSession(memory_arena *Arena, char *DefaultAddress = 0)
{
    online_session *Online = AllocateStruct(Arena, online_session);
    ZeroSize(Online, sizeof(*Online));
    return Online;
}

internal void
UpdateOnlineSession(online_session *Online, app_input *Input,
                    bool32 KeysToUi = false, v2 Aim = {}, u32 LearnBits = 0) {}
internal void OnlineDisconnect(online_session *Online) {}
internal bool32
OnlineConnect(online_session *Online, char *Address, char *Name)
{
    return false;
}
inline bool32 IsOnline(online_session *Online) { return false; }
internal online_quality *GetOnlineQuality(online_session *Online) { return 0; }
inline float GetOnlineSilence(online_session *Online) { return 0.f; }
internal online_phase
GetOnlinePhase(online_session *Online)
{
    return OnlinePhase_Offline;
}
internal void
GetOnlineStatusText(online_session *Online, char *Out, u32 OutSize)
{
    Out[0] = 0;
}
internal char *GetOnlineServerName(online_session *Online) { return (char *)""; }

#endif
