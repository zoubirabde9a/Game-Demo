/* Kunai (V): a thrown blade, faster than a fireball, that homes. On the
   throw it picks the nearest enemy unit (a player or a monster) inside a
   cone around the aim and within its range, then turns toward that unit
   every tick at KunaiTurnRate, so a sharp sidestep, a jump or a blink can
   still beat it. With nothing in the cone it flies straight along the
   aim. It hits the first enemy unit it comes near (by distance, so it
   never blocks anyone), and walls stop it.

   A player whose shield is up (SpawnShield: the E shield, the respawn
   shield) is not hurt: the kunai glances off and homes on whoever threw
   it, and the shielded player becomes its owner, so a kill with it is
   theirs. If the thrower's shield is up too it comes back again, as long
   as both keep their shields up. Each bounce gives it its full flight
   time again.

   Its numbers are in player_stats.cpp, its timing row in
   spawn_actions.cpp. The server simulates it; clients draw the replica
   (client/player_fx/kunai_fx.cpp). */

// NOTE(zoubir): cosine of the cone's half angle around the aim (about 40
// degrees) in which the throw looks for a target
#define KUNAI_SEEK_COS 0.76f
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
    Kunai->TimeLeft = PlayerStats.KunaiRange / PlayerStats.KunaiSpeed;
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

// NOTE(zoubir): the enemy nearest From inside the cone around Aim and
// within the kunai's range; units off to the side count as farther, so
// the one along the aim wins a near tie. 0 for none
internal world_entity *
FindKunaiTarget(world *World, world_entity *Owner, v2 From, v2 Aim)
{
    world_entity *Result = 0;
    float BestScore = 0.f;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Unit = &World->Entities[Index];
        if (!IsKunaiTarget(Unit, Owner))
        {
            continue;
        }
        v2 To = Unit->Position.XY - From;
        float Distance = Length(To);
        if (Distance > PlayerStats.KunaiRange || Distance < 0.001f)
        {
            continue;
        }
        float Cos = DotProduct(To, Aim) / Distance;
        if (Cos < KUNAI_SEEK_COS)
        {
            continue;
        }
        float Score = Distance * (2.f - Cos);
        if (!Result || Score < BestScore)
        {
            Result = Unit;
            BestScore = Score;
        }
    }
    return Result;
}

// NOTE(zoubir): the kunai key's spawn (spawn_actions.cpp)
internal void
ThrowKunai(app_state *AppState, world *World, memory_arena *Arena,
           world_entity *Player, v2 Dir, player_tick *Tick)
{
    v2 Start = Player->Position.XY + 24.f * Dir;
    float Height = GroundHeightAt(World, Player->Position.XY) + KUNAI_HAND_HEIGHT;
    v2 Velocity = PlayerStats.KunaiSpeed * Dir;
    world_entity *Kunai = AddKunai(AppState, World, Arena, V3(Start.X, Start.Y, Height),
                                   V3(Velocity.X, Velocity.Y, 0.f));
    Kunai->HasOwner = true;
    Kunai->OwnerSlot = Player->PlayerIndex;
    Kunai->FollowingEntity = FindKunaiTarget(World, Player, Player->Position.XY, Dir);
}

// NOTE(zoubir): Velocity turned toward Want by at most MaxTurn radians,
// keeping its speed
internal v2
TurnToward(v2 Velocity, v2 Want, float MaxTurn)
{
    float Speed = Length(Velocity);
    float Have = ATan2(Velocity.Y, Velocity.X);
    float Delta = ATan2(Want.Y, Want.X) - Have;
    while (Delta > Pi32) Delta -= 2.f * Pi32;
    while (Delta < -Pi32) Delta += 2.f * Pi32;
    Delta = Minimum(MaxTurn, Maximum(-MaxTurn, Delta));
    v2 Result = Speed * V2(Cos(Have + Delta), Sin(Have + Delta));
    return Result;
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
    Kunai->TimeLeft = PlayerStats.KunaiRange / PlayerStats.KunaiSpeed;
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

    world_entity *Owner = KunaiOwner(AppState, Kunai);
    world_entity *Target = Kunai->FollowingEntity;
    if (Target && !IsKunaiTarget(Target, Owner))
    {
        Target = Kunai->FollowingEntity = 0;
    }
    if (Target)
    {
        v2 Want = Target->Position.XY - Kunai->Position.XY;
        if (LengthSq(Want) > 0.0001f)
        {
            Kunai->Velocity.XY = TurnToward(Kunai->Velocity.XY, Want,
                                            PlayerStats.KunaiTurnRate * DeltaTime);
        }
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
