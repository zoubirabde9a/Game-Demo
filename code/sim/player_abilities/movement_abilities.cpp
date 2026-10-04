/* Movement abilities: the keys that move the player at once (dash,
   blink, slam), as a table. A monster kill takes some or all of each
   one's cooldown off (KillRefund): dash and slam are ready again at
   once, blink half way. A dash into a unit also hits it (DashStrike).
   Using one is the same for every row: it needs its key and its
   cooldown (or a press just before it ends, CanUseEarly), cuts a swing,
   a cast and an area cast short, lights
   DashFlash for its row's FlashSeconds (while it lasts the player cannot
   be hurt or shoved, IsDodging in entity.cpp, and clients draw the
   streak from it), plays the dash sound and then any combo it finishes
   (combos.cpp). Only how the body moves differs, one function per row.
   A new one is a row here, a name in player_movement, its motion, a
   button in player.h and a key in client/action_keys.cpp. */

// NOTE(zoubir): moves Player; Power is its row's. Returns false when it
// did nothing (a slam on the ground), which keeps the cooldown.
typedef bool32 player_motion_function(app_state *AppState, world *World,
                                      memory_arena *Arena, world_entity *Player,
                                      player_input *Input, float DeltaTime,
                                      float Power);

enum player_movement
{
    PlayerMove_Dash,
    PlayerMove_Blink,
    PlayerMove_Slam,
    PlayerMove_Count
};

struct player_movement_ability
{
    u32 Button;
    float Cooldown;
    // NOTE(zoubir): share of the cooldown left that a monster kill takes
    // off, so aggression keeps the player moving
    float KillRefund;
    float Power;
    // NOTE(zoubir): seconds of DashFlash it lights: the streak, the
    // afterimages and the dodge. The slam's dive is straight down, where a
    // streak piles up on one spot, so it lights none
    float FlashSeconds;
    player_motion_function *Motion;
    // NOTE(zoubir): what the combo trail calls it (combos.cpp)
    combo_move Move;
};

#define PLAYER_DASH_FLASH_SECONDS 0.15f
// NOTE(zoubir): the vertical speed an air dash leaves; a little up, so the
// dash holds its height for a moment against gravity
#define PLAYER_AIR_DASH_LIFT 120.f

// NOTE(zoubir): a burst of Power speed the way the keys point, or toward
// the aim when standing; ground drag eases it back to a walk in about a
// quarter second, about 130 units travelled. In the air it also stops the
// fall, so a jump and a dash carry the player level across a gap
internal bool32
DashMotion(app_state *AppState, world *World, memory_arena *Arena,
           world_entity *Player, player_input *Input, float DeltaTime,
           float Power)
{
    v2 Dir = GetPlayerAim(Player);
    float HeldSquared = LengthSq(Input->Move);
    if (HeldSquared > 0.0001f)
    {
        Dir = Input->Move * (1.f / SquareRoot(HeldSquared));
    }
    Player->Velocity.XY = Power * Dir;
    if (!IsOnGround(Player))
    {
        Player->Velocity.Z = Maximum(Player->Velocity.Z, PLAYER_AIR_DASH_LIFT);
    }
    return true;
}

// NOTE(zoubir): a jump through space to the cursor, at most Power away.
// It passes through monsters and players (Phasing) but is swept like any
// move against the rest, so it stops at the first wall, tree or rock on
// the way (sliding along it) rather than landing inside, and keeps the
// player's speed. Landing inside a unit is undone by separation at the
// end of the tick
internal bool32
BlinkMotion(app_state *AppState, world *World, memory_arena *Arena,
            world_entity *Player, player_input *Input, float DeltaTime,
            float Power)
{
    // NOTE(zoubir): never cursor input yet: the full reach
    float Reach = Player->AimReach > 0.f ? Player->AimReach : 1.f;
    float Distance = Reach * Power;
    v3 Velocity = Player->Velocity;
    Player->Velocity = V3(0.f, 0.f, 0.f);
    Player->Velocity.XY = (Distance / DeltaTime) * GetPlayerAim(Player);
    Player->Phasing = true;
    MoveEntity(Player, World, Arena, DeltaTime, AppState, V3(0.f, 0.f, 0.f),
               &Distance);
    Player->Phasing = false;
    Player->Velocity = Velocity;
    return true;
}

