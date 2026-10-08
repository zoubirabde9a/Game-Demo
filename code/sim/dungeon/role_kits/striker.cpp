/* The damage role's kit (role_abilities.cpp): Meteor on A, Giant
   Fireball on R, both with a cast, and Fireguard on C; from its tree
   Combustion on V. X is the game's fireball, as for everyone.

   Searing is the fire every spell leaves behind. A fireball the striker
   lands puts one stack on the monster, a Meteor blast
   SEARING_METEOR_STACKS and a Giant Fireball blast SEARING_GIANT_STACKS,
   up to SEARING_MOST: the bigger the spell, the hotter the burn. Each
   stack burns SEARING_BURN_PER_STACK a second (UpdateSearing). The mark
   lasts SEARING_SECONDS after its last stack, and when it runs out it
   explodes on its own for DETONATE_DAMAGE plus DETONATE_PER_STACK a
   stack, splashing DETONATE_SPLASH_SHARE of that on the monsters round
   it. So the rotation is to stack a mark high and let it go.

   Meteor winds up for a second (PlayerSpell_Meteor, sim/player_casts.cpp)
   at the spot the cursor was on when it was pressed, then calls a meteor
   down there. It lands INFERNO_DELAY later, strikes everything inside for
   INFERNO_DAMAGE and marks it; the ground then burns for
   INFERNO_BURN_SECONDS, keeping marks from running out while monsters
   stand in it. With Molten Ground each burn also slows what stands in it,
   for MOLTEN_GROUND_SLOW_SECONDS a rank.

   Giant Fireball winds up for 1.5 s (PlayerSpell_GiantFireball), then a
   slow ball flies from the striker toward the spot the cursor was on
   when it was pressed, wherever the striker walked meanwhile, and on past
   it. It blows up on the
   first monster it reaches, when it leaves the room it was cast in, or
   at the end of its flight: everything within GIANT_FIREBALL_RADIUS takes
   GIANT_FIREBALL_DAMAGE and its stacks, and with Cataclysm a stun of
   CATACLYSM_STUN_SECONDS, which holds a monster's wind-up (sim/update.cpp)
   until it ends.

   Fireguard wraps the striker in fire that takes the next
   FIREGUARD_ABSORB damage (DungeonScaleDamage) for FIREGUARD_SECONDS.
   What is left of it goes to clients as the slot's ClassMeter, so the
   ring round the striker empties the same online. Combustion makes every
   hit the striker lands COMBUSTION_SHARE harder for COMBUSTION_SECONDS.

   The damage counts as the caster's, so their class and talents scale it
   and it makes threat for them. */

// NOTE(zoubir): from OnRoleHit: the striker in slot By landed a fireball
internal void
OnStrikerShot(app_state *AppState, u32 By, world_entity *Target)
{
    AddSearing(AppState->Dungeon, &AppState->World, Target, 1, By);
}

// NOTE(zoubir): Stacks of Searing on Marked ran out: it explodes, the
// marked monster taking the whole blast and those round it a share
internal void
ExplodeSearing(app_state *AppState, world_entity *Marked, u32 Stacks, u32 By)
{
    world *World = &AppState->World;
    player_slot *Slot = &AppState->Players[By < MAX_PLAYERS ? By : 0];
    world_entity *Caster = By < MAX_PLAYERS ? Slot->Entity : 0;
    u32 BySlot = Caster ? By : SIM_NOBODY;
    float PerStack = DETONATE_PER_STACK + SEARING_HEAT_PER_STACK *
        (float)RoleRank(Slot, PlayerRole_Damage, StrikerTalent_SearingHeat);
    float Damage = DETONATE_DAMAGE + PerStack * (float)Stacks;
    if (Stacks >= SEARING_MOST && RoleRank(Slot, PlayerRole_Damage, StrikerTalent_Overload))
    {
        Damage *= 1.f + OVERLOAD_SHARE;
    }
    v3 Centre = Marked->Position;
    EmitBurst(&AppState->Events, SimBurst_InfernoBlast, (u8)BySlot, Centre);
    EmitSound(&AppState->Events, AssetType_SfxExplosion, Centre);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        v2 Offset = Monster->Position.XY - Centre.XY;
        if (!Monster->IsPresent || Monster->Type != EntityType_Monster || Monster->Hp <= 0.f ||
            (Monster != Marked && Length(Offset) > DETONATE_SPLASH_RADIUS))
        {
            continue;
        }
        hit Hit = {Monster == Marked ? Damage : DETONATE_SPLASH_SHARE * Damage,
                   120.f, 90.f, 90.f, 0.f, SimBurst_Count, StatusEffect_None, 0.f};
        v2 Away = LengthSq(Offset) > 1.f ? DirectionTo(Offset) : V2(1.f, 0.f);
        ApplyHit(AppState, World, Monster, &Hit, Away, Caster, BySlot);
    }
}

