/* Jump (Space): an upward kick from the ground, and one more in the air
   (PLAYER_JUMP_COUNT). The first clears a boulder and ground hazards
   (IsClearOfGround, entity.cpp); a second near the top of the first
   clears a dead tree. Walls are taller than both (WallCollision,
   collision_rules.cpp), so nobody leaves the map. Landing on top of
   something (a boulder, a monster's head) counts as ground. A press with
   no jump left is kept for PLAYER_JUMP_BUFFER and jumps on landing. Gravity is
   here because the jump's height and timing are tuned with it. */

// NOTE(zoubir): peak 36 units after 0.21 s, 0.43 s in the air; it was
// 230 against 1000 (26 units, too low for a boulder, and floaty)
#define PLAYER_GRAVITY 1600.f
#define PLAYER_JUMP_SPEED 340.f
// NOTE(zoubir): replaces the vertical speed, so a late second jump still
// rises a full 32 units instead of only slowing the fall
#define PLAYER_AIR_JUMP_SPEED 320.f
#define PLAYER_JUMP_COUNT 2
// NOTE(zoubir): a press with no jump left counts if the player lands
// within this long after it, so a jump pressed a moment early still goes
#define PLAYER_JUMP_BUFFER 0.12f
// NOTE(zoubir): how close above what is under it a player counts as
// standing; resting on a box top leaves a hair of space (TestWall)
#define PLAYER_GROUND_SNAP 0.5f

inline bool32
IsOnGround(world_entity *Player)
{
    bool32 Result = Player->Velocity.Z <= 0.f &&
        Player->Position.Z <= Player->GroundZ + PLAYER_GROUND_SNAP;
    return Result;
}

// NOTE(zoubir): auto-vault: a player on the ground pushing head-on into
// something low (a boulder) for PLAYER_VAULT_PUSH jumps over it on its
// own, as if it had pressed jump. Only solids whose top is within
// PLAYER_VAULT_HEIGHT of the feet count; walls and trees stay walls
#define PLAYER_VAULT_PUSH 0.12f
#define PLAYER_VAULT_HEIGHT 32.f
// NOTE(zoubir): how far ahead the vault looks, and the speed along the
// keys below which the player counts as stopped by what is ahead
#define PLAYER_VAULT_PROBE 4.f
#define PLAYER_VAULT_STALL_SPEED 20.f

// NOTE(zoubir): the top of the tallest solid just ahead of Player along
// Dir (a unit vector), or 0 when nothing solid is there
internal float
SolidTopAhead(app_state *AppState, world *World, world_entity *Player, v2 Dir)
{
    world_entity Probe = *Player;
    Probe.Position.XY += PLAYER_VAULT_PROBE * Dir;
    entity_collision_volume *Total = &Probe.Collision->TotalVolume;
    rectangle3 Box = RectCenterHalfDims(Probe.Position + Total->Offset,
                                        Total->HalfDims);
    world_entity *Nearby[64];
    u32 Count = GatherEntitiesInBox(World, Box, Nearby, ArrayCount(Nearby));
    float Result = 0.f;
    for(u32 Index = 0; Index < Count; Index++)
    {
        world_entity *Other = Nearby[Index];
        bool32 Solid = Other->Type == EntityType_StaticObject ||
            Other->Type == EntityType_Tiled;
        if (Other->IsPresent && Solid && Other->Collision &&
            CanCollide(AppState, EntityType_Player, Other->Type) &&
            EntityOverlap(&Probe, Other))
        {
            entity_collision_volume *Theirs = &Other->Collision->TotalVolume;
            Result = Maximum(Result, Other->Position.Z + Theirs->Offset.Z +
                             Theirs->HalfDims.Z);
        }
    }
    return Result;
}

// NOTE(zoubir): after the player's move; queues the vault's jump when the
// player has been held up by something low enough for long enough
internal void
UpdateVault(app_state *AppState, world *World, world_entity *Player,
            player_tick *Tick, float DeltaTime)
{
    v2 Dir = Player->Direction;
    float DirLength = Length(Dir);
    bool32 Stalled = Tick->Move && IsOnGround(Player) && DirLength > 0.f &&
        DotProduct(Player->Velocity.XY, Dir) < PLAYER_VAULT_STALL_SPEED * DirLength;
    float Top = Stalled ?
        SolidTopAhead(AppState, World, Player, Dir * (1.f / DirLength)) : 0.f;
    float Height = Top - Player->Position.Z;
    if (Height > 1.f && Height <= PLAYER_VAULT_HEIGHT)
    {
        Player->VaultPush += DeltaTime;
        if (Player->VaultPush >= PLAYER_VAULT_PUSH)
        {
            Player->VaultPush = 0.f;
            Player->JumpBuffer = PLAYER_JUMP_BUFFER;
        }
    }
    else
    {
        Player->VaultPush = 0.f;
    }
}

internal void
UseJump(app_state *AppState, world_entity *Player, player_input *Input,
        float DeltaTime, player_tick *Tick)
{
    Player->JumpBuffer = Maximum(0.f, Player->JumpBuffer - DeltaTime);
    if (WasPressed(Input, PlayerButton_Jump))
    {
        Player->JumpBuffer = PLAYER_JUMP_BUFFER;
    }
    if (!Tick->Jumping)
    {
        Player->JumpsUsed = 0;
    }
    else if (Player->JumpsUsed == 0)
    {
        // NOTE(zoubir): walking off a ledge spends the ground jump
        Player->JumpsUsed = 1;
    }
    if (Player->JumpBuffer > 0.f && Player->JumpsUsed < PLAYER_JUMP_COUNT)
    {
        Player->JumpBuffer = 0.f;
        Player->State = EntityState_Jumping;
        Player->Velocity.Z = Tick->Jumping ? PLAYER_AIR_JUMP_SPEED :
            PLAYER_JUMP_SPEED;
        if (Tick->Jumping)
        {
            EmitBurst(&AppState->Events, SimBurst_AirJump,
                      (u8)Player->PlayerIndex, Player->Position);
        }
        Player->JumpsUsed++;
        Tick->Jumping = true;
        EmitSound(&AppState->Events, AssetType_ZoubirAudio, Player->Position);
    }
}