// NOTE(zoubir): from the air only, straight down at Power, most of the
// sideways speed dropped so it lands where it started; the Slam area row
// fires on landing (FireAreaOnLanding)
internal bool32
SlamMotion(app_state *AppState, world *World, memory_arena *Arena,
           world_entity *Player, player_input *Input, float DeltaTime,
           float Power)
{
    if (IsOnGround(Player))
    {
        return false;
    }
    Player->Velocity.XY *= 0.3f;
    Player->Velocity.Z = -Power;
    Player->PendingLandArea = PlayerArea_Slam + 1;
    return true;
}

global_variable player_movement_ability PlayerMovements[PlayerMove_Count] =
{
    // NOTE(zoubir): Dash (Alt), leaving at 1440; Blink (F), as far as the
    // cursor can reach. Their numbers are in player_stats.cpp
    {PlayerButton_Dash, PlayerStats.DashCooldown, 1.f, PlayerStats.DashSpeed,
     PLAYER_DASH_FLASH_SECONDS, DashMotion, ComboMove_Dash},
    {PlayerButton_Blink, PlayerStats.BlinkCooldown, 0.5f, PlayerStats.BlinkReach,
     PLAYER_DASH_FLASH_SECONDS, BlinkMotion, ComboMove_Blink},
    // NOTE(zoubir): Slam (C), in the air. Straight down, so it does not
    // scale with PLAYER_MOVE_SCALE: from the top of a double jump it
    // already lands in under a tenth of a second
    {PlayerButton_Slam, 2.f, 1.f, 900.f, 0.f, SlamMotion, ComboMove_Slam},
};
static_assert(PlayerMove_Count <= PLAYER_MOVEMENT_SLOTS, "one cooldown each");

// NOTE(zoubir): what a dash does to a unit it runs into: a light hit, a
// shove on along the dash and a moment's stun, so dashing through a
// crowd scatters it. Once per unit: the dash's speed into it is gone
// after the first contact
global_variable hit DashStrikeHit = {10.f, 250.f, 0.f, 120.f, 0.35f, SimBurst_Impact};

internal void
DashStrike(app_state *AppState, world *World, world_entity *Player,
           world_entity *Target, v2 Away)
{
    ApplyHit(AppState, World, Target, &DashStrikeHit, Away,
             Player, Player->PlayerIndex);
}

internal void
RefundOnKill(world_entity *Player)
{
    for(u32 Index = 0; Index < PlayerMove_Count; Index++)
    {
        Player->MovementCooldowns[Index] *= 1.f - PlayerMovements[Index].KillRefund;
    }
}

// NOTE(zoubir): every movement ability's button; they move only the
// player, so a client predicts them (client/prediction.cpp)
internal u32
PlayerMovementButtons()
{
    u32 Result = 0;
    for(u32 Index = 0; Index < PlayerMove_Count; Index++)
    {
        Result |= PlayerMovements[Index].Button;
    }
    return Result;
}

internal void
UseMovementAbilities(app_state *AppState, world *World, memory_arena *Arena,
                     world_entity *Player, player_input *Input,
                     float DeltaTime, player_tick *Tick)
{
    Player->DashFlash = Maximum(0.f, Player->DashFlash - DeltaTime);
    for(u32 Index = 0; Index < PlayerMove_Count; Index++)
    {
        player_movement_ability *Ability = &PlayerMovements[Index];
        float *Cooldown = &Player->MovementCooldowns[Index];
        *Cooldown = Maximum(0.f, *Cooldown - DeltaTime);
        if (!WasPressed(Input, Ability->Button) || !CanUseEarly(*Cooldown) ||
            DeltaTime <= 0.f)
        {
            continue;
        }
        if (!Ability->Motion(AppState, World, Arena, Player, Input, DeltaTime,
                             Ability->Power))
        {
            continue;
        }
        // NOTE(zoubir): a blink into something deadly removes the player
        if (!Player->IsPresent)
        {
            return;
        }
        // NOTE(zoubir): it cuts a swing or cast short too: the player runs
        // out of it at full speed, and the next swing need not wait for the
        // old one's animation
        Player->ActionLock = 0.f;
        CancelAreaCast(Player);
        if (Player->State == EntityState_Attacking ||
            Player->State == EntityState_Casting)
        {
            Player->State = EntityState_Standing;
        }
        *Cooldown += Ability->Cooldown;
        Player->DashFlash = Maximum(Player->DashFlash, Ability->FlashSeconds);
        EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
        RunPlayerCombo(AppState, World, Arena, Player, Ability->Move, Tick);
    }
}
