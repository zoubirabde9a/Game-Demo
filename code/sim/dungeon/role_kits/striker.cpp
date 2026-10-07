/* The damage role's kit (role_abilities.cpp): Meteor on A and Giant
   Fireball on R, both with a cast; from its tree Detonate on C and
   Combustion on V. X is the game's fireball, as for everyone.

   The rotation is build and spend. Every fireball a striker lands, and
   every monster a Meteor or a Giant Fireball blast catches, puts a
   Searing stack on it, up to SEARING_MOST; the mark fades
   SEARING_SECONDS after its last stack. Detonate blows the marks up for
   DETONATE_DAMAGE plus DETONATE_PER_STACK a stack, so it pays to spend it
   on a full mark and not to let marks fade.

   Meteor winds up for a second (PlayerSpell_Meteor, sim/player_casts.cpp)
   at the spot the cursor was on when it was pressed, then calls a meteor
   down there. It lands INFERNO_DELAY later, strikes everything inside for
   INFERNO_DAMAGE and marks it; the ground then burns for
   INFERNO_BURN_SECONDS, keeping marks alive. Detonating a monster in
   burning ground detonates every other marked monster in that fire too.

   Giant Fireball winds up for 1.5 s (PlayerSpell_GiantFireball), then a
   slow ball flies along the aim the striker cast with. It blows up on the
   first monster it reaches, when it leaves the room it was cast in, or
   at the end of its flight: everything within GIANT_FIREBALL_RADIUS takes
   GIANT_FIREBALL_DAMAGE and a stack. Combustion makes every hit the
   striker lands COMBUSTION_SHARE harder for COMBUSTION_SECONDS
   (DungeonScaleDamage).

   The damage counts as the caster's, so their class and talents scale it
   and it makes threat for them. */

// NOTE(zoubir): from OnRoleHit: the striker's fireball landed
internal void
OnStrikerShot(app_state *AppState, world_entity *Target)
{
    AddSearing(AppState->Dungeon, &AppState->World, Target);
}

// NOTE(zoubir): the burning inferno Monster stands in, 0 for none
internal inferno *
InfernoUnder(dungeon_run *Run, world_entity *Monster)
{
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        if (Zone->Delay <= 0.f && Zone->Seconds > 0.f &&
            Length(Monster->Position.XY - Zone->Position.XY) <= Zone->Radius)
        {
            return Zone;
        }
    }
    return 0;
}

// NOTE(zoubir): the monster Detonate goes for: the one under the cursor
// in reach, else the marked one nearest the cursor within
// DETONATE_PICK_RADIUS of it; 0 for none
internal world_entity *
DetonateTarget(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    dungeon_run *Run = AppState->Dungeon;
    u32 Index = Slot->Input.Target;
    if (Index && Index - 1 < World->EntityCount)
    {
        world_entity *Unit = &World->Entities[Index - 1];
        if (Unit->IsPresent && Unit->Type == EntityType_Monster && Unit->Hp > 0.f &&
            Length(Unit->Position.XY - Player->Position.XY) <= DETONATE_RANGE)
        {
            return Unit;
        }
    }
    v2 Point = AimPoint(Player);
    world_entity *Result = 0;
    float Best = DETONATE_PICK_RADIUS;
    for(u32 Row = 0; Row < MAX_FOE_MARKS; Row++)
    {
        foe_mark *Mark = &Run->Marks[Row];
        world_entity *Monster = Mark->Stacks ?
            FindMonsterBySerial(World, Mark->Slot, Mark->Serial) : 0;
        if (!Monster || Monster->Hp <= 0.f ||
            Length(Monster->Position.XY - Player->Position.XY) > DETONATE_RANGE)
        {
            continue;
        }
        float Distance = Length(Monster->Position.XY - Point);
        if (Distance < Best)
        {
            Best = Distance;
            Result = Monster;
        }
    }
    return Result;
}

// NOTE(zoubir): Monster's mark goes off for the striker Player; returns
// the stacks it had
internal u32
DetonateMark(app_state *AppState, world_entity *Player, world_entity *Monster)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    foe_mark *Mark = FindFoeMark(Run, World, Monster);
    u32 Stacks = Mark ? Mark->Stacks : 0;
    if (Mark)
    {
        Mark->Stacks = 0;
        Mark->Seconds = 0.f;
    }
    EmitBurst(&AppState->Events, SimBurst_InfernoBlast, (u8)Player->PlayerIndex,
              Monster->Position);
    player_slot *Slot = &AppState->Players[Player->PlayerIndex];
    // NOTE(zoubir): Detonate's second rank, once Searing Heat
    float PerStack = DETONATE_PER_STACK +
        (RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Detonate) >= 2 ? SEARING_HEAT_PER_STACK : 0.f);
    DamageEntity(AppState, World, Monster, DETONATE_DAMAGE + PerStack * (float)Stacks, Player);
    return Stacks;
}

