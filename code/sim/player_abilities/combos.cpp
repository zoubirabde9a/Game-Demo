/* Combo moves: two or three of the player's moves in quick succession make
   a new one; dash then attack is a lunge, attack then dash a cutting dash
   (docs/combat-combos.md has the design and the full list).

   Each player keeps a trail of its last moves (ComboTrail,
   player_fields.inc). Every move calls RunPlayerCombo as it starts. That
   finds the row of PlayerCombos whose moves end in this one, with the
   earlier ones the newest of the trail, each within the row's Window of
   the next (the longest such row wins), fires it, and records the move.
   A combo is recorded as the move it ended on, so combos chain.

   A combo ending in an attack or a cast replaces the swing or fireball
   (StartSpawnAction spawns nothing of its own then); one ending in a
   dash, blink, slam or jump runs right after the move.

   A new combo is a row here and its function. */

typedef void player_combo_function(app_state *AppState, world *World,
                                   memory_arena *Arena, world_entity *Player,
                                   v2 Aim, player_tick *Tick);

#define COMBO_MAX_MOVES 3
static_assert(COMBO_MAX_MOVES <= PLAYER_COMBO_TRAIL + 1, "the trail holds the moves before the last");

struct player_combo
{
    // NOTE(zoubir): written over the player when it fires
    // (client/fx_bursts.cpp)
    char *Name;
    // NOTE(zoubir): in order; ComboMove_None after the last
    combo_move Moves[COMBO_MAX_MOVES];
    // NOTE(zoubir): the most seconds from the start of one move to the
    // start of the next
    float Window;
    sim_burst Burst;
    // NOTE(zoubir): touches only the player itself, so a client predicting
    // its own player runs it too (client/prediction.cpp); the rest wait
    // for the server
    bool32 MovesOnlyPlayer;
    player_combo_function *Fire;
};

// NOTE(zoubir): Lunge (dash, attack): a long thrust instead of the swing,
// with no root, so the dash carries the player through what it hits. It
// counts as the chain's first cut, so two more swings finish the chain
internal void
ComboLunge(app_state *AppState, world *World, memory_arena *Arena,
           world_entity *Player, v2 Aim, player_tick *Tick)
{
    SwingSwordCut(AppState, World, Arena, Player, Aim, SwordCut_Lunge);
    Player->ComboStep = SwordCut_First;
    Player->ComboTimer = SWORD_COMBO_WINDOW;
    Player->ActionLock = 0.f;
}

// NOTE(zoubir): Skewer (jump, dash, attack): the lunge from the air,
// throwing what it hits straight up
internal void
ComboSkewer(app_state *AppState, world *World, memory_arena *Arena,
            world_entity *Player, v2 Aim, player_tick *Tick)
{
    SwingSwordCut(AppState, World, Arena, Player, Aim, SwordCut_Skewer);
    Player->ComboStep = SwordCut_First;
    Player->ComboTimer = SWORD_COMBO_WINDOW;
    Player->ActionLock = 0.f;
}

// NOTE(zoubir): Ambush (blink, attack): the swing after a blink is the
// finisher at once, whatever the chain was
internal void
ComboAmbush(app_state *AppState, world *World, memory_arena *Arena,
            world_entity *Player, v2 Aim, player_tick *Tick)
{
    Tick->Acceleration *= 0.6f;
    Tick->DDPlayer.XY = Aim;
    SwingSwordCut(AppState, World, Arena, Player, Aim, SwordCut_Finisher);
    Player->ComboStep = SwordCut_Finisher;
    Player->ComboTimer = SWORD_COMBO_WINDOW;
}

// NOTE(zoubir): the cutting dash hits everything within RADIUS of the
// first LENGTH of the dash's path; a dash covers about 130
#define CUTTING_DASH_LENGTH (80.f * PLAYER_MOVE_SCALE)
#define CUTTING_DASH_RADIUS 40.f
global_variable hit CuttingDashHit = {18.f, 320.f, 0.f, 220.f, 0.3f, SimBurst_Impact};

