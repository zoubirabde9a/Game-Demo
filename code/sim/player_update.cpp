/* Player update: what one player does in a tick, driven by its slot's
   player_input (never the keyboard). UpdatePlayer reads as its steps:

   1. QueuePlayerActions: held directions set this tick's move; pressed
      buttons become queued actions (sword, fireball). Each waits a
      moment for the current animation or the fireball interval, so a
      press during a swing is not lost. Sword and fireball take the aim
      (toward the cursor) at the moment they start, not of the press.
   2. FinishPlayerActions: a swing or cast whose animation ended frees
      the player.
   3. RunPlayerActionQueue: the first action that can run now does
      (StartSpawnAction, player_abilities/spawn_actions.cpp).
   4. UpdatePlayerMoveState: moving, or stopping when the keys let go. A
      swing or cast roots the player only for its short ActionLock, a
      shove's Stagger for a moment (it slides). Then a
      swing's animation finishes while the player walks slower, and a
      cast's is cut by walking (the fireball has already left).
   5. UsePlayerAbilities: jump, the area abilities (shockwave, push,
      launch), the movement abilities (dash, blink, slam) and the
      rewinds, each when its key is pressed and its cooldown allows, or
      is about to (CanUseEarly, player_stats.cpp); a movement ability
      cuts a swing short. Then the cast under way (UpdatePlayerCast): the
      spells with a wind-up go off when it ends (sim/player_casts.cpp).
   6. PickPlayerAnimation: which animation to play; the body faces the
      aim, not the way it walks.
   7. MovePlayer: the walk toward the keys (quick to start, stop and
      turn, player_stats.cpp), the drag on a dash and gravity into
      MoveEntity.

   Steps 1-3 are in player_update/actions.cpp, 4 and 7 in
   player_update/movement.cpp, the cast in player_update/casts.cpp, 6 in
   player_update/animation.cpp. The
   abilities live in player_abilities/, as three tables: area abilities
   (area_abilities.cpp), spawn actions such as the sword and fireball
   (spawn_actions.cpp) and movement abilities (movement_abilities.cpp).
   A new ability is usually a row in one of them, a button in player.h
   and a key in client/action_keys.cpp. Jump is its own file. Every move
   also goes through RunPlayerCombo (combos.cpp), which turns some
   orders of moves into combos: dash then attack is a lunge. */

// NOTE(zoubir): what one tick of the player decides, handed between steps
struct player_tick
{
    bool32 Move;
    bool32 Jumping;
    // NOTE(zoubir): share of the run speed the keys walk at this tick; a
    // swing finishing or an area cast lowers it
    float Acceleration;
    // NOTE(zoubir): a cast holds the player's height (the slam's wind-up)
    bool32 Hover;
    v3 DDPlayer;
    float *AnimationSpeedRate;
    animation_type *AnimationType;
    animation_direction *AnimationDirection;
};

// NOTE(zoubir): the sprite row closest to Dir, for any angle; X wins a
// perfect diagonal
inline animation_direction
DominantFacing(v2 Dir)
{
    animation_direction Result;
    if (Absolute(Dir.X) >= Absolute(Dir.Y))
    {
        Result = Dir.X < 0.f ? AnimationDirection_Left : AnimationDirection_Right;
    }
    else
    {
        Result = Dir.Y < 0.f ? AnimationDirection_Up : AnimationDirection_Down;
    }
    return Result;
}

// NOTE(zoubir): every move calls it as it starts (combos.cpp, below)
internal bool32 RunPlayerCombo(app_state *AppState, world *World,
                               memory_arena *Arena, world_entity *Player,
                               combo_move Move, player_tick *Tick);

#include "player_abilities/sword.cpp"
#include "player_abilities/fireball.cpp"
#include "player_abilities/kunai.cpp"
#include "player_abilities/spawn_actions.cpp"
#include "player_abilities/jump.cpp"
#include "player_abilities/area_abilities.cpp"
#include "player_abilities/blink_landing.cpp"
#include "player_abilities/movement_abilities.cpp"
#include "player_abilities/combos.cpp"

#include "player_update/actions.cpp"
#include "player_update/movement.cpp"
#include "player_update/casts.cpp"
#include "player_update/animation.cpp"

