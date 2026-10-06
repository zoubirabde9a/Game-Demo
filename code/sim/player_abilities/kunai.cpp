/* Kunai (V): a thrown blade, faster than a fireball, aimed at a unit, not
   at the ground. The throw needs an enemy (a player or a monster) within
   KUNAI_PICK_RADIUS of the cursor and within its range; with none the key
   does nothing and the cooldown is not spent (CanStartSpawnAction). Once
   thrown it heads straight for that unit every tick, so it follows it
   wherever it runs or blinks, for up to KUNAI_FLIGHT_SECONDS. A jump
   lets it pass under for a moment, a wall stops it, and it falls when
   its unit dies. It hits the first enemy unit it comes near (by
   distance, so it never blocks anyone). The client marks the unit a
   throw would pick (client/player_fx/kunai_fx.cpp).

   A player whose shield is up (SpawnShield: the E shield, the respawn
   shield) is not hurt: the kunai glances off and homes on whoever threw
   it, and the shielded player becomes its owner, so a kill with it is
   theirs. If the thrower's shield is up too it comes back again, as long
   as both keep their shields up. Each bounce gives it its full flight
   time again.

   Its numbers are in player_stats.cpp, its timing row in
   spawn_actions.cpp. The server simulates it; clients draw the replica
   (client/player_fx/kunai_fx.cpp). */

// NOTE(zoubir): how near the cursor a unit must stand to be the target;
// with the cursor at the aim's limit (PLAYER_AIM_REACH) or past it, how
// near the aim's line beyond that
#define KUNAI_PICK_RADIUS 48.f
// NOTE(zoubir): the longest a kunai flies before it drops, chasing or
// sent back by a shield
#define KUNAI_FLIGHT_SECONDS 2.f
// NOTE(zoubir): how near it must come to a unit to hit it, across the
// ground; a little over a player's half width
#define KUNAI_HIT_RADIUS 18.f
#define KUNAI_HAND_HEIGHT 30.f
#define KUNAI_SIZE 16.f

internal world_entity *
AddKunai(app_state *AppState, world *World, memory_arena *Arena, v3 Position,
         v3 Velocity)
{
    world_entity *Kunai = AddEntity(AppState, World, Arena, EntityType_Kunai,
                                    Position, AppState->FireBallCollision);
    Kunai->Velocity = Velocity;
    Kunai->Dimensions = V2(KUNAI_SIZE, KUNAI_SIZE);
    Kunai->TimeLeft = KUNAI_FLIGHT_SECONDS;
    return Kunai;
}

// NOTE(zoubir): a unit the kunai thrown by Owner may hit: alive, present,
// a player or a monster, not its owner
inline bool32
IsKunaiTarget(world_entity *Unit, world_entity *Owner)
{
    bool32 Result = Unit->IsPresent && Unit != Owner && Unit->Hp > 0.f &&
        (Unit->Type == EntityType_Player || Unit->Type == EntityType_Monster);
    return Result;
}

inline world_entity *
KunaiOwner(app_state *AppState, world_entity *Kunai)
{
    world_entity *Result = Kunai->HasOwner ? AppState->Players[Kunai->OwnerSlot].Entity : 0;
    return Result;
}

// NOTE(zoubir): the enemy Owner's cursor is on: within KUNAI_PICK_RADIUS
// of the cursor (Aim, a unit direction, times AimReach of the aim's
// limit) and within the kunai's range, the nearest the cursor winning.
// A cursor at the limit may be farther out, so then the spot slides out
// along the aim to the unit, up to the range. 0 for none
internal world_entity *
FindKunaiTarget(world *World, world_entity *Owner, v2 Aim, float AimReach)
{
    world_entity *Result = 0;
    float BestMiss = 0.f;
    bool32 AtLimit = AimReach >= 0.999f;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Unit = &World->Entities[Index];
        if (!IsKunaiTarget(Unit, Owner))
        {
            continue;
        }
        v2 To = Unit->Position.XY - Owner->Position.XY;
        if (Length(To) > PlayerStats.KunaiRange)
        {
            continue;
        }
        float Along = DotProduct(To, Aim);
        float Spot = AtLimit ?
            Minimum(PlayerStats.KunaiRange, Maximum(PLAYER_AIM_REACH, Along)) :
            AimReach * PLAYER_AIM_REACH;
        float Miss = Length(To - Spot * Aim);
        if (Miss <= KUNAI_PICK_RADIUS && (!Result || Miss < BestMiss))
        {
            Result = Unit;
            BestMiss = Miss;
        }
    }
    return Result;
}

// NOTE(zoubir): the unit a kunai thrown now would go for (spawn_actions.cpp
// checks it before the key spends anything)
inline world_entity *
KunaiTargetFor(world *World, world_entity *Player)
{
    float Reach = Player->AimReach > 0.f ? Player->AimReach : 1.f;
    world_entity *Result = FindKunaiTarget(World, Player, GetPlayerAim(Player), Reach);
    return Result;
}

