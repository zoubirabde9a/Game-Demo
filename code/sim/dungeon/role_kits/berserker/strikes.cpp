/* Berserker strikes (role_kits/berserker.cpp): the axe's blows. Cleave and
   Whirlwind hit every foe they catch, harder for each other one with
   Sweeping Strikes; Execute spends the Rage, and heals with Bloodthirst.
   Each reaches only foes in the
   Berserker's room, so no blow wakes the room behind a gate. */

// NOTE(zoubir): a living monster in Room
inline bool32
IsBerserkerFoe(world *World, world_entity *Monster, u32 Room)
{
    bool32 Result = Monster->IsPresent && Monster->Type == EntityType_Monster &&
        Monster->Hp > 0.f && RoomAtPosition(World, Monster->Position.XY) == Room;
    return Result;
}

// NOTE(zoubir): whether any of Monster's body is within Reach of From and
// HalfAngle of Dir, counting its width as the sword does (IsInSwordSlice)
internal bool32
IsInBerserkerArc(v2 From, v2 Dir, float Reach, float HalfAngle, world_entity *Monster)
{
    v2 To = Monster->Position.XY - From;
    float Radius = Monster->Collision ? Monster->Collision->TotalVolume.HalfDims.X : 0.f;
    float Distance = Length(To);
    if (Distance - Radius > Reach)
    {
        return false;
    }
    if (Distance <= Radius + 1.f)
    {
        return true;
    }
    float Cos = DotProduct(To, Dir) / Distance;
    float Angle = acosf(Maximum(-1.f, Minimum(1.f, Cos)));
    float Widen = asinf(Minimum(1.f, Radius / Distance));
    bool32 Result = Angle <= HalfAngle + Widen;
    return Result;
}

// NOTE(zoubir): Sweeping Strikes: the share more a blow that caught Count
// foes deals each
inline float
SweepingScale(player_slot *Slot, u32 Count)
{
    float Result = 1.f;
    if (Count > 1 && RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_SweepingStrikes))
    {
        Result += SWEEPING_PER_FOE * (float)Minimum(Count - 1, (u32)SWEEPING_MOST);
    }
    return Result;
}

// NOTE(zoubir): Damage and Shove to every foe within Reach and HalfAngle
// of Dir from the Berserker (HalfAngle Pi for all round), the one most
// squarely in front taking all of it and the rest Splash of it; returns
// how many
internal u32
StrikeAround(app_state *AppState, player_slot *Slot, world_entity *Player, v2 Dir,
             float Reach, float HalfAngle, float Damage, float Shove, float Splash = 1.f)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    world_entity *Caught[32];
    u32 Count = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount && Count < ArrayCount(Caught);
        EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (IsBerserkerFoe(World, Monster, Room) &&
            IsInBerserkerArc(Player->Position.XY, Dir, Reach, HalfAngle, Monster))
        {
            Caught[Count++] = Monster;
        }
    }
    u32 Main = 0;
    float Best = -2.f;
    for(u32 Index = 0; Index < Count; Index++)
    {
        v2 To = Caught[Index]->Position.XY - Player->Position.XY;
        float Facing = DotProduct(NormalizeOr(To, Dir), Dir) - 0.002f * Length(To);
        if (Facing > Best)
        {
            Best = Facing;
            Main = Index;
        }
    }
    float Full = Damage * SweepingScale(Slot, Count);
    for(u32 Index = 0; Index < Count; Index++)
    {
        hit Hit = {Index == Main ? Full : Splash * Full, Shove, 0.f, 0.f, 0.f, SimBurst_Count};
        v2 Away = NormalizeOr(Caught[Index]->Position.XY - Player->Position.XY, Dir);
        ApplyHit(AppState, World, Caught[Index], &Hit, Away, Player, Player->PlayerIndex);
    }
    return Count;
}

// NOTE(zoubir): which way a swing goes: at the monster under the cursor
// (player_input.Target) when it is within Reach of the Berserker, its
// body counting, else along the aim
internal v2
SwingDirection(app_state *AppState, player_slot *Slot, world_entity *Player, float Reach)
{
    v2 Result = GetPlayerAim(Player);
    world *World = &AppState->World;
    u32 Index = Slot->Input.Target;
    if (Index && Index - 1 < World->EntityCount)
    {
        world_entity *Unit = &World->Entities[Index - 1];
        float Radius = Unit->Collision ? Unit->Collision->TotalVolume.HalfDims.X : 0.f;
        v2 To = Unit->Position.XY - Player->Position.XY;
        if (Unit->IsPresent && Unit->Type == EntityType_Monster && Unit->Hp > 0.f &&
            LengthSq(To) > 1.f && Length(To) - Radius <= Reach)
        {
            Result = DirectionTo(To);
        }
    }
    return Result;
}