// NOTE(zoubir): Detonate on the monster DetonateTarget picks, and in fire
// on every marked monster in the same inferno; false with no target, so
// the cooldown is not spent
internal bool32
CastDetonate(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world_entity *Target = DetonateTarget(AppState, Slot, Player);
    if (!Target)
    {
        return false;
    }
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    inferno *Fire = InfernoUnder(Run, Target);
    u32 Stacks = DetonateMark(AppState, Player, Target);
    if (Stacks >= SEARING_MOST && RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Overload))
    {
        Slot->CastRefund = OVERLOAD_SECONDS;
    }
    if (Fire)
    {
        for(u32 Row = 0; Row < MAX_FOE_MARKS; Row++)
        {
            foe_mark *Mark = &Run->Marks[Row];
            world_entity *Monster = Mark->Stacks ?
                FindMonsterBySerial(World, Mark->Slot, Mark->Serial) : 0;
            if (Monster && Monster != Target && Monster->Hp > 0.f &&
                Length(Monster->Position.XY - Fire->Position.XY) <= Fire->Radius)
            {
                DetonateMark(AppState, Player, Monster);
            }
        }
    }
    return true;
}

// NOTE(zoubir): returns whether the key cast. Meteor and Giant Fireball
// only start their cast here (FinishStrikerCast fires them); Detonate
// with no marked monster in reach does not cast
internal bool32
CastStrikerKey(app_state *AppState, player_slot *Slot, world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    switch(Key)
    {
        case 0:
        {
            Slot->RoleCastPoint = AimPoint(Player);
            StartPlayerCast(Player, PlayerSpell_Meteor, Player->Aim);
        } break;

        case 1:
        {
            StartPlayerCast(Player, PlayerSpell_GiantFireball, Player->Aim);
        } break;

        case 2:
        {
            return CastDetonate(AppState, Slot, Player);
        } break;

        case 3:
        {
            Slot->CombustSeconds = COMBUSTION_SECONDS;
            EmitBurst(&AppState->Events, SimBurst_InfernoCast, SlotIndex, ChestOf(Player),
                      ATan2(Player->Aim.Y, Player->Aim.X));
        } break;
    }
    return true;
}

// NOTE(zoubir): the meteor called down at Slot's cast point; nothing when
// every inferno is in use (the cooldown was spent at the press)
internal void
CallMeteor(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    dungeon_run *Run = AppState->Dungeon;
    inferno *Free = 0;
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        if (Zone->Delay <= 0.f && Zone->Seconds <= 0.f)
        {
            Free = Zone;
            break;
        }
    }
    if (!Free)
    {
        return;
    }
    u8 SlotIndex = (u8)Player->PlayerIndex;
    v2 Point = Slot->RoleCastPoint;
    Free->Position = V3(Point.X, Point.Y, Player->GroundZ);
    Free->Delay = INFERNO_DELAY;
    Free->Seconds = INFERNO_BURN_SECONDS + WILDFIRE_SECONDS *
        (float)RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Wildfire);
    Free->Radius = RoleSpellRadius(Slot, 0);
    Free->By = SlotIndex;
    EmitBurst(&AppState->Events, SimBurst_InfernoCast, SlotIndex, ChestOf(Player),
              ATan2(Point.Y - Player->Position.Y, Point.X - Player->Position.X));
}

// NOTE(zoubir): a Giant Fireball leaves Player's hand along Direction;
// nothing when all are in flight
internal void
LaunchGiantFireball(app_state *AppState, world_entity *Player, v2 Direction)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    v2 Dir = LengthSq(Direction) > 0.f ? DirectionTo(Direction) : V2(1.f, 0.f);
    for(u32 Index = 0; Index < MAX_GIANT_FIREBALLS; Index++)
    {
        giant_fireball *Ball = &Run->GiantFireballs[Index];
        if (Ball->Distance <= 0.f)
        {
            Ball->Position = ChestOf(Player);
            Ball->Position.XY += 24.f * Dir;
            Ball->Velocity = GIANT_FIREBALL_SPEED * Dir;
            Ball->Distance = GIANT_FIREBALL_RANGE;
            Ball->Room = RoomAtPosition(World, Player->Position.XY);
            Ball->By = Player->PlayerIndex;
            // NOTE(zoubir): clients fly their own copy of it from this
            // (client/dungeon/giant_fireball_fx.cpp); it is not in the snapshot
            EmitBurst(&AppState->Events, SimBurst_GiantFireball, (u8)Player->PlayerIndex,
                      Ball->Position, ATan2(Dir.Y, Dir.X));
            return;
        }
    }
}

