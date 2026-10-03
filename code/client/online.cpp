/* Online session: when a server address is set, the client connects to
   the dedicated server, sends the keys held each frame, and keeps the
   newest snapshot in Online->Client.Snapshot; RunWorldTick then draws the
   world from it instead of simulating. With no address it stays offline
   and runs its own simulation.

   The address and name come from the environment or server.txt
   (online_config.cpp). The connect screen (ui/connect_screen.cpp) calls
   OnlineConnect and OnlineDisconnect. When a connection ends for a reason
   worth retrying (server restarted, link lost, no answer) the session
   reconnects by itself, ONLINE_RECONNECT_TRIES times, waiting longer each
   time. The browser build has no UDP and is always offline. */

#define ONLINE_RECONNECT_TRIES 5
// NOTE(zoubir): the first retry waits this long, each later one this
// much more
#define ONLINE_RECONNECT_STEP 2.f

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
    // NOTE(zoubir): the last address typed could not be read as a.b.c.d:port
    bool32 BadAddress;
    char AddressText[64];
    char NameText[NET_NAME_SIZE];
    // NOTE(zoubir): automatic reconnects since the last successful join,
    // and seconds until the next one
    u32 Reconnects;
    float ReconnectIn;
    // NOTE(zoubir): local copies of the server's entities, client/replicas.cpp
    replica_table Replicas;
    // NOTE(zoubir): inputs the server has not applied yet, client/prediction.cpp
    prediction_history Prediction;
#if !COMPILER_EMSCRIPTEN
    net_client Client;
#endif
};


#if !COMPILER_EMSCRIPTEN

// NOTE(zoubir): the held keys, in the network's button bits. The server
// turns new presses into actions itself.
internal u16
NetButtonsFromKeyboard(app_input *Input)
{
    u16 Result = 0;
    if (Input->ButtonQ.EndedDown) Result |= NetButton_Left;
    if (Input->ButtonD.EndedDown) Result |= NetButton_Right;
    if (Input->ButtonZ.EndedDown) Result |= NetButton_Up;
    if (Input->ButtonS.EndedDown) Result |= NetButton_Down;
    if (Input->SpaceButton.EndedDown) Result |= NetButton_Jump;
    if (Input->AltButton.EndedDown) Result |= NetButton_Dash;
    if (Input->LeftButton.EndedDown) Result |= NetButton_Fireball;
    if (Input->RightButton.EndedDown) Result |= NetButton_Sword;
    if (Input->ButtonE.EndedDown) Result |= NetButton_Shockwave;
    return Result;
}


// NOTE(zoubir): leaves the server (if any) and goes back to the local game
internal void
OnlineDisconnect(online_session *Online)
{
    Online->Reconnects = ONLINE_RECONNECT_TRIES;
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
    if (Address != Online->AddressText)
    {
        CopyString(Online->AddressText, sizeof(Online->AddressText), Address);
    }
    if (Name != Online->NameText)
    {
        CopyString(Online->NameText, sizeof(Online->NameText), Name);
    }
    net_address Server;
    Online->BadAddress = !NetParseAddress(Online->AddressText, &Server);
    if (!Online->BadAddress && NetSocketsStartup())
    {
        u32 Salt = (u32)time(0) ^ (u32)(size_t)Online ^ Online->Client.Salt;
        Online->Enabled = NetClientConnect(&Online->Client, Server, Salt,
                                           SimContentId(), Online->NameText);
    }
    return Online->Enabled;
}

// NOTE(zoubir): a connection that ended on its own (not refused, not
// left) and has tries left
internal bool32
WillReconnect(online_session *Online)
{
    net_client_end Reason = Online->Client.EndReason;
    bool32 Result = Online->Enabled &&
        Online->Client.State == NetClient_Disconnected &&
        Online->Reconnects < ONLINE_RECONNECT_TRIES &&
        (Reason == NetEnd_ServerClosed || Reason == NetEnd_LostConnection ||
         Reason == NetEnd_NoAnswer);
    return Result;
}

internal online_session *
StartOnlineSession(memory_arena *Arena)
{
    online_session *Online = AllocateStruct(Arena, online_session);
    *Online = {};
    if (ReadOnlineConfig(Online->AddressText, sizeof(Online->AddressText),
                         Online->NameText, sizeof(Online->NameText)))
    {
        OnlineConnect(Online, Online->AddressText, Online->NameText);
    }
    return Online;
}

// NOTE(zoubir): KeysToUi while a screen takes the keyboard: the player
// holds nothing. Aim is the unit vector toward the cursor (zero: no change).
internal void
UpdateOnlineSession(online_session *Online, app_input *Input,
                    bool32 KeysToUi = false, v2 Aim = {})
{
    if (Online && Online->Enabled)
    {
        u16 Buttons = KeysToUi ? 0 : NetButtonsFromKeyboard(Input);
        NetClientUpdate(&Online->Client, Input->DeltaTime, Buttons, Aim.X, Aim.Y);
        if (Online->Client.State == NetClient_Connected)
        {
            Online->Reconnects = 0;
            Online->ReconnectIn = 0.f;
            RecordPredictedInput(&Online->Prediction, Online->Client.InputTick,
                                 Buttons, Input->DeltaTime, Aim);
        }
        else if (WillReconnect(Online))
        {
            if (Online->ReconnectIn <= 0.f)
            {
                Online->ReconnectIn =
                    ONLINE_RECONNECT_STEP * (float)(Online->Reconnects + 1);
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

internal void
GetOnlineStatusText(online_session *Online, char *Out, u32 OutSize)
{
    Out[0] = 0;
    if (Online && Online->BadAddress)
    {
        snprintf(Out, OutSize, "Offline: write the address as a.b.c.d:port");
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
            snprintf(Out, OutSize, "Connecting to %s", Online->AddressText);
        } break;
        case NetClient_Connected:
        {
            if (Online->NameText[0])
            {
                snprintf(Out, OutSize, "Online at %s as %s",
                         Online->AddressText, Online->NameText);
            }
            else
            {
                snprintf(Out, OutSize, "Online at %s as Player %u",
                         Online->AddressText, Client->PlayerIndex + 1);
            }
        } break;
        case NetClient_Disconnected:
        {
            if (WillReconnect(Online))
            {
                snprintf(Out, OutSize, "Connection lost, reconnecting in %.0f s",
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
        // NOTE(zoubir): joining a server on another map: build its ground
        // first. Terrain is never sent, both sides generate it from the id
        if (!Online->Replicas.Active &&
            Online->Client.MapId != AppState->World.MapId)
        {
            RebuildWorldForMap(AppState, Arena, Online->Client.MapId);
            Online->Replicas = {};
        }
        bool32 NewSnapshot = !Online->Replicas.Active ||
            Snapshot->Tick != Online->Replicas.LastAppliedTick;
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
        PredictLocalPlayer(AppState, Arena, &Online->Prediction, NewSnapshot,
                           Snapshot->InputTick, DeltaTime);
        return;
    }
    if (Online && Online->Replicas.Active)
    {
        LeaveReplicaWorld(AppState, Arena, &Online->Replicas);
        Online->Prediction = {};
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
StartOnlineSession(memory_arena *Arena)
{
    online_session *Online = AllocateStruct(Arena, online_session);
    *Online = {};
    return Online;
}

internal void
UpdateOnlineSession(online_session *Online, app_input *Input,
                    bool32 KeysToUi = false) {}
internal void OnlineDisconnect(online_session *Online) {}
internal bool32
OnlineConnect(online_session *Online, char *Address, char *Name)
{
    return false;
}
inline bool32 IsOnline(online_session *Online) { return false; }
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

#endif
