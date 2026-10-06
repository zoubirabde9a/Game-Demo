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

// NOTE(zoubir): the fireball's speed share at Level with Swift Flames at
// SwiftRank (FireballSpeedScale)
inline float
FireballScaleAt(u32 Level, u32 SwiftRank)
{
    float Extra = Level > 1 ? (float)(Level - 1) : 0.f;
    float Result = (1.f + TALENT_SWIFT_FLAMES_SCALE * (float)SwiftRank) *
        (1.f + TalentDefs[Talent_Fireball].SpeedPerLevel * Extra);
    return Result;
}

// NOTE(zoubir): what Talent does at Level (an ability's level, or a
// passive's rank) into Out; false when there is nothing beyond its
// summary and cooldown to say
internal bool32
TalentEffectText(player_slot *Slot, u32 Talent, u32 Level, char *Out, u32 OutSize)
{
    talent_def *Def = &TalentDefs[Talent];
    float Extra = Level > 1 ? (float)(Level - 1) : 0.f;
    Out[0] = 0;
    switch (Talent)
    {
        case Talent_Fireball:
        {
            float Scale = FireballScaleAt(Level, Slot->Ranks[Talent_SwiftFlames]);
            snprintf(Out, OutSize, "Speed %.0f, range %.0f", PlayerStats.FireballSpeed * Scale,
                     PlayerStats.FireballRange * Scale);
        } break;
        case Talent_SwiftFlames:
        {
            snprintf(Out, OutSize, "Fireballs +%u%% speed and range",
                     (u32)(100.f * TALENT_SWIFT_FLAMES_SCALE * (float)Level + 0.5f));
        } break;
        case Talent_TwinFlame:
        {
            snprintf(Out, OutSize, "%u fireball%s a cast", Level ? 2 : 1, Level ? "s" : "");
        } break;
        case Talent_Shield:
        {
            snprintf(Out, OutSize, "%.1f s untouchable",
                     PlayerMovements[PlayerMove_Shield].Power * (1.f + Def->PowerPerLevel * Extra));
        } break;
        case Talent_FleetFoot:
        {
            snprintf(Out, OutSize, "Run speed %.0f",
                     PlayerStats.RunSpeed * (1.f + TALENT_FLEET_FOOT_SCALE * (float)Level));
        } break;
        case Talent_Ward:
        {
            if (Level)
            {
                snprintf(Out, OutSize, "Back %.0f s after it breaks",
                         TalentWardSeconds[Minimum(Level, 2u)]);
            }
        } break;
        case Talent_SecondWind:
        {
            snprintf(Out, OutSize, "Respawn in %.1f s, %.1f s shield",
                     PLAYER_RESPAWN_SECONDS * (Level ? TALENT_SECOND_WIND_RESPAWN : 1.f),
                     Level ? TALENT_SECOND_WIND_SHIELD : PLAYER_SPAWN_SHIELD_SECONDS);
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
