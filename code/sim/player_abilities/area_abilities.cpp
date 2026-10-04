/* Area abilities: a table of player abilities that all do the same thing
   with different numbers. After a short cast (the player is slowed and
   shows the cast pose), every monster and other player in an area is hit:
   damage, a shove away from the area's centre or along the aim, a kick
   into the air, and a stun. Shockwave is the instant one: no cast, a ring
   around the player. A new one is a row in PlayerAreaAbilities (and its
   name in player_area), a button in player.h and a key in
   client/action_keys.cpp; no code.

   The cast can be cut short by a dash or a blink, which keeps the
   cooldown spent. While it lasts the player keeps the aim it started
   with, so the hit lands where the cast pose points. */

struct player_area_ability
{
    u32 Button;
    // NOTE(zoubir): seconds of wind-up before the hit, and before the next
    // use (counted from the press)
    float CastTime;
    float Cooldown;
    // NOTE(zoubir): the area: a circle of Radius whose centre is Reach
    // along the aim; ConeCos > -1 keeps only targets within that angle of
    // the aim, seen from the player
    float Reach;
    float Radius;
    float ConeCos;
    // NOTE(zoubir): what each target in it takes (hits.cpp)
    player_hit Hit;
    // NOTE(zoubir): what clients draw when it lands (events.h), and over
    // the area while it is cast so it can be seen coming (SimBurst_Count
    // for none)
    sim_burst Burst;
    sim_burst Telegraph;
};

// NOTE(zoubir): walk speed while casting
#define PLAYER_AREA_CAST_MOVE_SCALE 0.35f

// NOTE(zoubir): the rows of PlayerAreaAbilities, in order
enum player_area
{
    PlayerArea_Shockwave,
    PlayerArea_Push,
    PlayerArea_Launch,
    PlayerArea_Slam,
    PlayerArea_Count
};

global_variable player_area_ability PlayerAreaAbilities[PlayerArea_Count] =
{
    // NOTE(zoubir): Shockwave (E): at once, everything within 90 units
    // takes 40 and is thrown away from the player
    {PlayerButton_Shockwave, 0.f, 4.f, 0.f, 90.f, -1.f, {40.f, 500.f, 0.f, 200.f, 0.f, SimBurst_Count},
     SimBurst_ShockwaveRing, SimBurst_Count},
    // NOTE(zoubir): Push (R): a quick wide cone that throws a crowd off
    // the player and apart, out of each other's way
    {PlayerButton_Push, 0.12f, 2.5f, 0.f, 110.f, 0.34f, {10.f, 750.f, 0.f, 0.f, 0.3f, SimBurst_Count},
     SimBurst_PushCone, SimBurst_PushMark},
    // NOTE(zoubir): Launch (A): a ground burst at the aim that throws
    // everything in it into the air (88 units, almost a second under
    // monster gravity) and stuns it until well after it lands
    {PlayerButton_Launch, 0.3f, 5.f, 70.f, 55.f, -1.f, {20.f, 60.f, 420.f, 420.f, 1.6f, SimBurst_Count},
     SimBurst_LaunchColumn, SimBurst_LaunchMark},
    // NOTE(zoubir): Slam: no key of its own; the slam's dive
    // (movement_abilities.cpp) fires it where the player lands. Everything
    // within 80 units is thrown out and up and stunned
    {0, 0.f, 0.f, 0.f, 80.f, -1.f, {25.f, 380.f, 260.f, 260.f, 0.9f, SimBurst_Count},
     SimBurst_SlamRing, SimBurst_Count},
};
#define PLAYER_AREA_ABILITY_COUNT PlayerArea_Count
static_assert(PlayerArea_Count <= PLAYER_AREA_ABILITY_SLOTS, "one cooldown each");

