/* Stand-in for the real simulation (see game_api.h). Each player is a
   point that moves at PLACEHOLDER_SPEED while a direction button is held. */

#define PLACEHOLDER_SPEED 200.0f

struct placeholder_player
{
    bool32 Present;
    u16 Buttons;
    float X, Y;
    float VelX, VelY;
};

struct server_game
{
    placeholder_player Players[NET_MAX_CLIENTS];
};

internal void
GameInit(server_game *Game)
{
    *Game = {};
}

internal void
GamePlayerJoined(server_game *Game, u32 Slot)
{
    placeholder_player *Player = &Game->Players[Slot];
    *Player = {};
    Player->Present = true;
    Player->X = 100.0f + 50.0f * Slot;
    Player->Y = 100.0f;
}

internal void
GamePlayerLeft(server_game *Game, u32 Slot)
{
    Game->Players[Slot] = {};
}

internal void
GameApplyInput(server_game *Game, u32 Slot, net_input *Input)
{
    Game->Players[Slot].Buttons = Input->Buttons;
}

internal void
GameTick(server_game *Game, float Dt)
{
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        placeholder_player *Player = &Game->Players[Index];
        if (!Player->Present) continue;
        u16 B = Player->Buttons;
        Player->VelX = PLACEHOLDER_SPEED * (((B & NetButton_Right) ? 1 : 0) - ((B & NetButton_Left) ? 1 : 0));
        Player->VelY = PLACEHOLDER_SPEED * (((B & NetButton_Down) ? 1 : 0) - ((B & NetButton_Up) ? 1 : 0));
        Player->X += Player->VelX * Dt;
        Player->Y += Player->VelY * Dt;
    }
}

internal void
GameWriteSnapshot(server_game *Game, u32 ViewerSlot, net_snapshot *Out)
{
    Out->Count = 0;
    for (u32 Index = 0; Index < NET_MAX_CLIENTS; ++Index)
    {
        placeholder_player *Player = &Game->Players[Index];
        if (!Player->Present) continue;
        net_entity_state *E = &Out->Entities[Out->Count++];
        *E = {};
        E->Id = (u16)Index;
        E->Health = 100;
        E->X = Player->X;
        E->Y = Player->Y;
        E->VelX = Player->VelX;
        E->VelY = Player->VelY;
    }
}