// NOTE(zoubir): once a tick, before the marks fade (UpdateFoeMarks):
// every Searing mark burns its monster, and one about to run out explodes
internal void
UpdateSearing(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < MAX_FOE_MARKS; Index++)
    {
        foe_mark *Mark = &Run->Marks[Index];
        world_entity *Monster = Mark->Stacks ?
            FindMonsterBySerial(World, Mark->Slot, Mark->Serial) : 0;
        if (!Monster || Monster->Hp <= 0.f)
        {
            continue;
        }
        u32 By = Mark->SearBy < MAX_PLAYERS ? Mark->SearBy : 0;
        player_slot *Slot = &AppState->Players[By];
        Mark->BurnTimer += DeltaTime;
        if (Mark->BurnTimer >= SEARING_TICK)
        {
            Mark->BurnTimer -= SEARING_TICK;
            float PerStack = SEARING_BURN_PER_STACK + SEARING_HEAT_BURN *
                (float)RoleRank(Slot, PlayerRole_Damage, StrikerTalent_SearingHeat);
            DamageEntity(AppState, World, Monster, PerStack * (float)Mark->Stacks * SEARING_TICK,
                         Slot->Entity);
        }
        if (Monster->IsPresent && Monster->Hp > 0.f && Mark->Seconds - DeltaTime <= 0.f)
        {
            u32 Stacks = Mark->Stacks;
            Mark->Stacks = 0;
            Mark->Seconds = 0.f;
            Mark->BurnTimer = 0.f;
            ExplodeSearing(AppState, Monster, Stacks, By);
        }
    }
}

// NOTE(zoubir): once a tick: a Fireguard runs out with its time, and what
// is left of it goes out as a striker's ClassMeter
internal void
UpdateFireguards(app_state *AppState, float DeltaTime)
{
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->FireguardSeconds = Maximum(0.f, Slot->FireguardSeconds - DeltaTime);
        if (Slot->FireguardSeconds <= 0.f)
        {
            Slot->FireguardAbsorb = 0.f;
        }
        if (Slot->Role == PlayerRole_Damage)
        {
            Slot->ClassMeter = (u8)Minimum(255.f, Slot->FireguardAbsorb + 0.99f);
        }
    }
}

// NOTE(zoubir): from DungeonScaleDamage: a striker's Fireguard takes what
// it can of Damage on Target; returns the rest
internal float
FireguardTakes(app_state *AppState, player_slot *Slot, world_entity *Target, float Damage)
{
    if (Slot->FireguardAbsorb <= 0.f || Damage <= 0.f)
    {
        return Damage;
    }
    float Absorbed = Minimum(Damage, Slot->FireguardAbsorb);
    Slot->FireguardAbsorb -= Absorbed;
    if (Slot->FireguardAbsorb <= 0.f)
    {
        Slot->FireguardAbsorb = 0.f;
        Slot->FireguardSeconds = 0.f;
        EmitBurst(&AppState->Events, SimBurst_WardBreak, (u8)Target->PlayerIndex, ChestOf(Target));
    }
    return Damage - Absorbed;
}