// NOTE(zoubir): the hit, at the end of the cast
internal u32
FireAreaAbility(app_state *AppState, world *World, world_entity *Player,
                player_area_ability *Ability, v2 Aim)
{
    v2 Centre = Player->Position.XY + Ability->Reach * Aim;
    u32 HitCount = 0;
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Target = &World->Entities[EntityIndex];
        if (!Target->IsPresent || Target == Player || IsDeadPlayer(Target) ||
            (Target->Type != EntityType_Monster &&
             Target->Type != EntityType_Player))
        {
            continue;
        }
        v2 FromPlayer = Target->Position.XY - Player->Position.XY;
        float FromPlayerLength = Length(FromPlayer);
        v2 Away = FromPlayerLength > 0.001f ?
            FromPlayer * (1.f / FromPlayerLength) : Aim;
        if (Length(Target->Position.XY - Centre) > Ability->Radius ||
            DotProduct(Away, Aim) < Ability->ConeCos)
        {
            continue;
        }

        HitCount++;
        ApplyPlayerHit(AppState, World, Target, &Ability->Hit, Away,
                       Player->PlayerIndex, Player);
    }
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
    if (AppState->Players[Player->PlayerIndex].Predicted)
    {
        return;
    }
    v2 Aim = GetPlayerAim(Player);
    FireAreaAbility(AppState, World, Player, Ability, Aim);
    EmitBurst(&AppState->Events, Ability->Burst, (u8)Player->PlayerIndex,
              Player->Position);
    EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
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

inline bool32
IsCastingAreaAbility(world_entity *Player)
{
    bool32 Result = Player->CastingArea > 0;
    return Result;
}

// NOTE(zoubir): a dash or blink cuts the cast; the cooldown stays spent
inline void
CancelAreaCast(world_entity *Player)
{
    Player->CastingArea = 0;
}

internal void
UseAreaAbilities(app_state *AppState, world *World, world_entity *Player,
                 player_input *Input, float DeltaTime, player_tick *Tick)
{
    for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
    {
        Player->AreaCooldowns[Index] =
            Maximum(0.f, Player->AreaCooldowns[Index] - DeltaTime);
    }

    // NOTE(zoubir): a client predicting its own player runs the cast (the
    // slowdown, the pose, the cooldown) but leaves the hit and its sounds
    // and bursts to the server, which sends them with its snapshot
    bool32 Authoritative = !AppState->Players[Player->PlayerIndex].Predicted;
    if (!IsCastingAreaAbility(Player))
    {
        for(u32 Index = 0; Index < PLAYER_AREA_ABILITY_COUNT; Index++)
        {
            player_area_ability *Ability = &PlayerAreaAbilities[Index];
            if (WasPressed(Input, Ability->Button) &&
                Player->AreaCooldowns[Index] <= 0.f)
            {
                Player->AreaCooldowns[Index] = Ability->Cooldown;
                Player->CastingArea = Index + 1;
                Player->AreaCastLeft = Ability->CastTime;
                Player->CastingDirection = GetPlayerAim(Player);
                Player->AnimationState.SlotIndex = 0;
                if (!Authoritative)
                {
                    break;
                }
                EmitSound(&AppState->Events, AssetType_FireCast,
                          Player->Position);
                if (Ability->CastTime > 0.f)
                {
                    EmitBurst(&AppState->Events, SimBurst_CastGather,
                              (u8)Player->PlayerIndex, Player->Position);
                }
                if (Ability->Telegraph != SimBurst_Count)
                {
                    v2 Aim = Player->CastingDirection;
                    v3 Centre = Player->Position;
                    Centre.XY += Ability->Reach * Aim;
                    Centre.Z = 0.f;
                    EmitBurst(&AppState->Events, Ability->Telegraph,
                              (u8)Player->PlayerIndex, Centre,
                              ATan2(Aim.Y, Aim.X));
                }
                break;
            }
        }
    }
    if (!IsCastingAreaAbility(Player))
    {
        return;
    }

    if (Player->AreaCastLeft > 0.f)
    {
        Tick->Acceleration *= PLAYER_AREA_CAST_MOVE_SCALE;
    }
    Player->AreaCastLeft -= DeltaTime;
    if (Player->AreaCastLeft <= 0.f)
    {
        player_area_ability *Ability =
            &PlayerAreaAbilities[Player->CastingArea - 1];
        Player->CastingArea = 0;
        if (!Authoritative)
        {
            return;
        }
        v2 Aim = Player->CastingDirection;
        FireAreaAbility(AppState, World, Player, Ability, Aim);
        v3 Centre = Player->Position;
        Centre.XY += Ability->Reach * Aim;
        Centre.Z = 0.f;
        EmitBurst(&AppState->Events, Ability->Burst, (u8)Player->PlayerIndex,
                  Centre, ATan2(Aim.Y, Aim.X));
        EmitSound(&AppState->Events, AssetType_Dash, Player->Position);
    }
}
