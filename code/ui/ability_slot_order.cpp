/* Ability slot order (ability_bar.cpp): which slot of the bar comes where,
   and whether it shows. Outside a dungeon run the bar is AbilitySlotDefs
   in its own order, each ability once the player has it. In a run the
   class's spells come first, on A, R, C, V and W, then a gap and the
   abilities every class shares (sim/dungeon/role_abilities.cpp,
   RunAllowedButtons), with the right click's after X for a class that
   swings its own weapon there; a class spell shows once its tree
   unlocked it. */

// NOTE(zoubir): a run's bar, left to right; 0 is the gap
global_variable u32 RunSlotButtons[] =
{
    PlayerButton_Launch,
    PlayerButton_Push,
    PlayerButton_Slam,
    PlayerButton_Kunai,
    PlayerButton_Shockwave,
    0,
    PlayerButton_Cast,
    PlayerButton_Attack,
    PlayerButton_Shield,
    PlayerButton_Blink,
    PlayerButton_Jump,
};

// NOTE(zoubir): how many places the bar walks
inline u32
AbilitySlotCount(app_state *AppState)
{
    u32 Result = IsDungeon(AppState) ? ArrayCount(RunSlotButtons) : ABILITY_SLOT_DEF_COUNT;
    return Result;
}

// NOTE(zoubir): the AbilitySlotDefs index at Position on the bar; a gap
// is any def with nothing to paint
internal u32
AbilitySlotIndex(app_state *AppState, u32 Position)
{
    if (!IsDungeon(AppState))
    {
        return Position;
    }
    u32 Button = Position < ArrayCount(RunSlotButtons) ? RunSlotButtons[Position] : 0;
    for(u32 Index = 0; Index < ABILITY_SLOT_DEF_COUNT; Index++)
    {
        ability_slot_def *Def = &AbilitySlotDefs[Index];
        if (Button ? Def->Button == Button : !Def->Paint)
        {
            return Index;
        }
    }
    return 0;
}

// NOTE(zoubir): whether slot Def shows: a gap never does, an ability once
// the player has it, in a run only what the class casts
inline bool32
IsAbilitySlotShown(app_state *AppState, player_slot *Slot, ability_slot_def *Def)
{
    bool32 Result = Def->Paint && AbilityLevel(Slot, Def->Button) > 0;
    if (IsDungeon(AppState))
    {
        Result = Def->Paint && (RunAllowedButtons(AppState, Slot, 0) & Def->Button);
    }
    return Result;
}
