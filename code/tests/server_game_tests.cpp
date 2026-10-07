/* Server game tests: what the game behind the server (server/sim_game.cpp,
   server/event_relay.cpp) puts in each player's snapshot: the nearest
   entities first, the sounds near the player and every death, monster
   wind-ups and front-armour facings, and names told apart. No sockets:
   these call the game API directly. Included by server_tests.cpp, which
   calls RunServerGameTests. */

// Two players who pick the same name are told apart: the second becomes
// "Name 2", shortened to fit; a different case counts as the same name.
internal void
TestSameNamesAreToldApart()
{
    static server_game Game;
    GameInit(&Game);
    app_state *AppState = Game.AppState;
    GamePlayerJoined(&Game, 0);
    GamePlayerNamed(&Game, 0, "Gary");
    GamePlayerJoined(&Game, 1);
    GamePlayerNamed(&Game, 1, "gary");
    GamePlayerJoined(&Game, 2);
    GamePlayerNamed(&Game, 2, "Gary");
    Check(strcmp(AppState->Players[0].Name, "Gary") == 0);
    Check(strcmp(AppState->Players[1].Name, "gary 2") == 0);
    Check(strcmp(AppState->Players[2].Name, "Gary 3") == 0);

    GamePlayerJoined(&Game, 3);
    GamePlayerNamed(&Game, 3, "ABCDEFGHIJKLMNO");
    GamePlayerJoined(&Game, 4);
    GamePlayerNamed(&Game, 4, "ABCDEFGHIJKLMNO");
    Check(strcmp(AppState->Players[4].Name, "ABCDEFGHIJKLM 2") == 0);

    // No name stays no name ("Player N" on screen).
    GamePlayerJoined(&Game, 5);
    GamePlayerNamed(&Game, 5, "");
    GamePlayerJoined(&Game, 6);
    GamePlayerNamed(&Game, 6, "");
    Check(AppState->Players[6].Name[0] == 0);
    GameShutdown(&Game);
}

// Front-armoured monsters send their facing, a whole turn in 256 steps.
internal void
TestArmoredMonstersSendFacing()
{
    u32 Kind = 0;
    while (Kind < MonsterKind_Count && GetMonsterDef((monster_kind)Kind)->FrontArmor <= 0.f) ++Kind;
    if (Kind == MonsterKind_Count) return; // no armoured kind in this build
    u32 Plain = 0;
    while (Plain < MonsterKind_Count && GetMonsterDef((monster_kind)Plain)->FrontArmor > 0.f) ++Plain;

    static net_snapshot Out;
    Out = {};
    world_entity Monster = {};
    Monster.Type = EntityType_Monster;
    Monster.MonsterKind = (monster_kind)Kind;
    Monster.Direction = V2(0.f, -1.f);
    SimGameWriteFacing(&Monster, 5, &Out);
    Check(Out.FacingCount == 1);
    Check(Out.Facings[0].EntityIndex == 5);
    Check(Out.Facings[0].Angle == 192); // -90 degrees

    Monster.Direction = V2(-1.f, 0.f);
    SimGameWriteFacing(&Monster, 6, &Out);
    Check(Out.Facings[1].Angle == 128);

    // Unarmoured monsters and players send nothing.
    Monster.MonsterKind = (monster_kind)Plain;
    SimGameWriteFacing(&Monster, 7, &Out);
    Monster.Type = EntityType_Player;
    SimGameWriteFacing(&Monster, 8, &Out);
    Check(Out.FacingCount == 2);
}

// A player burned below the duel's one point of health is still alive,
// and must not reach the client as 0, which reads as dead there. It goes
// in hundredths, so the client's health bar shows the burn.
internal void
TestLivePlayerNeverSentAsDead()
{
    static net_snapshot Out;
    Out = {};
    world_entity Player = {};
    Player.Type = EntityType_Player;
    Player.MaxHp = 1.f;
    Player.Hp = 0.5f;
    SimGameWriteEntity(&Player, 3, &Out);
    Check(Out.Entities[0].Health == 50);
    Player.Hp = 0.f;
    SimGameWriteEntity(&Player, 4, &Out);
    Check(Out.Entities[1].Health == 0);
    Player.Hp = 73.f;
    SimGameWriteEntity(&Player, 5, &Out);
    Check(Out.Entities[2].Health == 7300);
    Player.Hp = 0.0001f;
    SimGameWriteEntity(&Player, 6, &Out);
    Check(Out.Entities[3].Health == 1);
}

