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