// NOTE(zoubir): from FinishPlayerCast (sim/player_update/casts.cpp), on
// the server or offline: a class spell's wind-up is over
internal void
FinishRoleCast(app_state *AppState, world_entity *Player, player_spell Spell)
{
    if (!IsDungeon(AppState) || Player->PlayerIndex >= MAX_PLAYERS)
    {
        return;
    }
    player_slot *Slot = &AppState->Players[Player->PlayerIndex];
    if (Spell == PlayerSpell_Meteor)
    {
        CallMeteor(AppState, Slot, Player);
    }
    else if (Spell == PlayerSpell_GiantFireball)
    {
        LaunchGiantFireball(AppState, Player, Player->CastingDirection);
    }
}

// NOTE(zoubir): Damage to every living monster within Radius of Centre,
// by the player in slot By; with Blast, a full hit with its shove and a
// Searing stack, else a burn that keeps the monster's mark alive
internal void
BurnAround(app_state *AppState, world *World, v3 Centre, float Radius, u32 By,
           float Damage, bool32 Blast)
{
    world_entity *Caster = AppState->Players[By].Entity;
    hit Hit = {Damage, 160.f, 140.f, 140.f, 0.f, SimBurst_Count, StatusEffect_Burning, 1.f};
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Centre.XY;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster ||
            Monster->Hp <= 0.f || Length(Offset) > Radius)
        {
            continue;
        }
        dungeon_run *Run = AppState->Dungeon;
        if (Blast)
        {
            AddSearing(Run, World, Monster);
            ApplyHit(AppState, World, Monster, &Hit, DirectionTo(Offset), Caster, By);
        }
        else
        {
            foe_mark *Mark = FindFoeMark(Run, World, Monster);
            if (Mark)
            {
                Mark->Seconds = SEARING_SECONDS;
            }
            DamageEntity(AppState, World, Monster, Damage, Caster);
        }
    }
}

// NOTE(zoubir): once a tick: meteors falling, then the ground burning
internal void
UpdateInfernos(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    UpdateFoeMarks(Run, World, DeltaTime);
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        if (Zone->Delay > 0.f)
        {
            Zone->Delay -= DeltaTime;
            if (Zone->Delay <= 0.f)
            {
                Zone->Delay = 0.f;
                Zone->TickTimer = 0.f;
                BurnAround(AppState, World, Zone->Position, Zone->Radius, Zone->By,
                           INFERNO_DAMAGE, true);
                EmitBurst(&AppState->Events, SimBurst_InfernoBlast, (u8)Zone->By, Zone->Position);
            }
        }
        else if (Zone->Seconds > 0.f)
        {
            Zone->Seconds = Maximum(0.f, Zone->Seconds - DeltaTime);
            Zone->TickTimer += DeltaTime;
            if (Zone->TickTimer >= INFERNO_BURN_TICK)
            {
                Zone->TickTimer -= INFERNO_BURN_TICK;
                BurnAround(AppState, World, Zone->Position, Zone->Radius, Zone->By,
                           INFERNO_BURN_PER_SECOND * INFERNO_BURN_TICK, false);
            }
        }
    }
}

// NOTE(zoubir): once a tick: each Giant Fireball flies on, and blows up on
// the first monster it reaches, on leaving its room or at the end of its
// flight
internal void
UpdateGiantFireballs(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < MAX_GIANT_FIREBALLS; Index++)
    {
        giant_fireball *Ball = &Run->GiantFireballs[Index];
        if (Ball->Distance <= 0.f)
        {
            continue;
        }
        v2 Step = DeltaTime * Ball->Velocity;
        Ball->Position.XY += Step;
        Ball->Distance -= Length(Step);
        bool32 Blow = Ball->Distance <= 0.f ||
            RoomAtPosition(World, Ball->Position.XY) != Ball->Room;
        for(u32 EntityIndex = 0; EntityIndex < World->EntityCount && !Blow; EntityIndex++)
        {
            world_entity *Monster = &World->Entities[EntityIndex];
            Blow = Monster->IsPresent && Monster->Type == EntityType_Monster &&
                Monster->Hp > 0.f &&
                Length(Monster->Position.XY - Ball->Position.XY) <= GIANT_FIREBALL_TOUCH;
        }
        if (Blow)
        {
            Ball->Distance = 0.f;
            if (AppState->Players[Ball->By].Entity)
            {
                BurnAround(AppState, World, Ball->Position, GIANT_FIREBALL_RADIUS, Ball->By,
                           GIANT_FIREBALL_DAMAGE, true);
            }
            EmitBurst(&AppState->Events, SimBurst_GiantFireballBlast, (u8)Ball->By, Ball->Position);
        }
    }
}

// NOTE(zoubir): from DungeonScaleDamage: a hit by Attacker that is about
// to kill Target; no role talent uses it now
internal void
OnRoleKill(player_slot *Attacker)
{
}
