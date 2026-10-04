#if !defined(SIM_PLAYER_H)
/* Players: everything the simulation knows about one participant. A
   player_slot owns a player entity plus the input that drives it. The
   local keyboard fills one slot's input; the network will fill the rest,
   so the simulation never reads the keyboard directly. */

#define MAX_PLAYERS 8
// NOTE(zoubir): how long a dead player stays out before coming back
#define PLAYER_RESPAWN_SECONDS 3.f
// NOTE(zoubir): bit the server sets in a player's snapshot Ability (the
// replica's AbilityIndex) while its dash streak shows
#define PLAYER_FLASH_DASH 1
// NOTE(zoubir): the cursor distance player_input.Aim can tell apart; the
// blink range
#define PLAYER_AIM_REACH 160.f

// NOTE(zoubir): buttons pressed this tick (edge, not held). Same order as
// the network's action buttons (NetButton_Jump onward, net/protocol.h),
// so one shift turns either into the other (PlayerButtonsFromNet)
enum player_button
{
    PlayerButton_Jump = 1 << 0,
    PlayerButton_Dash = 1 << 1,
    PlayerButton_Cast = 1 << 2,
    PlayerButton_Attack = 1 << 3,
    PlayerButton_Shockwave = 1 << 4,
    PlayerButton_Blink = 1 << 5,
    PlayerButton_Push = 1 << 6,
    PlayerButton_Launch = 1 << 7,
    PlayerButton_Slam = 1 << 8,
};
#define PLAYER_BUTTON_NET_SHIFT 4

struct player_input
{
    // NOTE(zoubir): -1, 0 or 1 per axis, Y down
    v2 Move;
    // NOTE(zoubir): from the player toward the cursor, scaled so length 1
    // is PLAYER_AIM_REACH away or farther (blink lands at the cursor when
    // it is closer); zero keeps the last aim
    v2 Aim;
    u32 Pressed;
};

// NOTE(zoubir): the rows of PlayerSpawnActions
// (sim/player_abilities/spawn_actions.cpp)
enum player_action
{
    PlayerAction_Sword,
    PlayerAction_FireBall,
    PlayerAction_Count
};

// NOTE(zoubir): an action that waits up to TimeRemaining for the current
// animation to finish, so presses during an attack are not lost
struct player_delayed_input
{
    player_action Type;
    float TimeRemaining;
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
    // NOTE(zoubir): chosen by the player, may be empty ("Player N" then)
    char Name[16];
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
AddPlayerDelayedInput(player_slot *Slot, player_action Type,
                      float TimeRemaining)
{
    if (Slot->DelayedInputCount < ArrayCount(Slot->DelayedInput))
    {
        player_delayed_input *NewInput =
            &Slot->DelayedInput[Slot->DelayedInputCount++];
        NewInput->Type = Type;
        NewInput->TimeRemaining = TimeRemaining;
    }
}

#define SIM_PLAYER_H
#endif
