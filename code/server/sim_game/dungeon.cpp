/* A dungeon run on the wire (sim/dungeon/, net/protocol.h): each
   player's role and what is drawn on them, in net_score.Dungeon and
   DungeonMore, and the run's rooms, wipes, boss, sanctuaries, infernos
   and one player's meter, in the snapshot's dungeon block. Read back by
   client/dungeon/dungeon_net.cpp. */

// NOTE(zoubir): X held to 0..1
inline float
DungeonShare(float X)
{
    float Result = X < 0.f ? 0.f : (X > 1.f ? 1.f : X);
    return Result;
}

// NOTE(zoubir): net_score.Dungeon for one player, 0 outside a run
internal u8
PackDungeonScore(app_state *AppState, player_slot *Slot)
{
    if (!IsDungeon(AppState))
    {
        return 0;
    }
    u32 Revive = (u32)(15.f * DungeonShare(Slot->ReviveSeconds / REVIVE_SECONDS));
    u32 Result = (Slot->Role & 3) |
        ((Slot->ShieldWallSeconds > 0.f) ? (1 << 2) : 0) |
        ((Slot->WardAbsorb > 0.f) ? (1 << 3) : 0) |
        (Revive << 4);
    return (u8)Result;
}

// NOTE(zoubir): net_score.DungeonMore for one player, 0 outside a run
internal u8
PackDungeonScoreMore(app_state *AppState, player_slot *Slot)
{
    if (!IsDungeon(AppState))
    {
        return 0;
    }
    u32 Result = ((Slot->RallySeconds > 0.f) ? 1 : 0) |
        ((Slot->RenewSeconds > 0.f) ? (1 << 1) : 0) |
        ((u32)Minimum((u32)Slot->Aggro, 7u) << 2) |
        (((u32)Slot->Role & 4) << 3);
    return (u8)Result;
}

// NOTE(zoubir): Seconds as tenths, at least 1 while any are left, at
// most Most
inline u8
DungeonTenths(float Seconds, u32 Most)
{
    u8 Result = Seconds > 0.f ? (u8)Minimum(Most, (u32)(10.f * Seconds) + 1) : 0;
    return Result;
}

// NOTE(zoubir): where the viewer stands, for which marks are nearest:
// its own body, else the middle of the map
inline v2
FoeMarkCenter(app_state *AppState, net_snapshot *Out)
{
    v2 Result = V2(0.f, 0.f);
    if (Out->HasOwnBody)
    {
        Result = V2(Out->OwnPosition[0], Out->OwnPosition[1]);
    }
    return Result;
}

// NOTE(zoubir): the foe marks nearest the viewer, at most
// NET_MAX_FOE_MARKS, as entity Ids the client knows
internal void
WriteFoeMarks(app_state *AppState, dungeon_run *Run, v2 Center, net_snapshot *Out)
{
    world *World = &AppState->World;
    float Distances[NET_MAX_FOE_MARKS];
    Out->MarkCount = 0;
    for(u32 Row = 0; Row < MAX_FOE_MARKS; Row++)
    {
        foe_mark *Mark = &Run->Marks[Row];
        world_entity *Monster = FoeMarkUsed(Mark) ?
            FindMonsterBySerial(World, Mark->Slot, Mark->Serial) : 0;
        if (!Monster)
        {
            continue;
        }
        float Distance = LengthSq(Monster->Position.XY - Center);
        u32 At = Out->MarkCount;
        if (At == NET_MAX_FOE_MARKS)
        {
            if (Distance >= Distances[At - 1])
            {
                continue;
            }
            At--;
        }
        else
        {
            Out->MarkCount++;
        }
        // NOTE(zoubir): kept sorted, nearest first
        while (At > 0 && Distances[At - 1] > Distance)
        {
            Distances[At] = Distances[At - 1];
            Out->MarkId[At] = Out->MarkId[At - 1];
            Out->MarkBits[At] = Out->MarkBits[At - 1];
            At--;
        }
        Distances[At] = Distance;
        Out->MarkId[At] = (u16)Mark->Slot;
        Out->MarkBits[At] = (u8)(Mark->Stacks | (Mark->SunderSeconds > 0.f ? NET_MARK_SUNDER : 0));
    }
}

// NOTE(zoubir): X in whole points, at most what a u16 holds
inline u16
MeterPoints(float X)
{
    u16 Result = (u16)Minimum(65535.f, Maximum(0.f, X + 0.5f));
    return Result;
}

