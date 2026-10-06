/* Area abilities: a table of player abilities that all do the same thing
   with different numbers. After its cast (player_casts.cpp: the player
   is slowed, shows the cast pose and keeps the aim it pressed with),
   every monster and other player in an area is hit: damage, a shove away
   from the area's centre or along the aim, a kick into the air, and a
   stun. A new one is a row in PlayerAreaAbilities (and its name in
   player_area), its spell in player_casts.cpp, a button in player.h and
   a key in client/action_keys.cpp. */

struct player_area_ability
{
    u32 Button;
    // NOTE(zoubir): its wind-up (player_casts.cpp), PlayerSpell_None for
    // one that never starts from its key
    player_spell Spell;
    // NOTE(zoubir): seconds before the next use, counted from the press
    float Cooldown;
    // NOTE(zoubir): the area: a circle of Radius whose centre is Reach
    // along the aim; ConeCos > -1 keeps only targets within that angle of
    // the aim, seen from the player
    float Reach;
    float Radius;
    float ConeCos;
    // NOTE(zoubir): what each target in it takes (sim/hit.cpp)
    hit Hit;
    // NOTE(zoubir): what clients draw when it lands (events.h), and over
    // the area while it is cast so it can be seen coming (SimBurst_Count
    // for none)
    sim_burst Burst;
    sim_burst Telegraph;
    // NOTE(zoubir): the shove pulls toward the area's centre instead of
    // throwing away from the player
    bool32 Pull;
};

// NOTE(zoubir): the rows of PlayerAreaAbilities, in order
enum player_area
{
    PlayerArea_Shockwave,
    PlayerArea_Push,
    PlayerArea_Launch,
    PlayerArea_Slam,
    PlayerArea_FrostNova,
    PlayerArea_GravityWell,
    PlayerArea_Count
};

global_variable player_area_ability PlayerAreaAbilities[PlayerArea_Count] =
{
    // NOTE(zoubir): Shockwave (E): everything within 120 units takes 40
    // and is thrown away from the player. The areas are sized against the
    // sword's reach (64, entity.h): each covers well past it
    {PlayerButton_Shockwave, PlayerSpell_Shockwave, 4.f, 0.f, 120.f, -1.f, {40.f, 500.f, 0.f, 200.f, 0.f, SimBurst_Count},
     SimBurst_ShockwaveRing, SimBurst_Count},
    // NOTE(zoubir): Push (R): a quick wide cone that throws a crowd off
    // the player and apart, out of each other's way. The lift keeps a
    // body off the ground for about 0.4 s, where a stunned monster keeps
    // most of its speed (THROWN_AIR_FRICTION): the lightest walker flies
    // about 200 units, where a shove along the ground stopped it in 75. The
    // stun outlasts the flight, so a wall it meets is a slam (impacts.cpp),
    // and it walks back dazed, slowed until 1.6 s after the hit
    {PlayerButton_Push, PlayerSpell_Push, 2.5f, 0.f, 150.f, 0.34f,
     {10.f, 900.f, 220.f, 0.f, 0.6f, SimBurst_Count, StatusEffect_Slowed, 1.6f},
     SimBurst_PushCone, SimBurst_PushMark},
    // NOTE(zoubir): Launch (A): a ground burst at the aim that throws
    // everything in it into the air (88 units, almost a second under
    // monster gravity) and stuns it until well after it lands. A circle of
    // 75 whose middle is 90 out, so it reaches from the player's feet
    {PlayerButton_Launch, PlayerSpell_Launch, 5.f, 90.f, 75.f, -1.f, {20.f, 60.f, 420.f, 420.f, 1.6f, SimBurst_Count},
     SimBurst_LaunchColumn, SimBurst_LaunchMark},
    // NOTE(zoubir): Slam: no key of its own; the slam's dive
    // (movement_abilities.cpp) fires it where the player lands. Everything
    // within 110 units is thrown out and up and stunned
    {0, PlayerSpell_None, 0.f, 0.f, 110.f, -1.f, {25.f, 380.f, 260.f, 260.f, 0.9f, SimBurst_Count},
     SimBurst_SlamRing, SimBurst_Count},
    // NOTE(zoubir): Frost Nova (G): everything within 130 units is frozen
    // in place for 0.9 s, then slowed for 2.5 s. No damage, so in a duel
    // it sets up the kill rather than making it
    {PlayerButton_FrostNova, PlayerSpell_FrostNova, 9.f, 0.f, 130.f, -1.f,
     {0.f, 60.f, 0.f, 0.f, 0.9f, SimBurst_Count, StatusEffect_Slowed, 2.5f},
     SimBurst_FrostNova, SimBurst_Count},
    // NOTE(zoubir): Gravity Well (T): a circle of 120 whose middle is 170
    // out along the aim; everything in it is pulled to the middle and held
    // for half a second, bunched up for a fireball
    {PlayerButton_GravityWell, PlayerSpell_GravityWell, 10.f, 170.f, 120.f, -1.f,
     {0.f, 1100.f, 0.f, 0.f, 0.5f, SimBurst_Count},
     SimBurst_GravityWell, SimBurst_GravityMark, true},
};
#define PLAYER_AREA_ABILITY_COUNT PlayerArea_Count
static_assert(PlayerArea_Count <= PLAYER_AREA_ABILITY_SLOTS, "one cooldown each");

// NOTE(zoubir): whether a player's attack can hit Target: a monster or
// another living player
inline bool32
IsHitTarget(world_entity *Target, world_entity *Player)
{
    bool32 Result = Target->IsPresent && Target != Player &&
        !IsDeadPlayer(Target) &&
        (Target->Type == EntityType_Monster || Target->Type == EntityType_Player);
    return Result;
}

