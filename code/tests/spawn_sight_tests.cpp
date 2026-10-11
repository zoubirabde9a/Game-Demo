/* Spawn sight tests (sim/monster_abilities/sight.cpp): a monster starts no
   ability on a player who cannot see it. The case that showed it: a
   player standing still at the Old Arena spawn, as one does behind the
   Play screen at launch, shelled by a Bilecaller Toad from off screen and
   left standing in its bile. The world is built as the offline client
   builds it and stepped at the client's 1/30 s frames, under the duel
   rules the client plays. The same goes for Frostbite Keep, where a
   Duskwing placed inside its reach of the spawn flew in and bit. Included
   by sim_tests.cpp, which calls RunSpawnSightTests. */

#define SIGHT_TEST_TICK (1.f / 30.f)

struct sight_world
{
    app_state *AppState;
    memory_arena Arena;
    memory_arena Constants;
    world_entity *Player;
};

// NOTE(zoubir): MapId (the Old Arena unless given) with one player at
// its spawn
internal sight_world
CreateSightWorld(u32 MapId = MapId_Arena)
{
    sight_world Result = {};
    Result.AppState = (app_state *)calloc(1, sizeof(app_state));
    memory_index Size = Megabytes(48);
    InitializeArena(&Result.Arena, (memory_index *)calloc(1, Size), Size);
    InitializeArena(&Result.Constants, (memory_index *)calloc(1, Megabytes(1)), Megabytes(1));
    Result.AppState->World.MapId = MapId;
    InitSimulation(Result.AppState, &Result.Arena, &Result.Constants);
    world *World = &Result.AppState->World;
    Result.Player = AddPlayerToSlot(Result.AppState, World, &Result.Arena, 0,
                                    PlayerSpawnPosition(World, 0));
    return Result;
}

internal void
DestroySightWorld(sight_world *Sight)
{
    free(Sight->AppState);
    free(Sight->Arena.Base);
    free(Sight->Constants.Base);
}

// NOTE(zoubir): a toad Offset from the player, its Bile Barrage ready
internal world_entity *
AddSightToad(sight_world *Sight, v2 Offset)
{
    world *World = &Sight->AppState->World;
    v3 Position = Sight->Player->Position;
    Position.XY += Offset;
    Position = OnGround(World, Position);
    Check(IsSpawnSpotFree(Sight->AppState, World, Position,
                          Sight->AppState->PlayerCollision));
    world_entity *Toad = SpawnMonster(Sight->AppState, World, &Sight->Arena,
                                      Position, MonsterKind_Toad);
    Toad->AbilityCooldowns[0] = 0.f;
    return Toad;
}

internal void
TestStillPlayerAtSpawnIsNotShelledFromOffScreen()
{
    GameRules = DuelRules;
    sight_world Sight = CreateSightWorld();
    world_entity *Player = Sight.Player;
    monster_ability *Barrage = &GetMonsterDef(MonsterKind_Toad)->Abilities[0];
    // NOTE(zoubir): straight north, inside Bile Barrage's reach and past
    // the top of the screen
    float Above = 0.5f * WORLD_VIEW_HEIGHT + 60.f;
    Check(Above < Barrage->MaxRange);
    world_entity *Toad = AddSightToad(&Sight, V2(0.f, -Above));
    Check(!IsInPlayerView(Player, Toad));

    // NOTE(zoubir): the player never moves. Every barrage must start with
    // the toad on screen, and no hit may land before the toad has been
    // seen for a whole windup
    float FullHp = Player->Hp;
    float SeenAt = -1.f;
    float FirstHitAt = -1.f;
    bool32 CastUnseen = false;
    bool32 Cast = false;
    float Seconds = 0.f;
    for(u32 Tick = 0; Tick < 12 * 30; Tick++)
    {
        u32 PhaseBefore = Toad->AbilityPhase;
        SimulateTick(Sight.AppState, &Sight.Arena, SIGHT_TEST_TICK);
        Seconds += SIGHT_TEST_TICK;
        if (SeenAt < 0.f && Toad->IsPresent && IsInPlayerView(Player, Toad))
        {
            SeenAt = Seconds;
        }
        if (PhaseBefore == AbilityPhase_Ready &&
            Toad->AbilityPhase == AbilityPhase_Windup)
        {
            Cast = true;
            CastUnseen |= !IsInPlayerView(Player, Toad);
        }
        if (FirstHitAt < 0.f && Player->Hp < FullHp)
        {
            FirstHitAt = Seconds;
        }
    }
    // NOTE(zoubir): the first seconds after a spawn, where the bug hurt
    Check(FirstHitAt < 0.f || FirstHitAt > 3.f);
    Check(!CastUnseen);
    Check(SeenAt > 0.f);
    Check(FirstHitAt < 0.f || FirstHitAt >= SeenAt + Barrage->Windup - 0.01f);
    // NOTE(zoubir): once on screen the toad is the threat it always was
    Check(Cast);
    DestroySightWorld(&Sight);
    GameRules = ClassicRules;
}

