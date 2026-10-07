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
// NOTE(zoubir): the same, while a respawned player is shielded
#define PLAYER_FLASH_SHIELD 2
#define PLAYER_SPAWN_SHIELD_SECONDS 1.5f
// NOTE(zoubir): how fast and far the player moves against the numbers the
// moves were first tuned at: run speed, dash, blink, long jump and the
// speed checks that follow them are written as old number times this. At
// 1.6 the player runs at 208 instead of 130 and every move goes 1.6 times
// as far in the same time; jump heights and monsters do not change. It was
// 2 (a run of 260), which played too fast
#define PLAYER_MOVE_SCALE 1.6f
// NOTE(zoubir): the cursor distance player_input.Aim can tell apart; the
// blink range. Kept at 320 when the scale went from 2 to 1.6: blink is
// instant, so its reach is not part of the pace
#define PLAYER_AIM_REACH 320.f

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
    // NOTE(zoubir): the three time rewinds (sim/time_rewind/)
    PlayerButton_RewindSelf = 1 << 9,
    PlayerButton_RewindBubble = 1 << 10,
    PlayerButton_RewindWorld = 1 << 11,
    // NOTE(zoubir): a moment of invulnerability (movement_abilities.cpp)
    PlayerButton_Shield = 1 << 12,
    // NOTE(zoubir): the two abilities only the talent tree gives
    // (area_abilities.cpp, progression/talents.cpp)
    PlayerButton_FrostNova = 1 << 13,
    PlayerButton_GravityWell = 1 << 14,
    // NOTE(zoubir): a homing blade (player_abilities/kunai.cpp)
    PlayerButton_Kunai = 1 << 15,
};
#define PLAYER_ALL_BUTTONS ((u32)(PlayerButton_Kunai << 1) - 1)
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
    // NOTE(zoubir): only while the client predicts its own player
    // (client/prediction.cpp): the presses it leaves to the server
    // (attacks, casts). They do nothing here but mark the combo trail, so
    // the client and the server agree on what came before a dash
    u32 ServerPressed;
    // NOTE(zoubir): a talent to spend a point on this tick, its talent_id
    // + 1, 0 for none (progression/talents.cpp). The server reads it from
    // spare bits of the held buttons (NET_LEARN_SHIFT, net/protocol.h)
    u32 Learn;
    // NOTE(zoubir): the unit the cursor is on (client/targeting.cpp): its
    // index in this world's entities + 1, 0 for none. Online the client
    // sends the server's index (net_input.Target), offline its own
    u32 Target;
    // NOTE(zoubir): a map asked for or a vote answered this tick
    // (map_vote_request, sim/map_vote.cpp), 0 for none. The server reads
    // it from spare bits of the held buttons (NET_VOTE_SHIFT)
    u32 Vote;
};

// NOTE(zoubir): the moves the combo trail records (player_fields.inc) and
// combos are made of (player_abilities/combos.cpp). Walking is not one
enum combo_move
{
    ComboMove_None,
    ComboMove_Jump,
    ComboMove_Dash,
    ComboMove_Blink,
    ComboMove_Slam,
    ComboMove_Attack,
    ComboMove_Cast,
    ComboMove_Count
};

// NOTE(zoubir): the rows of PlayerSpawnActions
// (sim/player_abilities/spawn_actions.cpp)
enum player_action
{
    PlayerAction_Sword,
    PlayerAction_FireBall,
    PlayerAction_Kunai,
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
    // NOTE(zoubir): set while the client steps its own slot to predict it
    // (client/prediction.cpp): what touches anyone else (a slam's hit)
    // waits for the server
    bool32 Predicted;
    // NOTE(zoubir): online, no input for this tick arrived from the
    // player's client (server/input_queue.cpp), so the player waits it out
    bool32 WaitingForInput;
    // NOTE(zoubir): experience, level and talents (sim/progression/)
#include "progression/progression_fields.inc"
#include "dungeon/dungeon_slot_fields.inc"
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
