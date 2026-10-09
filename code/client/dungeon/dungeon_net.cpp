/* A dungeon run online (sim/dungeon/, net/protocol.h): what the server
   sends of it, put where the HUD and the effects read it offline.

   Each score's Dungeon byte gives a player's role (with the health it
   brings, so the party frames and the bar show the right share), Shield
   Wall, a ward and revive progress; DungeonMore a rally, a renewal and
   how many monsters are after them, and the look the player wears. The snapshot's dungeon block gives
   the rooms, the fight, the healers' sanctuaries, the infernos and the
   foe marks, Searing and Sunder (on the replicas they were sent for), and
   one player's meter, kept until that player comes round again; the
   gate walls are then built or taken down
   locally from the room states (UpdateGates), so the local player's
   prediction stops at a closed gate as the server does. The role the
   player picks goes the other way, in role_requests.cpp. */

internal void
ApplyDungeonScore(app_state *AppState, player_slot *Slot, u8 Packed, u8 More)
{
    if (!IsDungeon(AppState))
    {
        return;
    }
    u32 Role = (Packed & 3) | ((More >> 3) & 12);
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
    Slot->WardFull = WARD_ABSORB;
    Slot->ReviveSeconds = REVIVE_SECONDS * (float)(Packed >> 4) / 15.f;
    Slot->RallySeconds = (More & 1) ? RALLY_SECONDS : 0.f;
    Slot->RenewSeconds = (More & (1 << 1)) ? RENEWAL_SECONDS : 0.f;
    Slot->Aggro = (u8)((More >> 2) & 7);
    Slot->Skin = (u8)(More >> 7);
}

// NOTE(zoubir): a new fight count zeroes every player's meter, as
// StartMeter did on the server; then the one player sent is filled in
internal void
ApplyMeterSnapshot(app_state *AppState, dungeon_run *Run, net_snapshot *Snapshot)
{
    if (!Snapshot->HasMeter)
    {
        return;
    }
    if (Snapshot->MeterFight != (Run->MeterFight & 31))
    {
        StartMeter(AppState, Run, Run->FightingRoom);
        Run->MeterFight = Snapshot->MeterFight;
    }
    if (Run->FightingRoom)
    {
        Run->MeterRoom = Run->FightingRoom;
    }
    Run->MeterSeconds = 0.1f * (float)Snapshot->MeterTenths;
    player_slot *Slot = &AppState->Players[Snapshot->MeterSlot & 7];
    Slot->MeterDamage = (float)Snapshot->MeterDamage;
    Slot->MeterHealing = (float)Snapshot->MeterHealing;
    Slot->MeterTaken = (float)Snapshot->MeterTaken;
}

internal void
ApplyDungeonSnapshot(app_state *AppState, memory_arena *Arena, net_snapshot *Snapshot,
                     u32 *LocalOfId, u32 IdCount)
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
    u8 BossClock = Snapshot->BossClock;
    Run->Clock.ShownStage = BossClock == NET_BOSS_ENRAGED ? BossClock_Enraged :
        (BossClock == 0 ? BossClock_None :
         ((float)BossClock <= BOSS_CLOCK_WARNING ? BossClock_Warned : BossClock_Running));
    Run->Clock.ShownSecondsLeft = BossClock == NET_BOSS_ENRAGED ? 0 : BossClock;
    Run->Clock.ShownAddSeconds = Snapshot->AddClock & ~NET_ADD_BURSTS;
    Run->Clock.ShownAddBursts = (Snapshot->AddClock & NET_ADD_BURSTS) != 0;
    for(u32 Index = 0; Index < MAX_SANCTUARIES; Index++)
    {
        sanctuary *Zone = &Run->Sanctuaries[Index];
        Zone->Seconds = 0.f;
        if (Index < Snapshot->SanctuaryCount)
        {
            Zone->Position = V3((float)Snapshot->SanctuaryX[Index],
                                (float)Snapshot->SanctuaryY[Index], 0.f);
            u8 Tenths = Snapshot->SanctuaryTenths[Index];
            Zone->Seconds = 0.1f * (float)(Tenths & ~NET_ZONE_WIDE);
            Zone->Radius = SANCTUARY_RADIUS * ((Tenths & NET_ZONE_WIDE) ? HALLOWED_RADIUS : 1.f);
        }
    }
    for(u32 Index = 0; Index < MAX_INFERNOS; Index++)
    {
        inferno *Zone = &Run->Infernos[Index];
        *Zone = {};
        if (Index < Snapshot->InfernoCount)
        {
            Zone->Position = V3((float)Snapshot->InfernoX[Index],
                                (float)Snapshot->InfernoY[Index], 0.f);
            u8 Packed = Snapshot->InfernoTenths[Index];
            float Seconds = 0.1f * (float)(Packed & 63);
            Zone->Delay = (Packed & NET_INFERNO_FALLING) ? Seconds : 0.f;
            // NOTE(zoubir): a falling meteor's burn is still to come; any
            // time will do for the drawing, which only asks whether it burns
            Zone->Seconds = (Packed & NET_INFERNO_FALLING) ? INFERNO_BURN_SECONDS : Seconds;
            Zone->Radius = INFERNO_RADIUS;
        }
    }

    // NOTE(zoubir): the marks, on the replicas they were sent for
    for(u32 Index = 0; Index < MAX_FOE_MARKS; Index++)
    {
        foe_mark *Mark = &Run->Marks[Index];
        *Mark = {};
        u32 Id = Index < Snapshot->MarkCount ? Snapshot->MarkId[Index] : IdCount;
        if (Id < IdCount && LocalOfId[Id])
        {
            Mark->Slot = LocalOfId[Id] - 1;
            u8 Bits = Snapshot->MarkBits[Index];
            Mark->Stacks = Minimum((u32)(Bits & NET_MARK_STACKS), (u32)SEARING_MOST);
            Mark->Seconds = Mark->Stacks ? SEARING_SECONDS : 0.f;
            Mark->SunderSeconds = (Bits & NET_MARK_SUNDER) ? SUNDER_SECONDS : 0.f;
            Mark->SunderShare = (Bits & NET_MARK_SUNDER) ? SUNDER_SHARE : 0.f;
        }
    }
    ApplyMeterSnapshot(AppState, Run, Snapshot);
    UpdateGates(AppState, &AppState->World, Arena, Run);
}
