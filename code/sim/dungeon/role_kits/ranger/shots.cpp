/* Ranger shots (role_kits/ranger.cpp): Focus, Hunter's Mark and the
   arrows in flight. An arrow's hit lands when the arrow gets there
   (RANGER_ARROW_SPEED, PIERCE_SPEED), so the damage number shows as the
   arrow the clients fly strikes. Hits on the foe under the Ranger's own
   mark build Focus, by how much the shot says (RangerShotFocus). */

// NOTE(zoubir): whether Monster is the foe under Slot's Hunter's Mark
inline bool32
IsRangerMarked(app_state *AppState, player_slot *Slot, world_entity *Monster)
{
    ranger_slot *Ranger = &Slot->Ranger;
    bool32 Result = Monster && Ranger->MarkSeconds > 0.f &&
        Ranger->MarkSlot == (u32)(Monster - AppState->World.Entities) &&
        Ranger->MarkSerial == Monster->MonsterSerial;
    return Result;
}

// NOTE(zoubir): the foe under Slot's mark, 0 for none
inline world_entity *
RangerMarkedFoe(app_state *AppState, player_slot *Slot)
{
    ranger_slot *Ranger = &Slot->Ranger;
    world_entity *Result = Ranger->MarkSeconds > 0.f ?
        FindMonsterBySerial(&AppState->World, Ranger->MarkSlot, Ranger->MarkSerial) : 0;
    if (Result && Result->Hp <= 0.f)
    {
        Result = 0;
    }
    return Result;
}

// NOTE(zoubir): Focus a hit of Shot on the marked foe builds
inline float
RangerShotFocus(u32 Shot)
{
    float Result = 0.f;
    switch(Shot)
    {
        case RangerShot_Quick: Result = QUICK_SHOT_FOCUS; break;
        case RangerShot_Rapid: Result = RAPID_FIRE_FOCUS; break;
        case RangerShot_Volley: Result = VOLLEY_FOCUS; break;
    }
    return Result;
}

// NOTE(zoubir): Amount more Focus; coming full flashes the bow
internal void
AddRangerFocus(app_state *AppState, player_slot *Slot, float Amount)
{
    ranger_slot *Ranger = &Slot->Ranger;
    bool32 WasFull = Ranger->Focus >= RANGER_FOCUS_MOST;
    Ranger->Focus = Minimum(RANGER_FOCUS_MOST, Ranger->Focus + Amount);
    Ranger->FocusHold = RANGER_FOCUS_HOLD;
    if (!WasFull && Ranger->Focus >= RANGER_FOCUS_MOST && Slot->Entity)
    {
        EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_FocusFull),
                  (u8)Slot->Entity->PlayerIndex, ChestOf(Slot->Entity));
    }
}

// NOTE(zoubir): Slot's mark goes on Foe, fresh, and clients are told
internal void
MarkRangerFoe(app_state *AppState, player_slot *Slot, world_entity *Foe, v2 From)
{
    ranger_slot *Ranger = &Slot->Ranger;
    Ranger->MarkSlot = (u32)(Foe - AppState->World.Entities);
    Ranger->MarkSerial = Foe->MonsterSerial;
    Ranger->MarkSeconds = MARK_SECONDS;
    Ranger->MarkKeep = RANGER_KEEP_SECONDS;
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Mark),
              (u8)(Slot - AppState->Players), ChestOf(Foe),
              ATan2(Foe->Position.Y - From.Y, Foe->Position.X - From.X));
}

// NOTE(zoubir): an arrow of Shot from player By at Foe, landing Delay
// from now; dropped when every arrow is in flight
internal void
LooseRangerArrow(app_state *AppState, u32 By, world_entity *Foe, u32 Shot, float Damage,
                 float Delay, v2 Away)
{
    ranger_run *Run = &AppState->Dungeon->Ranger;
    for(u32 Index = 0; Index < RANGER_MAX_ARROWS; Index++)
    {
        ranger_arrow *Arrow = &Run->Arrows[Index];
        if (Arrow->Shot == RangerShot_None)
        {
            Arrow->TargetSlot = (u32)(Foe - AppState->World.Entities);
            Arrow->TargetSerial = Foe->MonsterSerial;
            Arrow->Delay = Delay;
            Arrow->Damage = Damage;
            Arrow->Away = Away;
            Arrow->By = (u8)By;
            Arrow->Shot = (u8)Shot;
            return;
        }
    }
}

