#if !defined(SIM_PLAYER_H)
/* Players: everything the simulation knows about one participant. A
   player_slot owns a player entity plus the input that drives it. The
   local keyboard fills one slot's input; the network will fill the rest,
   so the simulation never reads the keyboard directly. */

#define MAX_PLAYERS 8
// NOTE(zoubir): how long a dead player stays out before coming back
#define PLAYER_RESPAWN_SECONDS 3.f

// NOTE(zoubir): buttons pressed this tick (edge, not held)
enum player_button
{
    PlayerButton_Attack = 1 << 0,
    PlayerButton_Cast = 1 << 1,
    PlayerButton_Jump = 1 << 2,
    PlayerButton_Dash = 1 << 3,
    PlayerButton_Shockwave = 1 << 4,
};

struct player_input
{
    // NOTE(zoubir): -1, 0 or 1 per axis, Y down
    v2 Move;
    u32 Pressed;
};

enum player_delayed_input_type
{
    PDI_Move,
    PDI_Attack,
    PDI_Cast,
    PDI_Count
};

// NOTE(zoubir): an action that waits up to TimeRemaining for the current
// animation to finish, so presses during an attack are not lost
struct player_delayed_input
{
    player_delayed_input_type Type;
    float TimeRemaining;
    v2 Dir;
};

struct player_slot
{
    bool32 Active;
    struct world_entity *Entity;
    player_input Input;
    v3 SpawnPosition;
    // NOTE(zoubir): Kills counts other players, MonsterKills counts monsters
    u32 Kills;
    u32 Deaths;
    u32 MonsterKills;
    // NOTE(zoubir): counts down while the player is dead (Hp <= 0)
    float RespawnTimer;

    player_delayed_input DelayedInput[32];
    u32 DelayedInputCount;
};

inline bool32
WasPressed(player_input *Input, u32 Button)
{
    bool32 Result = (Input->Pressed & Button) != 0;
    return Result;
}

inline void
AddPlayerDelayedInput(player_slot *Slot, player_delayed_input_type Type,
                      float TimeRemaining, v2 Dir)
{
    if (Slot->DelayedInputCount < ArrayCount(Slot->DelayedInput))
    {
        player_delayed_input *NewInput =
            &Slot->DelayedInput[Slot->DelayedInputCount++];
        NewInput->Type = Type;
        NewInput->TimeRemaining = TimeRemaining;
        NewInput->Dir = Dir;
    }
}

#define SIM_PLAYER_H
#endif
