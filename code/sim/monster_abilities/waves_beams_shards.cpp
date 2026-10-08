/* Waves, beams and shards: the Aurora Rift's ability kinds
   (docs/dungeon-rift.md), each a new way to dodge.

   Wave     a ring of frost rolls out along the ground. It goes through
            walls and outruns everyone, so it cannot be stepped out of:
            it is jumped, the moment it reaches you.
   Beam     a lance of light sweeps an arc across its target, hitting
            each player once as its edge crosses them. It cannot be
            jumped; the way to live through it is to stay ahead of the
            sweep or put a pillar between you and the monster.
   Shatter  a death effect: the corpse bursts into shards flying out all
            the way round, so whoever lands the kill in melee eats them.

   Everything here is worked out from the ability's timer, its aim and its
   points, which snapshots carry, so clients draw the rings and the beams
   where the server hits (client/dungeon/rift_fx.cpp). */

// NOTE(zoubir): the beam is walked this far at a time to find the wall
// that cuts it short
#define BEAM_STEP 6.f

// NOTE(zoubir): in shots_and_hazards.cpp, included after this
internal world_entity *AddMonsterShot(app_state *AppState, world *World,
                                      memory_arena *Arena, world_entity *Owner,
                                      monster_ability *Ability, v2 Direction);

inline v2
RotateBy(v2 V, float Angle)
{
    float C = Cos(Angle);
    float S = Sin(Angle);
    v2 Result = V2(C * V.X - S * V.Y, S * V.X + C * V.Y);
    return Result;
}

// NOTE(zoubir): seconds since the Active part began, 0 before it
inline float
AbilityActiveElapsed(world_entity *Entity, monster_ability *Ability)
{
    float Result = 0.f;
    if (Entity->AbilityPhase == AbilityPhase_Active)
    {
        Result = Maximum(0.f, Ability->Active - Entity->AbilityTimer);
    }
    return Result;
}

// NOTE(zoubir): how far ring Ring of a wave has rolled Elapsed seconds
// into its Active part; below 0 it has not left the monster yet
inline float
WaveFront(monster_ability *Ability, float Elapsed, u32 Ring)
{
    float Result = Ability->Speed * Elapsed - (float)Ring * Ability->Spread;
    return Result;
}

// NOTE(zoubir): each ring hits the players its front passed this frame:
// out there before, at or inside it now. A player running in toward the
// ring crosses it too, so where they were is taken from their velocity
internal void
UpdateWave(app_state *AppState, world *World, world_entity *Entity,
           monster_ability *Ability, float DeltaTime)
{
    float Elapsed = AbilityActiveElapsed(Entity, Ability);
    float Before = Maximum(0.f, Elapsed - DeltaTime);
    u32 Rings = Maximum(1u, Ability->Count);
    v2 Centre = Entity->Position.XY;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player ||
            Player->Hp <= 0.f || IsJumpingClear(Player))
        {
            continue;
        }
        v2 Away = Player->Position.XY - Centre;
        float Distance = Length(Away);
        if (Distance > Ability->Radius)
        {
            continue;
        }
        v2 Out = Distance > 0.f ? (1.f / Distance) * Away : Entity->AbilityAim;
        float WasAt = Distance - DeltaTime * DotProduct(Player->Velocity.XY, Out);
        for(u32 Ring = 0; Ring < Rings; Ring++)
        {
            float FrontNow = WaveFront(Ability, Elapsed, Ring);
            float FrontBefore = WaveFront(Ability, Before, Ring);
            if (FrontNow <= 0.f || FrontBefore >= Ability->Radius)
            {
                continue;
            }
            if (WasAt > FrontBefore && Distance <= FrontNow)
            {
                HitPlayerWith(AppState, World, Player, Entity, Ability->Damage,
                              Ability->Knockback * Out, 0.f, Ability->Status,
                              Ability->StatusSeconds, SimBurst_MonsterHit);
                break;
            }
        }
    }
}

// NOTE(zoubir): the sweep turns this way, +1 or -1, kept in the first
// point so clients sweep the same way
inline float
BeamTurn(world_entity *Entity)
{
    float Result = (Entity->AbilityPointCount && Entity->AbilityPoints[0].X < 0.f) ? -1.f : 1.f;
    return Result;
}

// NOTE(zoubir): the beam starts half its arc to one side of the target and
// sweeps across them to the other
internal void
StartBeam(app_state *AppState, world_entity *Entity, monster_ability *Ability)
{
    float Turn = MonsterRandomBetween(AppState, 0.f, 1.f) < 0.5f ? -1.f : 1.f;
    float Half = 0.5f * Ability->Spread * (Pi32 / 180.f);
    Entity->AbilityAim = RotateBy(Entity->AbilityAim, -Turn * Half);
    Entity->AbilityPoints[Entity->AbilityPointCount++] = V2(Turn, 0.f);
}

// NOTE(zoubir): where the beam points now: its starting aim through the
// windup, then turning steadily through the Active part
inline v2
BeamDirection(world_entity *Entity, monster_ability *Ability)
{
    float Share = Ability->Active > 0.f ?
        AbilityActiveElapsed(Entity, Ability) / Ability->Active : 0.f;
    Share = Minimum(1.f, Share);
    float Angle = BeamTurn(Entity) * Share * Ability->Spread * (Pi32 / 180.f);
    v2 Result = RotateBy(Entity->AbilityAim, Angle);
    return Result;
}

