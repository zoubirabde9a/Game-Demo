/* Standard cast online (client/cast_targeting.cpp): a real client and
   server, with the frame's input built the way app.cpp builds it. V aims
   the kunai and the server sees no press; the left click on a unit
   throws it there. Uses the round map test's server and client
   (round_map_tests.cpp). Included by server_tests.cpp, which calls
   TestStandardCastOnline. */

global_variable cast_targeting OnlineCastTargeting;
global_variable cursor_targeting OnlineCursorTargeting;

// NOTE(zoubir): one client frame as app.cpp runs it, then a server tick;
// the cursor on the world point Cursor, the camera at the origin
internal void
RunCastOnlineFrame(round_map_test *Test, app_input *Input, v2 Cursor)
{
    app_state *Client = Test->Client;
    Client->WorldZoom = 1.f;
    Client->CameraOffset = {};
    Input->MouseX = (i32)Cursor.X;
    Input->MouseY = (i32)Cursor.Y;
    player_input *Local = &Client->Players[Client->LocalPlayerIndex].Input;
    *Local = ReadKeyboardPlayerInput(Input, Client);
    app_input ServerInput = InputForServer(Input, Client);
    UpdateOnlineSession(Test->Online, &ServerInput, false, Local->Aim, 0, Local->Target);
    RunWorldTick(Client, &Test->Arena, Input->DeltaTime);
    ServerTick(&Test->Server);
    action_key Keys[ACTION_KEY_COUNT];
    GetActionKeys(Input, Keys);
    for(u32 Index = 0; Index < ACTION_KEY_COUNT; Index++)
    {
        Keys[Index].Key->Pressed = false;
    }
}

internal world_entity *
FindServerKunai(app_state *Game)
{
    world *World = &Game->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        if (World->Entities[Index].IsPresent && World->Entities[Index].Type == EntityType_Kunai)
        {
            return &World->Entities[Index];
        }
    }
    return 0;
}

internal void
TestStandardCastOnline()
{
    static round_map_test Test;
    StartRoundMapTest(&Test, "Aimer");
    OnlineCastTargeting = {};
    OnlineCastTargeting.Mode = CastMode_Standard;
    Test.Client->CastTargeting = &OnlineCastTargeting;
    OnlineCursorTargeting = {};
    Test.Client->CursorTargeting = &OnlineCursorTargeting;
    RunRoundMapTest(&Test, 3 * SERVER_TICK_RATE);
    Check(Test.Online->Replicas.Active);

    app_state *Game = Test.Server.Game.AppState;
    u32 SlotIndex = Test.Online->Client.PlayerIndex;
    world_entity *Own = Game->Players[SlotIndex].Entity;
    Check(Own != 0);
    if (!Own)
    {
        StopRoundMapTest(&Test);
        return;
    }

    app_input Input = {};
    Input.DeltaTime = 1.0f / SERVER_TICK_RATE;
    world_entity *Local = GetLocalPlayer(Test.Client);
    Check(Local != 0);
    v2 Off = Local ? Local->Position.XY + V2(0.f, 300.f) : V2(0.f, 0.f);

    Input.ButtonV.Pressed = Input.ButtonV.EndedDown = true;
    RunCastOnlineFrame(&Test, &Input, Off);
    Check(OnlineCastTargeting.Aiming == PlayerButton_Kunai);
    Input.ButtonV.EndedDown = false;
    for(u32 Frame = 0; Frame < SERVER_TICK_RATE / 2; Frame++)
    {
        RunCastOnlineFrame(&Test, &Input, Off);
    }
    Check(OnlineCastTargeting.Aiming == PlayerButton_Kunai);
    Check(!FindServerKunai(Game));
    Check(Own->ActionCooldowns[PlayerAction_Kunai] == 0.f);
    StopRoundMapTest(&Test);
}
