/* Berserker Leap (role_kits/berserker.cpp): A throws the Berserker up and
   over to the cursor in a high arc, LEAP_SECONDS long, and the landing
   slams everything within LEAP_RADIUS: a hit, a stun and a shove out.
   With Shattering Leap (the capstone) the slam also sunders what it
   strikes, so the party hits it harder for SHATTERING_LEAP_SECONDS.

   The flight is the body's own physics: the jump's speed up, and across
   the speed that reaches the spot in that time, the air barely slowing it
   (LongJump, as a long jump). While BERSERKER_FLAG_LEAPING is up the
   walk keys and the drag leave the speed across alone (ClassCarriesPlayer,
   class_kits.cpp), so the flight is a plain arc a predicting client flies
   the same way; each tick the speed across is still set again toward the
   spot, so a wall that slows it cannot throw it off. It slams where it comes down; one that never does (over a pit,
   against a wall that holds it up) gives up after LEAP_MOST_SECONDS. */

// NOTE(zoubir): PLAYER_GRAVITY and IsOnGround are player_abilities/jump.cpp,
// included after the dungeon; the leap must rise by the same gravity
#define LEAP_GRAVITY 1600.f
inline bool32 IsOnGround(world_entity *Player);

internal void
StartLeap(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    v2 To = AimPoint(Player);
    berserker_slot *State = &Slot->Berserker;
    State->Leaping = true;
    State->LeapTo = To;
    State->LeapAge = 0.f;
    Slot->ClassFlags |= BERSERKER_FLAG_LEAPING;
    Player->Velocity.XY = (1.f / LEAP_SECONDS) * (To - Player->Position.XY);
    Player->Velocity.Z = 0.5f * LEAP_GRAVITY * LEAP_SECONDS;
    Player->LongJump = true;
    v2 Toward = To - Player->Position.XY;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_BerserkerFirst, BerserkerBurst_Leap),
              (u8)Player->PlayerIndex, V3(To.X, To.Y, Player->GroundZ),
              ATan2(Toward.Y, Toward.X));
    EmitSound(&AppState->Events, AssetType_SfxJump, Player->Position);
}

// NOTE(zoubir): the landing: every foe round the Berserker struck, stunned
// and thrown out; with Shattering Leap each one left alive is sundered
// after the blow (foe_marks.cpp), taking more from the whole party
internal void
LeapSlam(app_state *AppState, player_slot *Slot, world_entity *Player)
{
    world *World = &AppState->World;
    bool32 Shatter = RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_ShatteringLeap) > 0;
    u32 Room = RoomAtPosition(World, Player->Position.XY);
    hit Hit = {LEAP_DAMAGE, LEAP_SHOVE, 160.f, 160.f, LEAP_STUN, SimBurst_Count};
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (!IsBerserkerFoe(World, Monster, Room) ||
            !IsInBerserkerArc(Player->Position.XY, V2(1.f, 0.f), LEAP_RADIUS, Pi32, Monster))
        {
            continue;
        }
        v2 Away = NormalizeOr(Monster->Position.XY - Player->Position.XY, V2(1.f, 0.f));
        ApplyHit(AppState, World, Monster, &Hit, Away, Player, Player->PlayerIndex);
        if (Shatter && AppState->Dungeon && Monster->Hp > 0.f)
        {
            AddSunder(AppState->Dungeon, World, Monster, SHATTERING_LEAP_SECONDS,
                      SHATTERING_LEAP_SHARE);
        }
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_BerserkerFirst, BerserkerBurst_Slam),
              (u8)Player->PlayerIndex, Player->Position);
    EmitSound(&AppState->Events, AssetType_SfxShieldSlam, Player->Position);
}

// NOTE(zoubir): once a tick for each Berserker: a leap in flight steers
// for its spot, then slams where it lands
internal void
UpdateLeap(app_state *AppState, player_slot *Slot, world_entity *Player, float DeltaTime)
{
    berserker_slot *State = &Slot->Berserker;
    if (!State->Leaping)
    {
        return;
    }
    State->LeapAge += DeltaTime;
    if (IsDeadPlayer(Player) || State->LeapAge > LEAP_MOST_SECONDS)
    {
        State->Leaping = false;
        return;
    }
    if (State->LeapAge > 0.05f && IsOnGround(Player))
    {
        State->Leaping = false;
        Player->Velocity.XY = {};
        LeapSlam(AppState, Slot, Player);
        return;
    }
    float Left = Maximum(2.f / 60.f, LEAP_SECONDS - State->LeapAge);
    v2 ToSpot = State->LeapTo - Player->Position.XY;
    Player->Velocity.XY = (1.f / Left) * ToSpot;
    if (LengthSq(ToSpot) < Square(4.f))
    {
        Player->Velocity.XY = {};
    }
    Player->LongJump = true;
}