// A fireball cast is heard by the caster and by players near it, once,
// and not by a player across the map.
internal bool32
SnapshotHasSound(net_snapshot *Snapshot, u8 Sound)
{
    for (u32 Index = 0; Index < Snapshot->SoundCount; ++Index)
    {
        if (Snapshot->Sounds[Index] == Sound) return true;
    }
    return false;
}

internal void
TestSoundsReachPlayersNearby()
{
    static server_game Game;
    GameInit(&Game);
    app_state *AppState = Game.AppState;
    GamePlayerJoined(&Game, 0);
    GamePlayerJoined(&Game, 1);
    GamePlayerJoined(&Game, 2);
    world_entity *Caster = AppState->Players[0].Entity;
    world_entity *Near = AppState->Players[1].Entity;
    world_entity *Far = AppState->Players[2].Entity;
    v3 Before = Near->Position;
    Near->Position = Caster->Position + V3(60.f, 0.f, 0.f);
    CheckAndChangeEntityChunk(AppState, &AppState->World, Game.Arena, Before, Near);
    Before = Far->Position;
    Far->Position = Caster->Position + V3(RELAY_HEARING_DISTANCE + 400.f, 0.f, 0.f);
    CheckAndChangeEntityChunk(AppState, &AppState->World, Game.Arena, Before, Far);

    static net_snapshot Out[3];
    for (u32 Slot = 0; Slot < 3; ++Slot) GameWriteSnapshot(&Game, Slot, &Out[Slot]);

    net_input Input = {};
    Input.Tick = 1;
    Input.Buttons = NetButton_Fireball;
    GameApplyInput(&Game, 0, &Input);
    for (u32 Tick = 0; Tick < 3; ++Tick) GameTick(&Game, 1.f / 60.f);

    for (u32 Slot = 0; Slot < 3; ++Slot)
    {
        Out[Slot] = {};
        GameWriteSnapshot(&Game, Slot, &Out[Slot]);
    }
    Check(SnapshotHasSound(&Out[0], (u8)AssetType_SfxFireCast));
    Check(SnapshotHasSound(&Out[1], (u8)AssetType_SfxFireCast));
    Check(!SnapshotHasSound(&Out[2], (u8)AssetType_SfxFireCast));

    // Heard once: the next snapshot does not repeat it.
    GameTick(&Game, 1.f / 60.f);
    Out[0] = {};
    GameWriteSnapshot(&Game, 0, &Out[0]);
    Check(!SnapshotHasSound(&Out[0], (u8)AssetType_SfxFireCast));

    // And it survives the trip through the protocol.
    Out[1].SoundCount = 1;
    Out[1].Sounds[0] = (u8)AssetType_SfxFireCast;
    static net_packet Packet;
    Packet = {};
    Packet.Header.Type = NetPacket_Snapshot;
    Packet.Snapshot = Out[1];
    static u8 Buffer[NET_MAX_PACKET_SIZE];
    u32 Size = NetWritePacket(&Packet, Buffer, sizeof(Buffer));
    Check(Size > 0);
    static net_packet Back;
    Check(NetReadPacket(Buffer, Size, &Back));
    Check(Back.Snapshot.SoundCount == 1 && Back.Snapshot.Sounds[0] == (u8)AssetType_SfxFireCast);
    GameShutdown(&Game);
}

// A player's death reaches every player's snapshot once, near or far.
internal void
TestKillsReachEveryone()
{
    static server_game Game;
    GameInit(&Game);
    app_state *AppState = Game.AppState;
    GamePlayerJoined(&Game, 0);
    GamePlayerJoined(&Game, 1);
    GamePlayerJoined(&Game, 2);
    static net_snapshot Out[3];
    for (u32 Slot = 0; Slot < 3; ++Slot) GameWriteSnapshot(&Game, Slot, &Out[Slot]);

    world_entity *Victim = AppState->Players[2].Entity;
    DamageEntity(AppState, &AppState->World, Victim, Victim->Hp + 1.f,
                 AppState->Players[0].Entity);
    GameTick(&Game, 1.f / 60.f);
    for (u32 Slot = 0; Slot < 3; ++Slot)
    {
        Out[Slot] = {};
        GameWriteSnapshot(&Game, Slot, &Out[Slot]);
        Check(Out[Slot].KillCount == 1);
        Check(Out[Slot].Kills[0].Killer == 0 && Out[Slot].Kills[0].Victim == 2);
    }
    GameTick(&Game, 1.f / 60.f);
    Out[0] = {};
    GameWriteSnapshot(&Game, 0, &Out[0]);
    Check(Out[0].KillCount == 0);
    GameShutdown(&Game);
}