// NOTE(zoubir): returns whether the key cast. Meteor and Giant Fireball
// only start their cast here (FinishRoleCast fires them), which a client
// predicting its own player does too, without the sound the server sends
internal bool32
CastStrikerKey(app_state *AppState, player_slot *Slot, world_entity *Player, u32 Key)
{
    u8 SlotIndex = (u8)Player->PlayerIndex;
    switch(Key)
    {
        case 0:
        case 1:
        {
            Slot->RoleCastPoint = AimPoint(Player);
            StartPlayerCast(Player, Key == 0 ? PlayerSpell_Meteor : PlayerSpell_GiantFireball,
                            Player->Aim);
            if (!Slot->Predicted)
            {
                EmitSound(&AppState->Events, AssetType_SfxMeteorCast, Player->Position);
            }
        } break;

        case 2:
        {
            Slot->FireguardAbsorb = FIREGUARD_ABSORB;
            Slot->FireguardSeconds = FIREGUARD_SECONDS;
            Slot->ClassMeter = (u8)FIREGUARD_ABSORB;
            EmitSound(&AppState->Events, AssetType_SfxCombustion, Player->Position);
            EmitBurst(&AppState->Events, SimBurst_InfernoCast, SlotIndex, ChestOf(Player),
                      ATan2(Player->Aim.Y, Player->Aim.X));
        } break;

        case 3:
        {
            Slot->CombustSeconds = COMBUSTION_SECONDS;
            EmitSound(&AppState->Events, AssetType_SfxCombustion, Player->Position);
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

// NOTE(zoubir): a Giant Fireball leaves Player's hand toward Point, or
// along Direction when Player stands on Point; nothing when all are in
// flight
internal void
LaunchGiantFireball(app_state *AppState, world_entity *Player, v2 Point, v2 Direction)
{
    dungeon_run *Run = AppState->Dungeon;
    world *World = &AppState->World;
    v2 ToPoint = Point - Player->Position.XY;
    v2 Dir = LengthSq(ToPoint) > Square(24.f) ? DirectionTo(ToPoint) :
        LengthSq(Direction) > 0.f ? DirectionTo(Direction) : V2(1.f, 0.f);
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
            EmitSound(&AppState->Events, AssetType_SfxGiantFireball, Ball->Position);
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
        LaunchGiantFireball(AppState, Player, Slot->RoleCastPoint, Player->CastingDirection);
    }
    else
    {
        FinishClassCast(AppState, Slot, Player, Spell);
    }
}

// NOTE(zoubir): Damage to every living monster within Radius of Centre,
// by the player in slot By; with Stacks, a full hit with its shove and
// that many Searing stacks, else a burn that keeps the monster's mark
// alive. Each one is stunned StunSeconds and slowed SlowSeconds, when
// above 0
internal void
BurnAround(app_state *AppState, world *World, v3 Centre, float Radius, u32 By,
           float Damage, u32 Stacks, float StunSeconds = 0.f, float SlowSeconds = 0.f)
{
    world_entity *Caster = AppState->Players[By].Entity;
    hit Hit = {Damage, 160.f, 140.f, 140.f, StunSeconds, SimBurst_Count, StatusEffect_Burning, 1.f};
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
        if (SlowSeconds > 0.f)
        {
            ApplyStatus(Monster, StatusEffect_Slowed, SlowSeconds);
        }
        if (Stacks)
        {
            AddSearing(Run, World, Monster, Stacks, By);
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

// NOTE(zoubir): once a tick: Searing burning, then meteors falling and
// the ground burning
internal void
UpdateInfernos(app_state *AppState, dungeon_run *Run, float DeltaTime)
{
    world *World = &AppState->World;
    UpdateSearing(AppState, Run, DeltaTime);
    UpdateFoeMarks(Run, World, DeltaTime);
    UpdateFireguards(AppState, DeltaTime);
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
                           INFERNO_DAMAGE, SEARING_METEOR_STACKS);
                EmitBurst(&AppState->Events, SimBurst_InfernoBlast, (u8)Zone->By, Zone->Position);
                EmitSound(&AppState->Events, AssetType_SfxExplosion, Zone->Position);
            }
        }
        else if (Zone->Seconds > 0.f)
        {
            Zone->Seconds = Maximum(0.f, Zone->Seconds - DeltaTime);
            Zone->TickTimer += DeltaTime;
            if (Zone->TickTimer >= INFERNO_BURN_TICK)
            {
                Zone->TickTimer -= INFERNO_BURN_TICK;
                float Slow = MOLTEN_GROUND_SLOW_SECONDS * (float)RoleRank(
                    &AppState->Players[Zone->By], PlayerRole_Damage, StrikerTalent_MoltenGround);
                BurnAround(AppState, World, Zone->Position, Zone->Radius, Zone->By,
                           INFERNO_BURN_PER_SECOND * INFERNO_BURN_TICK, 0, 0.f, Slow);
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
            player_slot *Caster = &AppState->Players[Ball->By];
            if (Caster->Entity)
            {
                float Stun = RoleRank(Caster, PlayerRole_Damage, StrikerTalent_Cataclysm) ?
                    CATACLYSM_STUN_SECONDS : 0.f;
                BurnAround(AppState, World, Ball->Position, GIANT_FIREBALL_RADIUS, Ball->By,
                           GIANT_FIREBALL_DAMAGE, SEARING_GIANT_STACKS, Stun);
            }
            EmitBurst(&AppState->Events, SimBurst_GiantFireballBlast, (u8)Ball->By, Ball->Position);
            EmitSound(&AppState->Events, AssetType_SfxExplosion, Ball->Position);
        }
    }
}

// NOTE(zoubir): from DungeonScaleDamage: a hit by Attacker that is about
// to kill Target; no role talent uses it now
internal void
OnRoleKill(player_slot *Attacker)
{
}