// NOTE(zoubir): how far the beam reaches from From along Direction before
// the first tile that blocks (a wall, a pillar), at most Ability->Speed
internal float
BeamReach(world *World, v2 From, v2 Direction, monster_ability *Ability)
{
    map_def *Map = GetMapDef((map_id)World->MapId);
    float Tile = (float)World->TileWidth;
    float Result = Ability->Speed;
    if (!Map || Tile <= 0.f)
    {
        return Result;
    }
    for(float Along = BEAM_STEP; Along < Ability->Speed; Along += BEAM_STEP)
    {
        v2 Point = From + Along * Direction;
        i32 X = (i32)floorf(Point.X / Tile);
        i32 Y = (i32)floorf(Point.Y / Tile);
        if (GetTerrainDef(TerrainAt(Map, X, Y))->Blocks)
        {
            Result = Along;
            break;
        }
    }
    return Result;
}

// NOTE(zoubir): the angle from Beam to Way, positive the way the beam
// turns, in (-Pi, Pi]
inline float
AngleAhead(v2 Beam, v2 Way, float Turn)
{
    float Result = Turn * atan2f(Beam.X * Way.Y - Beam.Y * Way.X, DotProduct(Beam, Way));
    return Result;
}

// NOTE(zoubir): each frame of the sweep, a player the beam's edge passes
// over is hit: ahead of an edge last frame, at or behind it now. Where
// they were is taken from their velocity, so one who runs back through
// the beam is hit too. On the first frame whoever stands in it is hit.
// A pillar between them and the monster keeps them out of reach
internal void
UpdateBeam(app_state *AppState, world *World, world_entity *Entity,
           monster_ability *Ability, float DeltaTime)
{
    float Elapsed = AbilityActiveElapsed(Entity, Ability);
    bool32 First = Elapsed <= DeltaTime;
    float Turn = BeamTurn(Entity);
    v2 From = Entity->Position.XY;
    v2 Direction = BeamDirection(Entity, Ability);
    float Share = Ability->Active > 0.f ?
        Maximum(0.f, Elapsed - DeltaTime) / Ability->Active : 0.f;
    v2 Before = RotateBy(Entity->AbilityAim,
                         Turn * Minimum(1.f, Share) * Ability->Spread * (Pi32 / 180.f));
    float Reach = BeamReach(World, From, Direction, Ability);
    v2 Push = (Turn * Ability->Knockback) * V2(-Direction.Y, Direction.X);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Player = &World->Entities[EntityIndex];
        if (!Player->IsPresent || Player->Type != EntityType_Player || Player->Hp <= 0.f)
        {
            continue;
        }
        v2 Way = Player->Position.XY - From;
        float Distance = Length(Way);
        if (Distance > Reach + Ability->Radius || Distance <= 0.f)
        {
            continue;
        }
        v2 WasWay = Way - DeltaTime * Player->Velocity.XY;
        // NOTE(zoubir): the beam's half width, as an angle at this distance
        float Edge = Ability->Radius / Maximum(Distance, Ability->Radius);
        float Now = AngleAhead(Direction, Way, Turn);
        float Was = AngleAhead(Before, WasWay, Turn);
        bool32 Inside = Now <= Edge && Now >= -Edge;
        bool32 Crossed = (Was > Edge && Now <= Edge) || (Was < -Edge && Now >= -Edge);
        if ((First && Inside) || (!First && Crossed))
        {
            HitPlayerWith(AppState, World, Player, Entity, Ability->Damage, Push,
                          0.f, Ability->Status, Ability->StatusSeconds,
                          SimBurst_MonsterHit);
        }
    }
}

// NOTE(zoubir): a shattering corpse throws the Volley at ShatterAbility
// all the way round, from where it fell; the shots carry its kind, the
// ability's slot and its affix like any other
internal void
ShatterMonster(app_state *AppState, world *World, memory_arena *Arena,
               monster_death_record *Record, monster_def *Def)
{
    if (Def->ShatterAbility >= Def->AbilityCount)
    {
        return;
    }
    monster_ability *Ability = &Def->Abilities[Def->ShatterAbility];
    world_entity Corpse;
    ZeroSize(&Corpse, sizeof(Corpse));
    Corpse.Position = Record->Position;
    Corpse.GroundZ = Record->Position.Z;
    Corpse.MonsterKind = Record->Kind;
    Corpse.AbilityIndex = Def->ShatterAbility;
    Corpse.EliteAffix = Record->EliteAffix;
    u32 Count = Maximum(1u, Ability->Count);
    float Offset = MonsterRandomBetween(AppState, 0.f, 2.f * Pi32);
    for(u32 Shard = 0; Shard < Count; Shard++)
    {
        float Angle = Offset + 2.f * Pi32 * (float)Shard / (float)Count;
        AddMonsterShot(AppState, World, Arena, &Corpse, Ability,
                       V2(Cos(Angle), Sin(Angle)));
    }
}