// More moving things than fit in a snapshot: the viewer still gets its own
// player first and everything near it, and the rest nearest first.
internal void
TestSnapshotPrefersWhatIsNear()
{
    static server_game Game;
    GameInit(&Game);
    app_state *AppState = Game.AppState;
    world *World = &AppState->World;
    GamePlayerJoined(&Game, 0);
    v3 Center = AppState->Players[0].Entity->Position;

    // Far ones first, so sending in entity order would cut the near ones.
    for (u32 Index = 0; Index < 70; ++Index)
    {
        v3 Spot = V3(2400.f, 100.f + 15.f * Index, 0.f);
        AddMonster(AppState, World, Game.Arena, Spot, (monster_kind)0);
    }
    u32 NearIds[4];
    for (u32 Index = 0; Index < 4; ++Index)
    {
        v3 Spot = Center + V3(40.f + 25.f * Index, 30.f, 0.f);
        NearIds[Index] = AddMonster(AppState, World, Game.Arena, Spot, (monster_kind)0)->ID;
    }

    static net_snapshot Out;
    Out = {};
    GameWriteSnapshot(&Game, 0, &Out);
    Check(Out.Count == NET_MAX_SNAPSHOT_ENTITIES);
    Check(Out.Entities[0].Id == AppState->Players[0].Entity->ID);
    for (u32 Near = 0; Near < 4; ++Near)
    {
        bool32 Found = false;
        for (u32 Index = 0; Index < Out.Count; ++Index)
        {
            if (Out.Entities[Index].Id == NearIds[Near]) Found = true;
        }
        Check(Found);
    }
    bool32 Ordered = true;
    float Previous = 0.f;
    for (u32 Index = 1; Index < Out.Count; ++Index)
    {
        float Dx = Out.Entities[Index].X - Center.X;
        float Dy = Out.Entities[Index].Y - Center.Y;
        float DistanceSq = Dx * Dx + Dy * Dy;
        // positions are sent to 1/8 unit, so allow a little slack
        if (DistanceSq + 1.0f < Previous) Ordered = false;
        Previous = DistanceSq;
    }
    Check(Ordered);
    GameShutdown(&Game);
}

internal void
TestSnapshotCarriesMonsterWindup()
{
    static server_game Game;
    // NOTE(zoubir): the default map, the Old Arena, has no monsters
    GameInit(&Game, MapId_Keep);
    GamePlayerJoined(&Game, 0);

    world *World = &Game.AppState->World;
    world_entity *Monster = 0;
    for (u32 Index = 0; Index < World->EntityCount && !Monster; ++Index)
    {
        world_entity *Entity = &World->Entities[Index];
        if (Entity->IsPresent && Entity->Type == EntityType_Monster) Monster = Entity;
    }
    Check(Monster != 0);
    if (!Monster) { GameShutdown(&Game); return; }

    static net_snapshot Snapshot;
    GameWriteSnapshot(&Game, 0, &Snapshot);
    Check(Snapshot.AbilityCount == 0); // monsters start ready, nothing to warn about

    Monster->AbilityPhase = AbilityPhase_Windup;
    Monster->AbilityIndex = 1;
    Monster->AbilityTimer = 0.4f;
    Monster->AbilityAim = {0, 1};
    Monster->AbilityPointCount = 2;
    Monster->AbilityPoints[0] = {500, 600};
    Monster->AbilityPoints[1] = {700, 800};
    GameWriteSnapshot(&Game, 0, &Snapshot);

    Check(Snapshot.AbilityCount == 1);
    net_ability_state *A = &Snapshot.Abilities[0];
    net_entity_state *Owner = &Snapshot.Entities[A->EntityIndex];
    Check(Owner->Type == EntityType_Monster);
    Check(Owner->X == Monster->Position.X && Owner->Y == Monster->Position.Y);
    Check(A->Phase == AbilityPhase_Windup && A->Ability == 1);
    Check(A->TimeLeft == 0.4f);
    Check(A->PointCount == 2 && A->PointX[1] == 700.0f && A->PointY[1] == 800.0f);

    Monster->AbilityPhase = AbilityPhase_Recover;
    GameWriteSnapshot(&Game, 0, &Snapshot);
    Check(Snapshot.AbilityCount == 0);
    GameShutdown(&Game);
}

