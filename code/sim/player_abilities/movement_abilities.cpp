/* Movement abilities: the keys that move the player at once (dash,
   blink, slam), as a table. A monster kill takes some or all of each
   one's cooldown off (KillRefund): dash and slam are ready again at
   once, blink half way. Using one is the same for every row: it needs its
   key and its cooldown, cuts a swing's or cast's root and an area cast,
   lights DashFlash (while it lasts the player cannot be hurt or shoved,
   IsDodging in entity.cpp, and clients draw the streak from it) and plays
   the dash sound. Only how the body moves differs, one function per row.
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
    player_motion_function *Motion;
};

#define PLAYER_DASH_FLASH_SECONDS 0.15f

// NOTE(zoubir): a burst of Power speed the way the keys point, or toward
// the aim when standing; ground drag eases it back to a walk in about a
// quarter second, about 65 units travelled
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
    return true;
}

// NOTE(zoubir): a jump through space to the cursor, at most Power away.
// It is swept like any move, so it stops at the first wall or unit on
// the way (sliding along it) rather than landing inside, and keeps the
// player's speed
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
    MoveEntity(Player, World, Arena, DeltaTime, AppState, V3(0.f, 0.f, 0.f),
               &Distance);
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
    // NOTE(zoubir): Dash (Alt)
    {PlayerButton_Dash, 0.8f, 1.f, 650.f, DashMotion},
    // NOTE(zoubir): Blink (F), as far as the cursor can reach
    {PlayerButton_Blink, 3.f, 0.5f, PLAYER_AIM_REACH, BlinkMotion},
    // NOTE(zoubir): Slam (C), in the air
    {PlayerButton_Slam, 2.f, 1.f, 900.f, SlamMotion},
};
static_assert(PlayerMove_Count <= PLAYER_MOVEMENT_SLOTS, "one cooldown each");

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
                     float DeltaTime)
{
    Player->DashFlash = Maximum(0.f, Player->DashFlash - DeltaTime);
    for(u32 Index = 0; Index < PlayerMove_Count; Index++)
    {
        player_movement_ability *Ability = &PlayerMovements[Index];
        float *Cooldown = &Player->MovementCooldowns[Index];
        *Cooldown = Maximum(0.f, *Cooldown - DeltaTime);
        if (!WasPressed(Input, Ability->Button) || *Cooldown > 0.f ||
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
        Player->ActionLock = 0.f;
        CancelAreaCast(Player);
        *Cooldown = Ability->Cooldown;
        Player->DashFlash = PLAYER_DASH_FLASH_SECONDS;
        EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
    }
}
