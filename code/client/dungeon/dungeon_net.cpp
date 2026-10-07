/* A dungeon run online (sim/dungeon/, net/protocol.h): what the server
   sends of it, put where the HUD and the effects read it offline.

   Each score's Dungeon byte gives a player's role (with the health it
   brings, so the party frames and the bar show the right share), Shield
   Wall, a ward and revive progress. The snapshot's dungeon block gives
   the rooms and the fight; the gate walls are then built or taken down
   locally from the room states (UpdateGates), so the local player's
   prediction stops at a closed gate as the server does. The role the
   player picks goes the other way, in role_requests.cpp. */

internal void
ApplyDungeonScore(app_state *AppState, player_slot *Slot, u8 Packed)
{
    if (!IsDungeon(AppState))
    {
        return;
    }
    u32 Role = Packed & 3;
    if (Role < PlayerRole_Count)
    {
        Slot->Role = (u8)Role;
        if (Slot->Entity)
        {
            Slot->Entity->MaxHp = GetRoleDef(Role)->MaxHp;
        }
    }
    Slot->ShieldWallSeconds = (Packed & (1 << 2)) ? SHIELD_WALL_SECONDS : 0.f;
    Slot->WardAbsorb = (Packed & (1 << 3)) ? WARD_ABSORB : 0.f;
    Slot->ReviveSeconds = REVIVE_SECONDS * (float)(Packed >> 4) / 15.f;
}

internal void
ApplyDungeonSnapshot(app_state *AppState, memory_arena *Arena, net_snapshot *Snapshot)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run || !Snapshot->HasDungeon)
    {
        return;
    }
    Run->FightingRoom = Snapshot->FightingRoom <= Run->RoomCount ? Snapshot->FightingRoom : 0;
    for(u32 Room = 1; Room <= Run->RoomCount; Room++)
    {
        bool32 Cleared = Room <= 8 && (Snapshot->RoomsCleared & (1 << (Room - 1)));
        Run->RoomStates[Room] = (u8)(Cleared ? RoomState_Cleared :
                                     (Room == Run->FightingRoom ? RoomState_Fighting :
                                      RoomState_Waiting));
    }
    Run->Wipes = Snapshot->Wipes;
    Run->ShownBossKind = Snapshot->BossKind < MonsterKind_Count ?
        (u32)Snapshot->BossKind : (u32)MonsterKind_Count;
    Run->ShownBossShare = (float)Snapshot->BossHealth / 255.f;
    Run->ShownFoesLeft = Snapshot->FoesLeft;
    UpdateGates(AppState, &AppState->World, Arena, Run);
}