// NOTE(zoubir): the area's centre, on the ground under the player
inline v3
AreaCentre(world_entity *Player, player_area_ability *Ability, v2 Aim)
{
    v3 Result = Player->Position;
    Result.XY += Ability->Reach * Aim;
    Result.Z = Player->GroundZ;
    return Result;
}

// NOTE(zoubir): the hit, at the end of the cast or on landing, and what
// clients see and hear of it. Returns how many it hit
internal u32
FireAreaAbility(app_state *AppState, world *World, world_entity *Player,
                player_area_ability *Ability, v2 Aim)
{
    v3 Centre = AreaCentre(Player, Ability, Aim);
    u32 HitCount = 0;
    // NOTE(zoubir): the ability's level stuns and slows longer and shoves
    // harder (sim/progression/talents.cpp); the slam's row has no key of
    // its own, its level is the slam's
    u32 Button = Ability->Button ? Ability->Button : (u32)PlayerButton_Slam;
    hit LeveledHit = AbilityLevelHit(AppState, Player, Button, &Ability->Hit);
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Target = &World->Entities[EntityIndex];
        if (!IsHitTarget(Target, Player) || IsJumpingClear(Target))
        {
            continue;
        }
        v2 Away = NormalizeOr(Target->Position.XY - Player->Position.XY, Aim);
        float FromCentre = Length(Target->Position.XY - Centre.XY);
        if (FromCentre > Ability->Radius ||
            DotProduct(Away, Aim) < Ability->ConeCos)
        {
            continue;
        }
        if (Ability->Pull)
        {
            // NOTE(zoubir): a shove that ends near the middle: the drag
            // takes most of it within a few units, so the speed scales
            // with how far out the target stands
            Away = NormalizeOr(Centre.XY - Target->Position.XY, -Aim);
            Away *= Minimum(1.f, FromCentre / Ability->Radius);
        }

        HitCount++;
        ApplyHit(AppState, World, Target, &LeveledHit, Away,
                 Player, Player->PlayerIndex);
    }
    EmitBurst(&AppState->Events, Ability->Burst, (u8)Player->PlayerIndex,
              Centre, ATan2(Aim.Y, Aim.X));
    EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
    return HitCount;
}

// NOTE(zoubir): an area ability that fires when the player next lands
// (PendingLandArea, its row + 1); called after every player move
internal void
FireAreaOnLanding(app_state *AppState, world *World, world_entity *Player)
{
    // NOTE(zoubir): anything that sends the player up again (an air dash,
    // a jump, being launched) ends the dive, and with it the hit; it used
    // to go off on whatever landing came next
    if (Player->Velocity.Z > 0.f)
    {
        Player->PendingLandArea = 0;
    }
    if (!Player->PendingLandArea || !IsOnGround(Player))
    {
        return;
    }
    player_area_ability *Ability =
        &PlayerAreaAbilities[Player->PendingLandArea - 1];
    Player->PendingLandArea = 0;
    if (!IsPredictedPlayer(AppState, Player))
    {
        FireAreaAbility(AppState, World, Player, Ability, GetPlayerAim(Player));
    }
}

// NOTE(zoubir): every area ability's button (client/prediction.cpp predicts
// their casts)
internal u32
PlayerAreaButtons()
{
    u32 Result = 0;
    for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
    {
        Result |= PlayerAreaAbilities[Index].Button;
    }
    return Result;
}

internal void
UseAreaAbilities(app_state *AppState, world_entity *Player,
                 player_input *Input, float DeltaTime)
{
    for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
    {
        Player->AreaCooldowns[Index] =
            Maximum(0.f, Player->AreaCooldowns[Index] - DeltaTime);
    }
    if (IsPlayerCasting(Player))
    {
        return;
    }
    for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
    {
        player_area_ability *Ability = &PlayerAreaAbilities[Index];
        if (Ability->Spell == PlayerSpell_None ||
            !WasPressed(Input, Ability->Button) ||
            !CanUseEarly(Player->AreaCooldowns[Index]))
        {
            continue;
        }
        Player->AreaCooldowns[Index] += Ability->Cooldown *
            PlayerCooldownScale(AppState, Player, Ability->Button);
        v2 Aim = GetPlayerAim(Player);
        StartPlayerCast(Player, Ability->Spell, Aim);
        // NOTE(zoubir): a client predicting its own player runs the cast
        // (the slowdown, the pose, the cooldown) but leaves the hit and
        // its sounds and bursts to the server, which sends them with its
        // snapshot
        if (!IsPredictedPlayer(AppState, Player))
        {
            EmitSound(&AppState->Events, AssetType_FireCast, Player->Position);
            EmitBurst(&AppState->Events, SimBurst_CastGather,
                      (u8)Player->PlayerIndex, Player->Position);
            if (Ability->Telegraph != SimBurst_Count)
            {
                EmitBurst(&AppState->Events, Ability->Telegraph,
                          (u8)Player->PlayerIndex,
                          AreaCentre(Player, Ability, Aim), ATan2(Aim.Y, Aim.X));
            }
        }
        break;
    }
}

// NOTE(zoubir): Spell's cast is done (player_update/casts.cpp): its row
// hits along the aim the cast started with
internal void
FireAreaCast(app_state *AppState, world *World, world_entity *Player,
             player_spell Spell)
{
    for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
    {
        player_area_ability *Ability = &PlayerAreaAbilities[Index];
        if (Ability->Spell == Spell)
        {
            FireAreaAbility(AppState, World, Player, Ability,
                            Player->CastingDirection);
        }
    }
}
