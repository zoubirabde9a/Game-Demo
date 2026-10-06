/* What talents change in play (talents.cpp has the table and the point
   rules): cooldowns and power by ability level, the longer stuns and
   harder shoves of a levelled area spell (AbilityLevelHit), fireball and
   run speed, respawn time and shield, and the Ward that takes one hit
   whole (WardTakesHit). The simulation calls these where it uses each
   number; a player with no talents gets the rules' plain values. */

inline player_slot *
TalentSlotOf(app_state *AppState, world_entity *Player)
{
    player_slot *Result = (Player->Type == EntityType_Player &&
                           Player->PlayerIndex < MAX_PLAYERS) ?
        &AppState->Players[Player->PlayerIndex] : 0;
    return Result;
}

// NOTE(zoubir): what Button's cooldown is multiplied by at its level
inline float
CooldownScaleForLevel(u32 Level)
{
    float Result = Level > 1 ? 1.f - TALENT_COOLDOWN_PER_LEVEL * (float)(Level - 1) : 1.f;
    return Result;
}

internal float
PlayerCooldownScale(app_state *AppState, world_entity *Player, u32 Button)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    float Result = Slot ? CooldownScaleForLevel(AbilityLevel(Slot, Button)) : 1.f;
    return Result;
}

internal float
PlayerPowerScale(app_state *AppState, world_entity *Player, u32 Button)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    u32 Talent = TalentForButton(Button);
    float Result = 1.f;
    if (Slot && Talent < Talent_Count)
    {
        u32 Level = TalentLevel(Slot, Talent);
        if (Level > 1)
        {
            Result += TalentDefs[Talent].PowerPerLevel * (float)(Level - 1);
        }
    }
    return Result;
}

// NOTE(zoubir): levels Player has bought past the first in Button's
// ability, 0 for none
internal float
ExtraAbilityLevels(app_state *AppState, world_entity *Player, u32 Button, u32 *Talent)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    *Talent = TalentForButton(Button);
    float Result = 0.f;
    if (Slot && *Talent < Talent_Count)
    {
        u32 Level = TalentLevel(Slot, *Talent);
        Result = Level > 1 ? (float)(Level - 1) : 0.f;
    }
    return Result;
}

// NOTE(zoubir): Hit as Player's level of Button's ability lands it:
// longer stuns and statuses, harder shoves
internal hit
AbilityLevelHit(app_state *AppState, world_entity *Player, u32 Button, hit *Hit)
{
    hit Result = *Hit;
    u32 Talent;
    float Extra = ExtraAbilityLevels(AppState, Player, Button, &Talent);
    if (Extra > 0.f)
    {
        talent_def *Def = &TalentDefs[Talent];
        if (Result.StunSeconds > 0.f)
        {
            Result.StunSeconds += Def->StunPerLevel * Extra;
        }
        if (Result.StatusSeconds > 0.f)
        {
            Result.StatusSeconds += Def->StatusPerLevel * Extra;
        }
        Result.Shove *= 1.f + Def->ShovePerLevel * Extra;
    }
    return Result;
}

// NOTE(zoubir): a passive's rank on the player behind Player, 0 for none
inline u32
PlayerTalentRank(app_state *AppState, world_entity *Player, u32 Talent)
{
    player_slot *Slot = TalentSlotOf(AppState, Player);
    u32 Result = Slot ? Slot->Ranks[Talent] : 0;
    return Result;
}

// NOTE(zoubir): Swift Flames' ranks and the fireball's own levels
inline float
FireballSpeedScale(app_state *AppState, world_entity *Player)
{
    u32 Talent;
    float Extra = ExtraAbilityLevels(AppState, Player, PlayerButton_Cast, &Talent);
    float Result = (1.f + TALENT_SWIFT_FLAMES_SCALE *
                    (float)PlayerTalentRank(AppState, Player, Talent_SwiftFlames)) *
        (1.f + TalentDefs[Talent_Fireball].SpeedPerLevel * Extra);
    return Result;
}

inline float
RunSpeedScale(app_state *AppState, world_entity *Player)
{
    float Result = 1.f + TALENT_FLEET_FOOT_SCALE *
        (float)PlayerTalentRank(AppState, Player, Talent_FleetFoot);
    return Result;
}

inline float
RespawnSeconds(player_slot *Slot)
{
    float Result = PLAYER_RESPAWN_SECONDS;
    if (Slot->Ranks[Talent_SecondWind])
    {
        Result *= TALENT_SECOND_WIND_RESPAWN;
    }
    return Result;
}

inline float
RespawnShieldSeconds(player_slot *Slot)
{
    float Result = Slot->Ranks[Talent_SecondWind] ?
        TALENT_SECOND_WIND_SHIELD : PLAYER_SPAWN_SHIELD_SECONDS;
    return Result;
}

// NOTE(zoubir): the Ward talent taking a hit of Damage on Target instead
// of it; true when it did (DamageEntity, ApplyHit). A spent ward comes
// back after its rank's seconds (UpdateProgression)
internal bool32
WardTakesHit(app_state *AppState, world_entity *Target, float Damage)
{
    player_slot *Slot = TalentSlotOf(AppState, Target);
    bool32 Result = false;
    if (Slot && Damage > 0.f && Slot->WardReady && Slot->Ranks[Talent_Ward] &&
        !Slot->Predicted)
    {
        Slot->WardReady = false;
        Slot->WardRecharge = TalentWardSeconds[Slot->Ranks[Talent_Ward]];
        v3 Chest = Target->Position;
        Chest.Z += 16.f;
        EmitBurst(&AppState->Events, SimBurst_WardBreak,
                  (u8)Target->PlayerIndex, Chest);
        EmitSound(&AppState->Events, AssetType_Dash, Target->Position);
        Result = true;
    }
    return Result;
}