// NOTE(zoubir): one player's meter, a different one each snapshot: all
// eight would cost every snapshot 56 bytes it has no room for. At 20
// snapshots a second a party of four each get theirs five times a second.
// A snapshot too full for it leaves it out (NetWriteSnapshotFitting)
internal void
WriteMeter(app_state *AppState, dungeon_run *Run, net_snapshot *Out)
{
    u32 Active[MAX_PLAYERS];
    u32 ActiveCount = 0;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        if (AppState->Players[SlotIndex].Active)
        {
            Active[ActiveCount++] = SlotIndex;
        }
    }
    u32 SlotIndex = ActiveCount ? Active[(Out->Tick / 3) % ActiveCount] : 0;
    player_slot *Slot = &AppState->Players[SlotIndex];
    Out->HasMeter = 1;
    Out->MeterSlot = (u8)SlotIndex;
    Out->MeterFight = (u8)(Run->MeterFight & 31);
    Out->MeterTenths = MeterPoints(10.f * Run->MeterSeconds);
    Out->MeterDamage = MeterPoints(Slot->MeterDamage);
    Out->MeterHealing = MeterPoints(Slot->MeterHealing);
    Out->MeterTaken = MeterPoints(Slot->MeterTaken);
}

internal void
WriteDungeonSnapshot(app_state *AppState, net_snapshot *Out)
{
    dungeon_run *Run = AppState->Dungeon;
    Out->HasDungeon = Run ? 1 : 0;
    Out->HasMeter = 0;
    if (!Run)
    {
        return;
    }
    Out->FightingRoom = (u8)Run->FightingRoom;
    Out->RoomsCleared = 0;
    for(u32 Room = 1; Room <= Run->RoomCount && Room <= 8; Room++)
    {
        if (Run->RoomStates[Room] == RoomState_Cleared)
        {
            Out->RoomsCleared |= (u8)(1 << (Room - 1));
        }
    }
    Out->Wipes = (u8)Minimum(Run->Wipes, 255u);
    Out->BossKind = Run->ShownBossKind < MonsterKind_Count ? (u8)Run->ShownBossKind : NET_NO_BOSS;
    Out->BossHealth = (u8)(255.f * DungeonShare(Run->ShownBossShare) + 0.5f);
    Out->FoesLeft = (u8)Minimum(Run->ShownFoesLeft, 255u);
    Out->BossClock = 0;
    Out->AddClock = (u8)(Minimum(Run->Clock.ShownAddSeconds, 127u) |
                         (Run->Clock.ShownAddBursts ? NET_ADD_BURSTS : 0));
    if (Out->BossKind != NET_NO_BOSS && Run->Clock.ShownStage == BossClock_Enraged)
    {
        Out->BossClock = NET_BOSS_ENRAGED;
    }
    else if (Out->BossKind != NET_NO_BOSS && Run->Clock.ShownStage != BossClock_None)
    {
        Out->BossClock = (u8)Minimum(Maximum(Run->Clock.ShownSecondsLeft, 1u), 254u);
    }
    Out->SanctuaryCount = 0;
    for(u32 Index = 0; Index < MAX_SANCTUARIES && Out->SanctuaryCount < NET_MAX_SANCTUARIES; Index++)
    {
        sanctuary *Zone = &Run->Sanctuaries[Index];
        if (Zone->Seconds > 0.f)
        {
            u32 Slot = Out->SanctuaryCount++;
            Out->SanctuaryX[Slot] = (i16)Zone->Position.X;
            Out->SanctuaryY[Slot] = (i16)Zone->Position.Y;
            Out->SanctuaryTenths[Slot] = (u8)(DungeonTenths(Zone->Seconds, 127) |
                ((Zone->Radius > SANCTUARY_RADIUS + 1.f) ? NET_ZONE_WIDE : 0));
        }
    }
    Out->InfernoCount = 0;
    for(u32 Index = 0; Index < MAX_INFERNOS && Out->InfernoCount < NET_MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        if (Zone->Delay > 0.f || Zone->Seconds > 0.f)
        {
            u32 Slot = Out->InfernoCount++;
            Out->InfernoX[Slot] = (i16)Zone->Position.X;
            Out->InfernoY[Slot] = (i16)Zone->Position.Y;
            u32 Tenths = Zone->Delay > 0.f ?
                (DungeonTenths(Zone->Delay, 63) | NET_INFERNO_FALLING) : DungeonTenths(Zone->Seconds, 63);
            Out->InfernoTenths[Slot] = (u8)Tenths;
        }
    }

    WriteFoeMarks(AppState, Run, FoeMarkCenter(AppState, Out), Out);
    WriteMeter(AppState, Run, Out);
}
