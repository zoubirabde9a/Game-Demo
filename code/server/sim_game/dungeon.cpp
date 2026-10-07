/* A dungeon run on the wire (sim/dungeon/, net/protocol.h): each
   player's role and what is drawn on them, in net_score.Dungeon, and the
   run's rooms, wipes, boss and sanctuaries, in the snapshot's dungeon
   block. Read back
   by client/dungeon/dungeon_net.cpp. */

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
            Out->SanctuaryTenths[Slot] = (u8)Minimum(255u, (u32)(10.f * Zone->Seconds) + 1);
        }
    }
}
