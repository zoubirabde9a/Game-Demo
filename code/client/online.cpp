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
    // NOTE(zoubir): a server was set at launch but no name, so the session
    // waits offline for the player to pick one (ui/name_prompt.cpp)
    bool32 NeedsName;
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
    // NOTE(zoubir): when the mouse moves these keys are spells; the walk
    // comes in with the other bits (client/click_move.cpp, app.cpp)
    if (!MouseMoves())
    {
        if (MoveKeyDown(Input, BINDING_MOVE_LEFT)) Result |= NetButton_Left;
        if (MoveKeyDown(Input, BINDING_MOVE_RIGHT)) Result |= NetButton_Right;
        if (MoveKeyDown(Input, BINDING_MOVE_UP)) Result |= NetButton_Up;
        if (MoveKeyDown(Input, BINDING_MOVE_DOWN)) Result |= NetButton_Down;
    }
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
// stay offline). An address of "offline" means stay offline. With
// RequireName and no name saved it does not join yet: NeedsName is set
// and the connect screen asks for one first
internal online_session *
StartOnlineSession(memory_arena *Arena, char *DefaultAddress = 0,
                   bool32 RequireName = false)
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
    char Name[NET_NAME_SIZE];
    CleanPlayerName(Name, sizeof(Name), Online->NameText);
    CopyString(Online->NameText, sizeof(Online->NameText), Name);
    Online->NeedsName = RequireName && Online->AddressText[0] &&
        PlayerNameProblem(Online->NameText) != 0;
    if (Online->AddressText[0] && !Online->NeedsName)
    {
        OnlineConnect(Online, Online->AddressText, Online->NameText);
    }
    return Online;
}

// NOTE(zoubir): the server's Id + 1 of the replica at local entity index
// + 1 Local, 0 for none: the unit the cursor is on, as the server knows it
internal u16
ReplicaServerTarget(replica_table *Table, u32 Local)
{
    u16 Result = 0;
    for(u32 Id = 0; Local && Id < MAX_REPLICAS; Id++)
    {
        if (Table->LocalIndexPlusOne[Id] == Local)
        {
            Result = (u16)(Id + 1);
            break;
        }
    }
    return Result;
}

// NOTE(zoubir): KeysToUi while a screen takes the keyboard: the player
// holds nothing. Aim is the unit vector toward the cursor (zero: no change).
// Target is player_input.Target, the unit the cursor is on (local entity
// index + 1, client/targeting.cpp), sent as the server's Id + 1
internal void
UpdateOnlineSession(online_session *Online, app_input *Input,
                    bool32 KeysToUi = false, v2 Aim = {}, u32 LearnBits = 0,
                    u32 Target = 0, u32 RoleRequest = 0)
{
    if (Online && Online->Enabled)
    {
        // NOTE(zoubir): LearnBits is the talent field
        // (client/talent_requests.cpp) and RoleRequest the role byte
        // (client/dungeon/role_requests.cpp), sent even while a screen has the keys
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
                u32 Tick = NetClientQueueInput(&Online->Client, Buttons, Aim.X, Aim.Y,
                                               ReplicaServerTarget(&Online->Replicas, Target),
                                               (u8)RoleRequest);
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

// NOTE(zoubir): the frame's world update, from the snapshot or the simulation
#include "online/world_tick.cpp"

#else

internal void
RunWorldTick(app_state *AppState, memory_arena *Arena, float DeltaTime)
{
    SimulateTick(AppState, Arena, DeltaTime);
}

internal online_session *
StartOnlineSession(memory_arena *Arena, char *DefaultAddress = 0,
                   bool32 RequireName = false)
{
    online_session *Online = AllocateStruct(Arena, online_session);
    ZeroSize(Online, sizeof(*Online));
    return Online;
}

internal void
UpdateOnlineSession(online_session *Online, app_input *Input,
                    bool32 KeysToUi = false, v2 Aim = {}, u32 LearnBits = 0,
                    u32 Target = 0, u32 RoleRequest = 0) {}
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
