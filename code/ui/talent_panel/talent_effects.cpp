/* What a talent does in numbers, at a given level or rank, for the
   tooltip ("now" and "next") and the stats sidebar. Read from the same
   tables the simulation uses (sim/progression/talents.cpp, the ability
   rows in sim/player_abilities/), so a retuned number shows here too. */

// NOTE(zoubir): the area row Button fires, 0 for none; the slam's row
// has no key, the slam's dive sets it off
internal player_area_ability *
AreaRowForButton(u32 Button)
{
    player_area_ability *Result = 0;
    for(u32 Index = 0; Index < PlayerArea_Count && Button; Index++)
    {
        if (PlayerAreaAbilities[Index].Button == Button)
        {
            Result = &PlayerAreaAbilities[Index];
        }
    }
    if (Button == PlayerButton_Slam)
    {
        Result = &PlayerAreaAbilities[PlayerArea_Slam];
    }
    return Result;
}

// NOTE(zoubir): what Talent does at Level (an ability's level, or a
// passive's rank) into Out; false when there is nothing beyond its
// summary and cooldown to say
// NOTE(zoubir): a run talent of the class tree at Level ranks: each effect's
// amount a rank times the ranks, grown by its tier (ClassSlotDepthScale,
// sim/dungeon/class_tree.cpp)
internal void
RunTalentEffectText(player_slot *Slot, u32 Talent, u32 Level, char *Out, u32 OutSize)
{
    run_mod_def *Mod = &RunModDefs[ClassRunModAt(Slot, Talent)];
    float Depth = ClassSlotDepthScale(ClassTreeSlot(Talent));
    u32 Used = 0;
    for(u32 Part = 0; Part < 2 && Mod->Effect[Part] && Used + 1 < OutSize; Part++)
    {
        int Wrote = snprintf(Out + Used, OutSize - Used, "%s", Part ? ", " : "");
        Used += Wrote > 0 ? (u32)Wrote : 0;
        Wrote = snprintf(Out + Used, OutSize - Used, RunEffectFormats[Mod->Effect[Part]],
                         RunEffectShown(Mod->Effect[Part], Mod->PerRank[Part] * (float)Level * Depth));
        Used = Minimum(Used + (Wrote > 0 ? (u32)Wrote : 0), OutSize - 1);
    }
}

// NOTE(zoubir): a duel perk at Rank ranks, in numbers
// (sim/progression/talents/duel_perks.cpp)
internal void
DuelPerkEffectText(u32 Perk, u32 Rank, char *Out, u32 OutSize)
{
    float R = (float)Rank;
    switch (Perk)
    {
        case DuelPerk_SwiftFlames: snprintf(Out, OutSize, "Fireballs +%.0f%% speed and range", 100.f * DUEL_SWIFT_FLAMES_SCALE * R); break;
        case DuelPerk_Kindling: snprintf(Out, OutSize, "Fireball cooldown -%.0f%%", 100.f * DUEL_KINDLING_COOLDOWN * R); break;
        case DuelPerk_Searing: snprintf(Out, OutSize, "Fireballs +%.0f%% damage", 100.f * DUEL_SEARING_DAMAGE * R); break;
        case DuelPerk_Scorch: snprintf(Out, OutSize, "A fireball hit slows for %.1f s", DUEL_SCORCH_SLOW * R); break;
        case DuelPerk_TwinFlame: snprintf(Out, OutSize, "%u fireball%s a cast", Rank ? 2 : 1, Rank ? "s" : ""); break;
        case DuelPerk_Pyre: snprintf(Out, OutSize, "Fireballs +%.0f%% damage, cooldown +%.0f%%", 100.f * DUEL_PYRE_DAMAGE * R, 100.f * DUEL_PYRE_COOLDOWN * R); break;
        case DuelPerk_Overcharge: snprintf(Out, OutSize, "+%.0f%% damage dealt, +%.0f%% taken", 100.f * DUEL_OVERCHARGE_DAMAGE * R, 100.f * DUEL_OVERCHARGE_TAKEN * R); break;
        case DuelPerk_FleetFoot: snprintf(Out, OutSize, "Run speed %.0f", PlayerStats.RunSpeed * (1.f + DUEL_FLEET_FOOT_SCALE * R)); break;
        case DuelPerk_Quickstep: snprintf(Out, OutSize, "Blink cooldown -%.0f%%", 100.f * DUEL_QUICKSTEP_COOLDOWN * R); break;
        case DuelPerk_KeenEdge: snprintf(Out, OutSize, "Sword and kunai +%.0f%% damage", 100.f * DUEL_KEEN_EDGE_DAMAGE * R); break;
        case DuelPerk_Hunter: snprintf(Out, OutSize, "Kunai cooldown -%.0f%%", 100.f * DUEL_HUNTER_COOLDOWN * R); break;
        case DuelPerk_Momentum: snprintf(Out, OutSize, "%.1f s off blink, slam and kunai a hit", DUEL_MOMENTUM_REFUND * R); break;
        case DuelPerk_Adrenaline: snprintf(Out, OutSize, "+%.0f%% run speed under half health", 100.f * DUEL_ADRENALINE_SCALE * R); break;
        case DuelPerk_Phantom: snprintf(Out, OutSize, "%.1f s untouchable after a blink", DUEL_PHANTOM_SECONDS * R); break;
        case DuelPerk_Ward: if (Rank) snprintf(Out, OutSize, "Back %.0f s after it breaks", DuelWardSeconds[Minimum(Rank, 2u)]); break;
        case DuelPerk_ThickSkin: snprintf(Out, OutSize, "%.0f%% less damage taken", 100.f * DUEL_THICK_SKIN_SHARE * R); break;
        case DuelPerk_Mending: snprintf(Out, OutSize, "%.1f health a second", DUEL_MENDING_PER_SECOND * R); break;
        case DuelPerk_Steadfast: snprintf(Out, OutSize, "Stuns hold you %.0f%% shorter", 100.f * DUEL_STEADFAST_SHARE * R); break;
        case DuelPerk_SecondWind: snprintf(Out, OutSize, "Once a round: 1 health and %.1f s shield", DUEL_SECOND_WIND_SHIELD); break;
        case DuelPerk_Thorns: snprintf(Out, OutSize, "%.0f%% of a player's hit back at them", 100.f * DUEL_THORNS_SHARE * R); break;
        case DuelPerk_Bastion: snprintf(Out, OutSize, "Shield heals %.0f, waits %.0f%% longer", DUEL_BASTION_HEAL, 100.f * DUEL_BASTION_COOLDOWN * R); break;
    }
}

