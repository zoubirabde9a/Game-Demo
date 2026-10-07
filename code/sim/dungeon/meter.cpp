/* Meter (dungeon.cpp): what each player did in the last room fought, for
   the HUD's meter (ui/dungeon/damage_meter.cpp): the damage they dealt
   to monsters, the health they gave allies, and the health they lost.

   It counts only while a room is fought, and starts again from 0 when the
   next fight starts (StartEncounter, a retry after a wipe included), so
   between fights it still shows the one just over. Only health that moved
   counts: the overkill past a monster's last point and the healing past
   a full bar are left out.

   DamageEntity (entity.cpp) and HealPlayer (role_kits/allies.cpp) report
   here. Online the server sends one player's numbers a snapshot, in turn
   (server/sim_game/dungeon.cpp). */

// NOTE(zoubir): from StartEncounter: a fresh meter for Room
internal void
StartMeter(app_state *AppState, dungeon_run *Run, u32 Room)
{
    Run->MeterRoom = Room;
    Run->MeterSeconds = 0.f;
    Run->MeterFight++;
    for(u32 SlotIndex = 0; SlotIndex < MAX_PLAYERS; SlotIndex++)
    {
        player_slot *Slot = &AppState->Players[SlotIndex];
        Slot->MeterDamage = 0.f;
        Slot->MeterHealing = 0.f;
        Slot->MeterTaken = 0.f;
    }
}

// NOTE(zoubir): once a tick, from UpdateDungeon: the fight's length, for
// the numbers a second
inline void
UpdateMeter(dungeon_run *Run, float DeltaTime)
{
    if (Run->FightingRoom)
    {
        Run->MeterSeconds += DeltaTime;
    }
}

// NOTE(zoubir): from DamageEntity: a hit by Source took Lost health from
// Target
internal void
CountMeterDamage(app_state *AppState, world_entity *Target, world_entity *Source,
                 float Lost)
{
    dungeon_run *Run = AppState->Dungeon;
    if (!Run || !Run->FightingRoom || !(Lost > 0.f))
    {
        return;
    }
    if (Target->Type == EntityType_Player && Target->PlayerIndex < MAX_PLAYERS)
    {
        AppState->Players[Target->PlayerIndex].MeterTaken += Lost;
    }
    player_slot *Attacker = DungeonAttackerSlot(AppState, Source);
    if (Attacker && Target->Type == EntityType_Monster)
    {
        Attacker->MeterDamage += Lost;
    }
}

// NOTE(zoubir): from HealPlayer: the player in slot By gave Given health
internal void
CountMeterHealing(app_state *AppState, u32 By, float Given)
{
    dungeon_run *Run = AppState->Dungeon;
    if (Run && Run->FightingRoom && By < MAX_PLAYERS && Given > 0.f)
    {
        AppState->Players[By].MeterHealing += Given;
    }
}