// NOTE(zoubir): Cutting dash (attack, dash): the blade spins around the
// player along the dash, throwing what it passes out to the sides
internal void
ComboCuttingDash(app_state *AppState, world *World, memory_arena *Arena,
                 world_entity *Player, v2 Aim, player_tick *Tick)
{
    v2 Dir = Aim;
    float Speed = Length(Player->Velocity.XY);
    if (Speed > 1.f)
    {
        Dir = Player->Velocity.XY * (1.f / Speed);
    }
    v2 Start = Player->Position.XY;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Target = &World->Entities[EntityIndex];
        if (!Target->IsPresent || Target == Player || IsDeadPlayer(Target) ||
            (Target->Type != EntityType_Monster &&
             Target->Type != EntityType_Player))
        {
            continue;
        }
        v2 To = Target->Position.XY - Start;
        float Along = Minimum(CUTTING_DASH_LENGTH, Maximum(0.f, DotProduct(To, Dir)));
        v2 Side = To - Along * Dir;
        float Width = Target->Collision ?
            Target->Collision->TotalVolume.HalfDims.X : 0.f;
        float SideLength = Length(Side);
        if (SideLength - Width > CUTTING_DASH_RADIUS)
        {
            continue;
        }
        v2 Away = SideLength > 0.001f ? Side * (1.f / SideLength) :
            V2(-Dir.Y, Dir.X);
        ApplyHit(AppState, World, Target, &CuttingDashHit, Away,
                 Player, Player->PlayerIndex);
    }
}

// NOTE(zoubir): the angle between the flame fan's fireballs
#define FLAME_FAN_SPREAD 0.26f

// NOTE(zoubir): Flame fan (dash, cast): three fireballs in a fan instead
// of one
internal void
ComboFlameFan(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Player, v2 Aim, player_tick *Tick)
{
    for(int Side = -1; Side <= 1; Side++)
    {
        float Angle = (float)Side * FLAME_FAN_SPREAD;
        v2 Dir = V2(Aim.X * Cos(Angle) - Aim.Y * Sin(Angle),
                    Aim.X * Sin(Angle) + Aim.Y * Cos(Angle));
        SpawnFireBall(AppState, World, Arena, Player, Dir, Tick);
    }
}

// NOTE(zoubir): the least speed a long jump leaves with (a walk is about
// 260, a dash starts at 1300), and the share of the air drag left until
// it lands (movement.cpp)
#define LONG_JUMP_SPEED (380.f * PLAYER_MOVE_SCALE)
#define LONG_JUMP_DRAG_SCALE 0.2f

// NOTE(zoubir): Long jump (dash, jump): the jump keeps the dash's speed
// and the air barely slows it, so it goes about twice as far
internal void
ComboLongJump(app_state *AppState, world *World, memory_arena *Arena,
              world_entity *Player, v2 Aim, player_tick *Tick)
{
    v2 Dir = Aim;
    float Speed = Length(Player->Velocity.XY);
    if (Speed > 1.f)
    {
        Dir = Player->Velocity.XY * (1.f / Speed);
    }
    Player->Velocity.XY = Maximum(Speed, LONG_JUMP_SPEED) * Dir;
    Player->LongJump = true;
}

global_variable player_combo PlayerCombos[] =
{
    {"LUNGE", {ComboMove_Dash, ComboMove_Attack}, 0.35f,
     SimBurst_Lunge, false, ComboLunge},
    {"SKEWER", {ComboMove_Jump, ComboMove_Dash, ComboMove_Attack}, 0.45f,
     SimBurst_Skewer, false, ComboSkewer},
    {"CUTTING DASH", {ComboMove_Attack, ComboMove_Dash}, 0.4f,
     SimBurst_CuttingDash, false, ComboCuttingDash},
    {"FLAME FAN", {ComboMove_Dash, ComboMove_Cast}, 0.35f,
     SimBurst_FlameFan, false, ComboFlameFan},
    {"LONG JUMP", {ComboMove_Dash, ComboMove_Jump}, 0.3f,
     SimBurst_LongJump, true, ComboLongJump},
    {"AMBUSH", {ComboMove_Blink, ComboMove_Attack}, 0.5f,
     SimBurst_Ambush, false, ComboAmbush},
};