internal bool32
TalentEffectText(player_slot *Slot, u32 Talent, u32 Level, char *Out, u32 OutSize)
{
    talent_def *Def = &TalentDefs[Talent];
    float Extra = Level > 1 ? (float)(Level - 1) : 0.f;
    Out[0] = 0;
    bool32 RunMod = IsClassTalent(Talent) && ClassRunModAt(Slot, Talent);
    if (RunMod)
    {
        RunTalentEffectText(Slot, Talent, Level, Out, OutSize);
    }
    bool32 Wild = IsDuelWildTalent(Talent);
    if (Wild)
    {
        DuelPerkEffectText(DuelPerkAt(Slot->DuelSeed, Talent), Level, Out, OutSize);
    }
    switch ((RunMod || Wild) ? Talent_Count : Talent)
    {
        case Talent_Fireball:
        {
            float Scale = FireballScaleAt(Level, DuelPerkRank(Slot, DuelPerk_SwiftFlames));
            float Damage = FireballDamageScaleAt(Level, DuelPerkRank(Slot, DuelPerk_Searing),
                                                 DuelPerkRank(Slot, DuelPerk_Pyre));
            snprintf(Out, OutSize, "%.0f damage, speed %.0f, range %.0f",
                     PlayerStats.FireballDamage * Damage, PlayerStats.FireballSpeed * Scale,
                     PlayerStats.FireballRange * Scale);
        } break;
        case Talent_Shield:
        {
            snprintf(Out, OutSize, "%.1f s untouchable",
                     PlayerMovements[PlayerMove_Shield].Power * (1.f + Def->PowerPerLevel * Extra));
        } break;
        case Talent_Kunai:
        {
            snprintf(Out, OutSize, "%.0f damage, speed %.0f",
                     PlayerStats.KunaiDamage * (1.f + Def->PowerPerLevel * Extra),
                     PlayerStats.KunaiSpeed);
        } break;
        default:
        {
            player_area_ability *Row = AreaRowForButton(Def->Button);
            if (Row)
            {
                char *At = Out;
                u32 Left = OutSize;
                float Stun = Row->Hit.StunSeconds + Def->StunPerLevel * Extra;
                float Status = Row->Hit.StatusSeconds + Def->StatusPerLevel * Extra;
                float Shove = Row->Hit.Shove * (1.f + Def->ShovePerLevel * Extra);
                int Wrote = 0;
                if (Stun > 0.f)
                {
                    Wrote = snprintf(At, Left, "%s %.2g s", Talent == Talent_FrostNova ? "Freeze" :
                                     Talent == Talent_GravityWell ? "Hold" : "Stun", Stun);
                    At += Wrote; Left -= (u32)Wrote;
                }
                if (Status > 0.f && Left > 1)
                {
                    Wrote = snprintf(At, Left, "%sslow %.2g s", At != Out ? ", " : "", Status);
                    At += Wrote; Left -= (u32)Wrote;
                }
                if (Def->ShovePerLevel > 0.f && Left > 1)
                {
                    snprintf(At, Left, "%sshove %.0f", At != Out ? ", " : "", Shove);
                }
            }
        } break;
    }
    bool32 Result = Out[0] != 0;
    return Result;
}