// NOTE(zoubir): the kunai key's spawn (spawn_actions.cpp)
internal void
ThrowKunai(app_state *AppState, world *World, memory_arena *Arena,
           world_entity *Player, v2 Dir, player_tick *Tick)
{
    world_entity *Target = KunaiTargetFor(World, Player);
    if (!Target)
    {
        return;
    }
    v2 Toward = NormalizeOr(Target->Position.XY - Player->Position.XY, Dir);
    v2 Start = Player->Position.XY + 24.f * Toward;
    float Height = GroundHeightAt(World, Player->Position.XY) + KUNAI_HAND_HEIGHT;
    v2 Velocity = PlayerStats.KunaiSpeed * Toward;
    world_entity *Kunai = AddKunai(AppState, World, Arena, V3(Start.X, Start.Y, Height),
                                   V3(Velocity.X, Velocity.Y, 0.f));
    Kunai->HasOwner = true;
    Kunai->OwnerSlot = Player->PlayerIndex;
    Kunai->FollowingEntity = Target;
}

// NOTE(zoubir): the kunai glances off Shielded, whose player now owns it,
// and heads back for its last owner
internal void
ReflectKunai(app_state *AppState, world_entity *Kunai, world_entity *Shielded)
{
    world_entity *Thrower = KunaiOwner(AppState, Kunai);
    Kunai->HasOwner = true;
    Kunai->OwnerSlot = Shielded->PlayerIndex;
    Kunai->FollowingEntity = (Thrower && IsKunaiTarget(Thrower, Shielded)) ? Thrower : 0;
    float Speed = Length(Kunai->Velocity.XY);
    v2 Back = Kunai->FollowingEntity ?
        NormalizeOr(Kunai->FollowingEntity->Position.XY - Kunai->Position.XY,
                    -Kunai->Velocity.XY) :
        NormalizeOr(-Kunai->Velocity.XY, V2(1.f, 0.f));
    Kunai->Velocity.XY = Speed * Back;
    Kunai->TimeLeft = KUNAI_FLIGHT_SECONDS;
    EmitBurst(&AppState->Events, SimBurst_KunaiReflect, (u8)Shielded->PlayerIndex,
              Kunai->Position, ATan2(Back.Y, Back.X));
    EmitSound(&AppState->Events, AssetType_Dash, Kunai->Position);
}

internal void
KunaiHit(app_state *AppState, world *World, world_entity *Kunai, world_entity *Target)
{
    world_entity *Owner = KunaiOwner(AppState, Kunai);
    float Scale = Owner ? PlayerPowerScale(AppState, Owner, PlayerButton_Kunai) : 1.f;
    hit Hit = {Scale * PlayerStats.KunaiDamage, 90.f, 0.f, 60.f, 0.f, SimBurst_Impact};
    v2 Away = NormalizeOr(Kunai->Velocity.XY, Target->Position.XY - Kunai->Position.XY);
    u32 BySlot = Kunai->HasOwner ? Kunai->OwnerSlot : SIM_NOBODY;
    ApplyHit(AppState, World, Target, &Hit, Away, Kunai, BySlot);
}

internal void
UpdateKunai(world_entity *Kunai, world *World, memory_arena *Arena,
            float DeltaTime, app_state *AppState)
{
    Kunai->TimeLeft -= DeltaTime;
    if (Kunai->TimeLeft <= 0.f)
    {
        RemoveEntity(World, Kunai);
        return;
    }

    // NOTE(zoubir): it was thrown at one unit and drops when that unit
    // dies or goes
    world_entity *Owner = KunaiOwner(AppState, Kunai);
    world_entity *Target = Kunai->FollowingEntity;
    if (!Target || !IsKunaiTarget(Target, Owner))
    {
        Kunai->FollowingEntity = 0;
        RemoveEntity(World, Kunai);
        return;
    }
    v2 Want = Target->Position.XY - Kunai->Position.XY;
    if (LengthSq(Want) > 0.0001f)
    {
        Kunai->Velocity.XY = PlayerStats.KunaiSpeed * NormalizeOr(Want, V2(1.f, 0.f));
    }

    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Unit = &World->Entities[Index];
        if (IsKunaiTarget(Unit, Owner) && !IsJumpingClear(Unit) &&
            Length(Unit->Position.XY - Kunai->Position.XY) <= KUNAI_HIT_RADIUS)
        {
            if (Unit->Type == EntityType_Player && Unit->SpawnShield > 0.f)
            {
                ReflectKunai(AppState, Kunai, Unit);
                break;
            }
            KunaiHit(AppState, World, Kunai, Unit);
            RemoveEntity(World, Kunai);
            return;
        }
    }

    v3 Start = Kunai->Position;
    float Expected = Length(Kunai->Velocity.XY) * DeltaTime;
    v3 DDEntity = {};
    float MaxDistance = 10000.f;
    MoveEntity(Kunai, World, Arena, DeltaTime, AppState, DDEntity, &MaxDistance);
    if (Kunai->IsPresent && Length(Kunai->Position.XY - Start.XY) < 0.5f * Expected)
    {
        RemoveEntity(World, Kunai);
    }
}
