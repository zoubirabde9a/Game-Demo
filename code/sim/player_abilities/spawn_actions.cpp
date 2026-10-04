/* Spawn actions: the attacks that put something into the world (a sword
   swing, a fireball), as a table. Each row says how long the player is
   rooted, how often it may go (both from player_stats.cpp), how long a
   press waits in the action queue for its turn, and which function makes
   the thing. Starting one
   is the same for every row (StartSpawnAction); only the spawn differs.
   A new one is a row here, a name in player_action, its spawn function,
   a button in player.h and a key in client/action_keys.cpp. A combo
   that ends in one of these (combos.cpp) spawns instead of its row. */

typedef void player_spawn_function(app_state *AppState, world *World,
                                   memory_arena *Arena, world_entity *Player,
                                   v2 Dir, player_tick *Tick);

struct player_spawn_action
{
    u32 Button;
    // NOTE(zoubir): the state and animation while it plays
    entity_state State;
    animation_type Animation;
    // NOTE(zoubir): seconds the player cannot walk from the start; the
    // rest of the animation can be walked out of
    float Lock;
    // NOTE(zoubir): seconds before the next one may start
    float Interval;
    // NOTE(zoubir): how long a press waits in the queue for its turn
    float Linger;
    // NOTE(zoubir): wait for the last one's animation to end, not only
    // the interval
    bool32 OnePerAnimation;
    asset_type_id Sound;
    player_spawn_function *Spawn;
    // NOTE(zoubir): what the combo trail calls it (combos.cpp)
    combo_move Move;
};

// NOTE(zoubir): a queued press waits longer than a whole swing (6 frames
// of 0.03 s, animations.cpp), so a click anywhere in a swing chains the
// next one; at 0.15 s clicks in a swing's first moments were dropped
#define PLAYER_ACTION_LINGER 0.25f
static_assert(PlayerAction_Count <= PLAYER_ACTION_SLOTS, "one cooldown each");

global_variable player_spawn_action PlayerSpawnActions[PlayerAction_Count] =
{
    // NOTE(zoubir): Sword (right click). The interval is one swing
    // animation: in the air the jump state replaced the swing state, so
    // without it every click swung at once
    {PlayerButton_Attack, EntityState_Attacking, AnimationType_Attack,
     PlayerStats.SwordLock, PlayerStats.SwordInterval, PLAYER_ACTION_LINGER,
     true, AssetType_Dash, SpawnSwordSwing, ComboMove_Attack},
    // NOTE(zoubir): Fireball (left click), about three a second; with no
    // limit each click restarted the cast animation. A press waits out
    // the whole interval
    {PlayerButton_Cast, EntityState_Casting, AnimationType_Cast,
     PlayerStats.FireballLock, PlayerStats.FireballInterval,
     PlayerStats.FireballInterval, false, AssetType_FireCast,
     SpawnFireBall, ComboMove_Cast},
};

inline bool32
CanStartSpawnAction(world_entity *Player, u32 Index)
{
    player_spawn_action *Action = &PlayerSpawnActions[Index];
    bool32 Result = Player->ActionCooldowns[Index] <= 0.f &&
        !(Action->OnePerAnimation && Player->State == Action->State);
    return Result;
}

internal void
StartSpawnAction(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Player, u32 Index, v2 Dir, player_tick *Tick)
{
    player_spawn_action *Action = &PlayerSpawnActions[Index];
    Player->State = Action->State;
    Player->ActionLock = Action->Lock;
    Player->ActionCooldowns[Index] = Action->Interval;
    Player->CastingDirection = Dir;
    Player->AnimationState.SlotIndex = 0;
    *Tick->AnimationType = Action->Animation;
    // NOTE(zoubir): a combo that ends here spawns its own thing instead
    if (!RunPlayerCombo(AppState, World, Arena, Player, Action->Move, Tick))
    {
        Action->Spawn(AppState, World, Arena, Player, Dir, Tick);
    }
    EmitSound(&AppState->Events, Action->Sound, Player->Position);
}
