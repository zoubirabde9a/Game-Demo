/* Berserker Rage (role_kits/berserker.cpp): whole points in the slot's
   ClassMeter, so every client reads it, with the fraction of a point not
   yet whole kept in berserker_slot.RageCarry. Hits landed and taken give
   it (Unbridled Wrath gives more); after BERSERKER_CALM_SECONDS with
   neither it drains, except under Berserk. */

inline u32
BerserkerRage(player_slot *Slot)
{
    u32 Result = Slot->ClassMeter;
    return Result;
}

inline bool32
HasRage(player_slot *Slot, u32 Amount)
{
    bool32 Result = BerserkerRage(Slot) >= Amount;
    return Result;
}

// NOTE(zoubir): Amount more Rage (less when negative), kept within
// 0..BERSERKER_RAGE_MAX; a gain grows with Unbridled Wrath
internal void
AddRage(player_slot *Slot, float Amount)
{
    if (Amount > 0.f && RoleRank(Slot, PlayerRole_Berserker, BerserkerTalent_UnbridledWrath))
    {
        Amount *= 1.f + UNBRIDLED_WRATH_SHARE;
    }
    float Total = (float)Slot->ClassMeter + Slot->Berserker.RageCarry + Amount;
    Total = Maximum(0.f, Minimum((float)BERSERKER_RAGE_MAX, Total));
    float Whole = floorf(Total);
    Slot->ClassMeter = (u8)Whole;
    Slot->Berserker.RageCarry = Total - Whole;
}

// NOTE(zoubir): a press short of the Rage a spell costs: the HUD flashes
internal void
LackRage(player_slot *Slot)
{
    Slot->Berserker.NoRageSeconds = BERSERKER_NO_RAGE_SECONDS;
    Slot->ClassFlags |= BERSERKER_FLAG_NO_RAGE;
}

// NOTE(zoubir): takes Amount Rage, or with less than that takes none and
// says so. A client predicting its own Berserker leaves its Rage to the
// server, which sends it back
internal bool32
SpendRage(player_slot *Slot, u32 Amount)
{
    if (!HasRage(Slot, Amount))
    {
        LackRage(Slot);
        return false;
    }
    if (!Slot->Predicted)
    {
        Slot->ClassMeter = (u8)(Slot->ClassMeter - Amount);
    }
    return true;
}

// NOTE(zoubir): once a tick: Rage for the health lost since the last, the
// drain once calm, and Berserk and the HUD's flash running out
internal void
UpdateRage(player_slot *Slot, world_entity *Player, float DeltaTime)
{
    berserker_slot *State = &Slot->Berserker;
    State->CalmSeconds += DeltaTime;
    State->BerserkSeconds = Maximum(0.f, State->BerserkSeconds - DeltaTime);
    State->NoRageSeconds = Maximum(0.f, State->NoRageSeconds - DeltaTime);
    if (Player->Hp < State->LastHp && Player->Hp > 0.f)
    {
        AddRage(Slot, RAGE_PER_DAMAGE_TAKEN * (State->LastHp - Player->Hp));
        State->CalmSeconds = 0.f;
    }
    State->LastHp = Player->Hp;
    if (State->CalmSeconds > BERSERKER_CALM_SECONDS && State->BerserkSeconds <= 0.f)
    {
        AddRage(Slot, -RAGE_DECAY * DeltaTime);
    }
}