// NOTE(zoubir): the screen is wider than tall, so the same reach to the
// side is in view and the toad fires at once
internal void
TestToadInViewToTheSideStillShells()
{
    GameRules = DuelRules;
    sight_world Sight = CreateSightWorld();
    monster_ability *Barrage = &GetMonsterDef(MonsterKind_Toad)->Abilities[0];
    float Aside = Barrage->MaxRange - 20.f;
    world_entity *Toad = AddSightToad(&Sight, V2(-Aside, 0.f));
    Check(IsInPlayerView(Sight.Player, Toad));
    SimulateTick(Sight.AppState, &Sight.Arena, SIGHT_TEST_TICK);
    Check(Toad->AbilityPhase == AbilityPhase_Windup);
    DestroySightWorld(&Sight);
    GameRules = ClassicRules;
}

// NOTE(zoubir): true when no monster stands within its own aggro range of
// any slot's spawn point
internal bool32
NoMonsterReachesASpawn(world *World)
{
    for(u32 EntityIndex = 0; EntityIndex < World->EntityCount; EntityIndex++)
    {
        world_entity *Monster = &World->Entities[EntityIndex];
        if (Monster->IsPresent && Monster->Type == EntityType_Monster &&
            !IsFarFromSpawnPoints(World, Monster->Position.XY,
                                  GetMonsterDef(Monster->MonsterKind)->AggroRange))
        {
            return false;
        }
    }
    return true;
}

// NOTE(zoubir): the keep is filled before anyone joins; its monsters must
// still start out of reach of every spawn point, at launch and each time
// the map is built again (a round restart or a map change), and a player
// standing still at the spawn takes no hit in the first seconds
internal void
TestStillPlayerAtKeepSpawnIsNotBitten()
{
    GameRules = DuelRules;
    sight_world Sight = CreateSightWorld(MapId_Keep);
    app_state *AppState = Sight.AppState;
    world *World = &AppState->World;
    world_entity *Player = Sight.Player;
    AddFamiliar(AppState, World, &Sight.Arena, Player);
    Check(CountLiveMonsters(World) == MapMonsterPopulation(World));
    Check(NoMonsterReachesASpawn(World));

    float FullHp = Player->Hp;
    for(u32 Tick = 0; Tick < 8 * 30; Tick++)
    {
        SimulateTick(AppState, &Sight.Arena, SIGHT_TEST_TICK);
    }
    Check(Player->Hp == FullHp);

    for(u32 Round = 1; Round <= 12; Round++)
    {
        RebuildWorldForMap(AppState, &Sight.Arena, MapId_Keep);
        AppState->Monsters->Series = Seed(Round * 7919u);
        FillMonsterPopulation(AppState, World, &Sight.Arena, AppState->Monsters);
        Check(CountLiveMonsters(World) == MapMonsterPopulation(World));
        Check(NoMonsterReachesASpawn(World));
    }
    DestroySightWorld(&Sight);
    GameRules = ClassicRules;
}

internal void
RunSpawnSightTests()
{
    RUN(TestStillPlayerAtSpawnIsNotShelledFromOffScreen);
    RUN(TestToadInViewToTheSideStillShells);
    RUN(TestStillPlayerAtKeepSpawnIsNotBitten);
}