// NOTE(zoubir): an arrow from Player at Foe, its burst for clients to fly
// (Variant: RangerArrow_*); Foe 0 shoots along the aim at nothing
internal void
ShootRangerArrow(app_state *AppState, world_entity *Player, world_entity *Foe, u32 Shot,
                 float Damage, u32 Variant)
{
    u8 By = (u8)Player->PlayerIndex;
    v3 Spot;
    v2 Dir;
    if (Foe)
    {
        v2 Offset = Foe->Position.XY - Player->Position.XY;
        Dir = NormalizeOr(Offset, GetPlayerAim(Player));
        Spot = ChestOf(Foe);
        LooseRangerArrow(AppState, By, Foe, Shot, Damage, Length(Offset) / RANGER_ARROW_SPEED, Dir);
    }
    else
    {
        Dir = GetPlayerAim(Player);
        Spot = ChestOf(Player);
        Spot.XY += 0.6f * QUICK_SHOT_RANGE * Dir;
        Variant = RangerArrow_Miss;
    }
    EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Arrow), By, Spot,
              RangerBurstAngle(ATan2(Dir.Y, Dir.X), Variant));
}

// NOTE(zoubir): a hit of Shot on Monster for the Ranger in slot By, which
// OnRangerHit knows by Hitting
internal void
RangerHit(app_state *AppState, u32 By, world_entity *Monster, u32 Shot, float Damage,
          float Shove, v2 Away, status_effect Status = StatusEffect_None, float StatusSeconds = 0.f)
{
    player_slot *Slot = &AppState->Players[By];
    world_entity *Player = Slot->Entity;
    if (!Player || !Monster->IsPresent || Monster->Hp <= 0.f)
    {
        return;
    }
    hit Hit = {Damage, Shove, 0.f, 0.f, 0.f, SimBurst_Count, Status, StatusSeconds};
    Slot->Ranger.Hitting = Shot;
    ApplyHit(AppState, &AppState->World, Monster, &Hit, Away, Player, By);
    Slot->Ranger.Hitting = RangerShot_None;
}

// NOTE(zoubir): once a tick: arrows reach their foes
internal void
UpdateRangerArrows(app_state *AppState, ranger_run *Run, float DeltaTime)
{
    for(u32 Index = 0; Index < RANGER_MAX_ARROWS; Index++)
    {
        ranger_arrow *Arrow = &Run->Arrows[Index];
        if (Arrow->Shot == RangerShot_None)
        {
            continue;
        }
        Arrow->Delay -= DeltaTime;
        if (Arrow->Delay > 0.f)
        {
            continue;
        }
        world_entity *Foe = FindMonsterBySerial(&AppState->World, Arrow->TargetSlot,
                                                Arrow->TargetSerial);
        u32 Shot = Arrow->Shot;
        Arrow->Shot = RangerShot_None;
        if (Foe && Foe->Hp > 0.f)
        {
            float Shove = Shot == RangerShot_Pierce ? PIERCE_SHOVE : QUICK_SHOT_SHOVE;
            RangerHit(AppState, Arrow->By, Foe, Shot, Arrow->Damage, Shove, Arrow->Away);
            if (Shot == RangerShot_Quick || Shot == RangerShot_Pierce)
            {
                EmitSound(&AppState->Events, AssetType_SfxHit, Foe->Position);
            }
        }
    }
}

// NOTE(zoubir): once a tick for a Ranger: its mark runs down, and with
// Lethal Mark it jumps from a foe that died to the nearest one left
internal void
UpdateRangerMark(app_state *AppState, player_slot *Slot, float DeltaTime)
{
    ranger_slot *Ranger = &Slot->Ranger;
    if (Ranger->MarkSeconds <= 0.f)
    {
        return;
    }
    Ranger->MarkSeconds = Maximum(0.f, Ranger->MarkSeconds - DeltaTime);
    world *World = &AppState->World;
    world_entity *Was = Ranger->MarkSlot < World->EntityCount ? &World->Entities[Ranger->MarkSlot] : 0;
    world_entity *Foe = RangerMarkedFoe(AppState, Slot);
    if (Foe)
    {
        // NOTE(zoubir): the look again where the foe is now, a keep-alive
        // (variant 1) that plays no landing
        Ranger->MarkKeep -= DeltaTime;
        if (Ranger->MarkKeep <= 0.f)
        {
            Ranger->MarkKeep += RANGER_KEEP_SECONDS;
            EmitBurst(&AppState->Events, ClassBurst(SimBurst_RangerFirst, RangerBurst_Mark),
                      (u8)(Slot - AppState->Players), ChestOf(Foe), RangerBurstAngle(0.f, 1));
        }
        return;
    }
    if (Ranger->MarkSeconds <= 0.f || !Was)
    {
        return;
    }
    Ranger->MarkSeconds = 0.f;
    if (RoleRank(Slot, PlayerRole_Ranger, RangerTalent_LethalMark) && Slot->Entity)
    {
        v2 Where = Was->Position.XY;
        world_entity *Next = NearestFoe(World, Where, LETHAL_MARK_JUMP,
                                        RoomAtPosition(World, Slot->Entity->Position.XY), 0, 0);
        if (Next)
        {
            MarkRangerFoe(AppState, Slot, Next, Where);
        }
    }
}