// Bots fill the highest slots no human uses, play (they move), make room
// when a human takes their slot, and leave when fewer are wanted.
internal void
TestBotsFillFreeSlots()
{
    static server_game Game;
    GameInit(&Game);
    app_state *AppState = Game.AppState;
    float Dt = 1.f / 60.f;
    Game.BotTarget = 3;
    GameKeepBots(&Game, 0, Dt);
    Check(AppState->Players[7].Active && AppState->Players[6].Active && AppState->Players[5].Active);
    Check(!AppState->Players[4].Active);
    Check(strcmp(AppState->Players[7].Name, "Bot 8") == 0);

    v3 Start[3];
    for (u32 Index = 0; Index < 3; ++Index) Start[Index] = AppState->Players[5 + Index].Entity->Position;
    for (u32 Tick = 0; Tick < 600; ++Tick)
    {
        GameKeepBots(&Game, 0, Dt);
        GameTick(&Game, Dt);
    }
    u32 Moved = 0;
    for (u32 Index = 0; Index < 3; ++Index)
    {
        world_entity *Bot = AppState->Players[5 + Index].Entity;
        if (Bot && LengthSq(Bot->Position.XY - Start[Index].XY) > Square(32.f)) ++Moved;
    }
    Check(Moved >= 2);

    // A human joins slot 7 (the network's choice); the bot there is gone
    // and another slot gets one.
    GamePlayerJoined(&Game, 7);
    GamePlayerNamed(&Game, 7, "Gary");
    GameKeepBots(&Game, 1u << 7, Dt);
    Check(!Game.Bots[7].Active);
    Check(strcmp(AppState->Players[7].Name, "Gary") == 0);
    Check(Game.Bots[4].Active && Game.Bots[5].Active && Game.Bots[6].Active);

    // Fewer wanted: the extra ones leave.
    Game.BotTarget = 1;
    GameKeepBots(&Game, 1u << 7, Dt);
    u32 Left = 0;
    for (u32 Slot = 0; Slot < MAX_PLAYERS; ++Slot) Left += Game.Bots[Slot].Active ? 1 : 0;
    Check(Left == 1);
    Check(AppState->Players[7].Active);
    GameShutdown(&Game);
}

// A talent asked for in the held buttons' talent field is spent once,
// however many inputs repeat it, and again only after the field lets go;
// the viewer's snapshot carries its experience and ranks, and everyone's
// level.
internal void
TestTalentFieldSpendsPoints()
{
    static server_game Game;
    GameInit(&Game);
    app_state *AppState = Game.AppState;
    GamePlayerJoined(&Game, 0);
    GamePlayerJoined(&Game, 1);
    player_slot *Slot = &AppState->Players[0];
    AwardXp(AppState, Slot, XpToReach(4));
    Check(TalentPointsLeft(Slot) == 3);

    u32 Ask = (u32)(Talent_Shield + 1) << NET_LEARN_SHIFT;
    net_input Input = {};
    for (u32 Repeat = 0; Repeat < 4; ++Repeat)
    {
        Input.Tick = Repeat + 1;
        Input.Buttons = Ask;
        GameApplyInput(&Game, 0, &Input);
        GameTick(&Game, 1.f / 60.f);
    }
    Check(Slot->Ranks[Talent_Shield] == 1);
    Input.Tick = 10;
    Input.Buttons = 0;
    GameApplyInput(&Game, 0, &Input);
    GameTick(&Game, 1.f / 60.f);
    Input.Tick = 11;
    Input.Buttons = Ask;
    GameApplyInput(&Game, 0, &Input);
    GameTick(&Game, 1.f / 60.f);
    Check(Slot->Ranks[Talent_Shield] == 2);
    // NOTE(zoubir): the field is no button press
    Check(Slot->Entity->MovementCooldowns[PlayerMove_Shield] == 0.f);

    static net_snapshot Out;
    Out = {};
    GameWriteSnapshot(&Game, 0, &Out);
    Check(Out.Xp == Slot->Xp);
    Check(Out.TalentRanks[Talent_Shield] == 2);
    Check(Out.ScoreCount == 2 && Out.Scores[0].Level == 4 && Out.Scores[1].Level == 1);
    GameShutdown(&Game);
}

internal void
RunServerGameTests()
{
    TestLivePlayerNeverSentAsDead();
    TestTalentFieldSpendsPoints();
    TestSnapshotPrefersWhatIsNear();
    TestSoundsReachPlayersNearby();
    TestKillsReachEveryone();
    TestArmoredMonstersSendFacing();
    TestSameNamesAreToldApart();
    TestSnapshotCarriesMonsterWindup();
    TestBotsFillFreeSlots();
}