internal void
UsePlayerAbilities(app_state *AppState, world *World, memory_arena *Arena,
                   player_slot *Slot, float DeltaTime, player_tick *Tick)
{
    world_entity *Player = Slot->Entity;
    player_input *Input = &Slot->Input;
    UseJump(AppState, World, Arena, Player, Input, DeltaTime, Tick);
    UseAreaAbilities(AppState, Player, Input, DeltaTime);
    UseMovementAbilities(AppState, World, Arena, Player, Input, DeltaTime, Tick);
    UseRewindAbilities(AppState, Player, Input, DeltaTime);
    UpdatePlayerCast(AppState, World, Arena, Player, Input, DeltaTime, Tick);
}

// NOTE(zoubir): in sim/dungeon/role_abilities.cpp: a tank's and a
// healer's keys cast their role's spells in a dungeon run
internal void UseRoleAbilities(app_state *AppState, world *World, memory_arena *Arena,
                               player_slot *Slot, float DeltaTime);
internal u32 RunAllowedButtons(app_state *AppState, player_slot *Slot, u32 Allowed);
internal void FinishRoleCast(app_state *AppState, world_entity *Player, player_spell Spell);

internal void
UpdatePlayer(player_slot *Slot, world *World,
             memory_arena *Arena,
             float DeltaTime, app_state *AppState,
             float *AnimationSpeedRate,
             animation_type *AnimationType,
             animation_direction *AnimationDirection)
{
    world_entity *Player = Slot->Entity;
    // NOTE(zoubir): abilities the match leaves out (GameRules) and the
    // talent tree has not unlocked do nothing, nor does any but jump
    // during the break between rounds. A dungeon run has its own set: the
    // shared abilities and the class's spells (sim/dungeon/role_abilities.cpp)
    u32 Allowed = RunAllowedButtons(AppState, Slot, PlayerAllowedButtons(Slot)) &
        RoundBreakButtons(AppState);
    Slot->Input.Pressed &= Allowed;
    Slot->Input.ServerPressed &= Allowed;
    // NOTE(zoubir): a rooted player cannot walk; a stunned or falling one
    // can neither move nor act
    if (IsRooted(Player))
    {
        Slot->Input.Move = V2(0.f, 0.f);
    }
    if (IsDisabled(Player))
    {
        Slot->Input.Move = V2(0.f, 0.f);
        Slot->Input.Pressed = 0;
        // NOTE(zoubir): a cast winding up and clicks waiting in the queue
        // are lost too; they used to go off while stunned
        CancelPlayerCast(Player);
        Slot->DelayedInputCount = 0;
    }
    // NOTE(zoubir): a dungeon role's keys, before the game's abilities read
    // them (sim/dungeon/role_abilities.cpp)
    UseRoleAbilities(AppState, World, Arena, Slot, DeltaTime);
    *AnimationType = AnimationType_Stand;
    *AnimationDirection = Player->AnimationState.LastAnimationDirection;
    *AnimationSpeedRate = 1.f;

    Player->ActionLock = Maximum(0.f, Player->ActionLock - DeltaTime);
    Player->Stagger = Maximum(0.f, Player->Stagger - DeltaTime);
    Player->ComboTimer = Maximum(0.f, Player->ComboTimer - DeltaTime);
    Player->SpawnShield = Maximum(0.f, Player->SpawnShield - DeltaTime);
    AgeComboTrail(Player, DeltaTime);
    for(u32 Index = 0; Index < PlayerAction_Count; Index++)
    {
        Player->ActionCooldowns[Index] =
            Maximum(0.f, Player->ActionCooldowns[Index] - DeltaTime);
    }
    player_tick Tick = {};
    Tick.Acceleration = 1.f;
    Tick.AnimationSpeedRate = AnimationSpeedRate;
    Tick.AnimationType = AnimationType;
    Tick.AnimationDirection = AnimationDirection;

    QueuePlayerActions(Slot, &Tick);
    FinishPlayerActions(Player, &Tick);
    RunPlayerActionQueue(AppState, World, Arena, Slot, DeltaTime, &Tick);
    UpdatePlayerMoveState(AppState, Player, &Tick);
    UsePlayerAbilities(AppState, World, Arena, Slot, DeltaTime, &Tick);
    PickPlayerAnimation(Player, &Tick);
    MovePlayer(AppState, World, Arena, Player, DeltaTime, &Tick);
}