// NOTE(zoubir): the right click: a swing through the arc in front, each one
// the other way round from the last
internal void
Cleave(app_state *AppState, world *World, player_slot *Slot, world_entity *Player)
{
    bool32 Sweeping = RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_SweepingStrikes) > 0;
    float Reach = CLEAVE_REACH * (Sweeping ? SWEEPING_REACH : 1.f);
    v2 Dir = SwingDirection(AppState, Slot, Player, Reach);
    // NOTE(zoubir): a wider swing tells clients so in the burst's height,
    // which reaches them whole (they do not know the talents of others)
    v3 At = Player->Position;
    At.Z += Sweeping ? BERSERKER_WIDE_CLEAVE : 0.f;
    berserker_burst Burst = Slot->Berserker.CleaveBack ? BerserkerBurst_CleaveBack :
        BerserkerBurst_Cleave;
    Slot->Berserker.CleaveBack = !Slot->Berserker.CleaveBack;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_BerserkerFirst, Burst),
              (u8)Player->PlayerIndex, At, ATan2(Dir.Y, Dir.X));
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    StrikeAround(AppState, Slot, Player, Dir, Reach, CLEAVE_HALF_ANGLE, CLEAVE_DAMAGE, CLEAVE_SHOVE,
                 CLEAVE_SPLASH);
}

// NOTE(zoubir): one turn of the Whirlwind: everything round the Berserker
internal void
WhirlHit(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    Slot->Berserker.WhirlHits++;
    EmitSound(&AppState->Events, AssetType_SfxSword, Player->Position);
    StrikeAround(AppState, Slot, Player, GetPlayerAim(Player), WHIRLWIND_RADIUS, Pi32,
                 WHIRLWIND_DAMAGE, 40.f);
}

// NOTE(zoubir): the end of Execute's wind-up: the chop lands on the foe it
// was aimed at, or the one nearest in reach, spending all the Rage; with
// nobody there it hits the ground and the Rage stays
internal void
Execute(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    world_entity *Foe = NearestFoe(World, Slot->RoleCastPoint, 60.f, Room, 0, 0);
    if (!Foe || Length(Foe->Position.XY - Player->Position.XY) > EXECUTE_REACH + 30.f)
    {
        Foe = NearestFoe(World, Player->Position.XY, EXECUTE_REACH, Room, 0, 0);
    }
    v2 Dir = GetPlayerAim(Player);
    if (LengthSq(Player->CastingDirection) > 0.f)
    {
        Dir = DirectionTo(Player->CastingDirection);
    }
    v3 Spot = Player->Position;
    Spot.XY += 0.6f * EXECUTE_REACH * Dir;
    if (Foe)
    {
        Dir = NormalizeOr(Foe->Position.XY - Player->Position.XY, Dir);
        bool32 Massacre = RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_Massacre) > 0;
        float Low = Massacre ? MASSACRE_LOW_SHARE : EXECUTE_LOW_SHARE;
        float Rage = (float)BerserkerRage(Slot);
        float Damage = EXECUTE_DAMAGE + EXECUTE_PER_RAGE * Rage;
        if (Foe->MaxHp > 0.f && Foe->Hp < Low * Foe->MaxHp)
        {
            Damage *= EXECUTE_LOW_SCALE;
            EmitSound(&AppState->Events, AssetType_SfxExplosion, Foe->Position);
        }
        hit Hit = {Damage, EXECUTE_SHOVE, 120.f, 160.f, 0.f, SimBurst_Count};
        float Before = Foe->Hp;
        ApplyHit(AppState, World, Foe, &Hit, Dir, Player, Player->PlayerIndex);
        u32 Thirst = RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_Bloodthirst);
        if (Thirst)
        {
            float Share = Thirst >= 2 ? BLOODTHIRST_RANK2_HEAL : BLOODTHIRST_HEAL_SHARE;
            HealPlayer(AppState, Player->PlayerIndex, Player, Share * (Before - Maximum(0.f, Foe->Hp)));
            EmitSound(&AppState->Events, AssetType_SfxHeal, Player->Position);
        }
        Slot->ClassMeter = 0;
        Slot->Berserker.RageCarry = 0.f;
        if (Massacre && Foe->Hp <= 0.f)
        {
            AddRage(Slot, (float)MASSACRE_REFUND);
        }
        Spot = Foe->Position;
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_BerserkerFirst, BerserkerBurst_Execute),
              (u8)Player->PlayerIndex, Spot, ATan2(Dir.Y, Dir.X));
    EmitSound(&AppState->Events, AssetType_SfxShieldSlam, Spot);
}
