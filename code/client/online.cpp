/* Online session: when a server address is configured, the client
   connects to the dedicated server, sends the keys held each frame, and
   keeps the newest snapshot in Online->Client.Snapshot. With no address it
   stays offline and the game runs its own simulation, as before.

   The address comes from the GAME_SERVER environment variable, or else
   the first line of server.txt in the folder the game runs from, written
   as "a.b.c.d:port". The name other players see comes from GAME_NAME, or
   else the second line of server.txt; empty shows as "Player N". The
   browser build has no UDP and is always offline. */

#define ONLINE_ADDRESS_FILE "server.txt"
#define ONLINE_ADDRESS_ENV "GAME_SERVER"
#define ONLINE_NAME_ENV "GAME_NAME"

struct online_session
{
    bool32 Enabled;
    char AddressText[64];
    // NOTE(zoubir): local copies of the server's entities, client/replicas.cpp
    replica_table Replicas;
    // NOTE(zoubir): inputs the server has not applied yet, client/prediction.cpp
    prediction_history Prediction;
#if !COMPILER_EMSCRIPTEN
    net_client Client;
#endif
};

// NOTE(zoubir): copies the first line of Text, trimmed of spaces
internal void
CopyFirstLine(char *Out, u32 OutSize, char *Text)
{
    while (*Text == ' ' || *Text == '\t')
    {
        Text++;
    }
    u32 Length = 0;
    while (Text[Length] && Text[Length] != '\r' && Text[Length] != '\n' &&
           Length + 1 < OutSize)
    {
        Out[Length] = Text[Length];
        Length++;
    }
    while (Length > 0 && (Out[Length - 1] == ' ' || Out[Length - 1] == '\t'))
    {
        Length--;
    }
    Out[Length] = 0;
}

// NOTE(zoubir): environment first, then the file; false when neither is set.
// getenv and fopen are standard and portable; MSVC's "unsafe" warning on
// them does not apply to reading one short line.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996)
#endif
// NOTE(zoubir): the text after LineIndex line breaks, or "" past the end
internal char *
SkipLines(char *Text, u32 LineIndex)
{
    for(; LineIndex > 0 && *Text; Text++)
    {
        if (*Text == '\n')
        {
            LineIndex--;
        }
    }
    return Text;
}

// NOTE(zoubir): each setting from its environment variable, else from its
// line of server.txt. Returns false when there is no address at all.
internal bool32
ReadOnlineConfig(char *Address, u32 AddressSize, char *Name, u32 NameSize)
{
    char File[256] = {};
    FILE *Handle = fopen(ONLINE_ADDRESS_FILE, "rb");
    if (Handle)
    {
        fread(File, 1, sizeof(File) - 1, Handle);
        fclose(Handle);
    }

    char *FromEnv = getenv(ONLINE_ADDRESS_ENV);
    CopyFirstLine(Address, AddressSize,
                  (FromEnv && FromEnv[0]) ? FromEnv : File);
    FromEnv = getenv(ONLINE_NAME_ENV);
    CopyFirstLine(Name, NameSize,
                  (FromEnv && FromEnv[0]) ? FromEnv : SkipLines(File, 1));
    return Address[0] != 0;
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

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

internal online_session *
StartOnlineSession(memory_arena *Arena)
{
    online_session *Online = AllocateStruct(Arena, online_session);
    *Online = {};
    net_address Server;
    char Name[NET_NAME_SIZE];
    if (ReadOnlineConfig(Online->AddressText, sizeof(Online->AddressText),
                         Name, sizeof(Name)) &&
        NetParseAddress(Online->AddressText, &Server) &&
        NetSocketsStartup())
    {
        u32 Salt = (u32)time(0) ^ (u32)(size_t)Online;
        Online->Enabled = NetClientConnect(&Online->Client, Server, Salt,
                                           SimContentId(), Name);
    }
    return Online;
}

internal void
UpdateOnlineSession(online_session *Online, app_input *Input)
{
    if (Online && Online->Enabled)
    {
        u16 Buttons = NetButtonsFromKeyboard(Input);
        NetClientUpdate(&Online->Client, Input->DeltaTime, Buttons, 0.f, 0.f);
        if (Online->Client.State == NetClient_Connected)
        {
            RecordPredictedInput(&Online->Prediction, Online->Client.InputTick,
                                 Buttons, Input->DeltaTime);
        }
    }
}

// NOTE(zoubir): true while the server, not the local simulation, owns the
// world; drawing the snapshot comes in plan step 7b
inline bool32
IsOnline(online_session *Online)
{
    bool32 Result = Online && Online->Enabled &&
        Online->Client.State == NetClient_Connected;
    return Result;
}

internal void
GetOnlineStatusText(online_session *Online, char *Out, u32 OutSize)
{
    Out[0] = 0;
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
            snprintf(Out, OutSize, "Online at %s as Player %u",
                     Online->AddressText, Client->PlayerIndex + 1);
        } break;
        case NetClient_Disconnected:
        {
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
        bool32 NewSnapshot = !Online->Replicas.Active ||
            Snapshot->Tick != Online->Replicas.LastAppliedTick;
        SyncReplicas(AppState, Arena, &Online->Replicas, Snapshot, DeltaTime,
                     Online->Client.PlayerIndex);
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

internal void UpdateOnlineSession(online_session *Online, app_input *Input) {}
inline bool32 IsOnline(online_session *Online) { return false; }
internal void
GetOnlineStatusText(online_session *Online, char *Out, u32 OutSize)
{
    Out[0] = 0;
}

#endif
