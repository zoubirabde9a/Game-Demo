/* A dungeon run on the wire (sim/dungeon/, net/protocol.h): each
   player's role and what is drawn on them, in net_score.Dungeon and
   DungeonMore, and the run's rooms, wipes, boss, sanctuaries and
   infernos, in the snapshot's dungeon block. Read back by
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
        ((u32)Minimum((u32)Slot->Aggro, 7u) << 2);
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

internal void
WriteDungeonSnapshot(app_state *AppState, net_snapshot *Out)
{
    dungeon_run *Run = AppState->Dungeon;
    Out->HasDungeon = Run ? 1 : 0;
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
            Out->InfernoTenths[Slot] = (u8)(Tenths |
                ((Zone->Radius > INFERNO_RADIUS + 1.f) ? NET_ZONE_WIDE : 0));
        }
    }
}