// NOTE(zoubir): the combo whose burst is Burst, 0 for none
internal player_combo *
FindComboByBurst(u32 Burst)
{
    player_combo *Result = 0;
    for(u32 Index = 0; Index < ArrayCount(PlayerCombos); Index++)
    {
        if ((u32)PlayerCombos[Index].Burst == Burst)
        {
            Result = &PlayerCombos[Index];
        }
    }
    return Result;
}

inline void
AgeComboTrail(world_entity *Player, float DeltaTime)
{
    for(u32 Index = 0; Index < PLAYER_COMBO_TRAIL; Index++)
    {
        // NOTE(zoubir): capped, so an old entry's age stays exact enough
        // to compare against a window
        Player->ComboTrailAge[Index] =
            Minimum(60.f, Player->ComboTrailAge[Index] + DeltaTime);
    }
}

inline void
RecordComboMove(world_entity *Player, combo_move Move)
{
    for(u32 Index = PLAYER_COMBO_TRAIL - 1; Index > 0; Index--)
    {
        Player->ComboTrail[Index] = Player->ComboTrail[Index - 1];
        Player->ComboTrailAge[Index] = Player->ComboTrailAge[Index - 1];
    }
    Player->ComboTrail[0] = Move;
    Player->ComboTrailAge[0] = 0.f;
}

// NOTE(zoubir): the combo Move would finish now, 0 for none
internal player_combo *
FindPlayerCombo(world_entity *Player, combo_move Move)
{
    player_combo *Result = 0;
    u32 ResultLength = 0;
    for(u32 Index = 0; Index < ArrayCount(PlayerCombos); Index++)
    {
        player_combo *Combo = &PlayerCombos[Index];
        u32 Length = 0;
        while (Length < COMBO_MAX_MOVES && Combo->Moves[Length] != ComboMove_None)
        {
            Length++;
        }
        if (Length < 2 || Length <= ResultLength ||
            Combo->Moves[Length - 1] != Move)
        {
            continue;
        }
        // NOTE(zoubir): walk back from the newest entry; each must be the
        // row's move and within the window of the one after it
        bool32 Match = true;
        float Later = 0.f;
        for(u32 Back = 0; Back + 1 < Length && Match; Back++)
        {
            Match = Player->ComboTrail[Back] == (u32)Combo->Moves[Length - 2 - Back] &&
                Player->ComboTrailAge[Back] - Later <= Combo->Window;
            Later = Player->ComboTrailAge[Back];
        }
        if (Match)
        {
            Result = Combo;
            ResultLength = Length;
        }
    }
    return Result;
}

// NOTE(zoubir): called by every move as it starts (after it, for the
// movement abilities and the jump); returns whether it finished a combo
internal bool32
RunPlayerCombo(app_state *AppState, world *World, memory_arena *Arena,
               world_entity *Player, combo_move Move, player_tick *Tick)
{
    player_combo *Combo = FindPlayerCombo(Player, Move);
    bool32 Predicted = AppState->Players[Player->PlayerIndex].Predicted;
    if (Combo && (!Predicted || Combo->MovesOnlyPlayer))
    {
        v2 Aim = GetPlayerAim(Player);
        Combo->Fire(AppState, World, Arena, Player, Aim, Tick);
        // NOTE(zoubir): a swing or cast points along the aim, anything
        // else the way the player is going
        v2 Facing = Aim;
        if (Move != ComboMove_Attack && Move != ComboMove_Cast &&
            LengthSq(Player->Velocity.XY) > 1.f)
        {
            Facing = Player->Velocity.XY;
        }
        EmitBurst(&AppState->Events, Combo->Burst, (u8)Player->PlayerIndex,
                  Player->Position, ATan2(Facing.Y, Facing.X));
    }
    RecordComboMove(Player, Move);
    return Combo != 0;
}
