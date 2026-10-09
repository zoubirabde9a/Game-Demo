/* Ability card contents (ability_tooltip.cpp): what the card over a
   hovered slot says, worked out before it is drawn. A dungeon class's
   spell reads its row in the class table (sim/dungeon/roles.cpp) and its
   numbers (spell_numbers.cpp), scaled by the class and its talents as a
   hit on a fresh foe would be; the game's own abilities read their talent
   (sim/progression/talents.cpp) at the level held, and the next level
   when a point can go in. */

#define ABILITY_TIP_STATS 6

#define TIP_COLOR_DAMAGE  UI_RGBA(255, 172, 112, 255)
#define TIP_COLOR_HEAL    UI_RGBA(126, 232, 146, 255)
#define TIP_COLOR_SHIELD  UI_RGBA(142, 202, 255, 255)
#define TIP_COLOR_CONTROL UI_RGBA(204, 164, 255, 255)
#define TIP_COLOR_BUFF    UI_RGBA(255, 214, 112, 255)
#define TIP_COLOR_PLAIN   UI_RGBA(214, 220, 232, 255)
#define TIP_COLOR_GOOD    UI_RGBA(126, 232, 146, 255)
#define TIP_COLOR_BAD     UI_RGBA(255, 120, 100, 255)

// NOTE(zoubir): one line of numbers: the figure in its colour, then what
// it is in a quieter one ("9" "damage, 5 hits")
struct ability_tip_stat
{
    char Value[24];
    char Label[64];
    u32 Color;
};

struct ability_tip_card
{
    char *Name;
    char Kind[64];
    char *Description;
    char DescriptionCopy[160];
    ability_tip_stat Stat[ABILITY_TIP_STATS];
    u32 StatCount;
    char Cooldown[40];
    char Left[24];   // seconds still to run, empty when ready
    bool32 HasCooldown;
    char Cost[48];
    u32 CostColor;
    char Next[112];  // what one more level gives, empty for none
    char Hint[64];
    u32 IconCell;    // in the ability bar's atlas
};

inline ability_tip_stat *
AddTipStat(ability_tip_card *Card, u32 Color)
{
    ability_tip_stat *Result = 0;
    if (Card->StatCount < ABILITY_TIP_STATS)
    {
        Result = &Card->Stat[Card->StatCount++];
        Result->Value[0] = 0;
        Result->Label[0] = 0;
        Result->Color = Color;
    }
    return Result;
}

// NOTE(zoubir): the numbers people read: 9, 0.6, 12.5
inline void
TipNumber(char *Out, u32 OutSize, float Value)
{
    snprintf(Out, OutSize, "%.3g", Value);
}

// NOTE(zoubir): Spell's row in SpellNumberTable, 0 when it has none
internal spell_numbers *
FindSpellNumbers(char *Spell)
{
    spell_numbers *Result = 0;
    for(u32 Index = 0; Index < ArrayCount(SpellNumberTable) && !Result; Index++)
    {
        if (SpellNumberTable[Index].Spell && strcmp(SpellNumberTable[Index].Spell, Spell) == 0)
        {
            Result = &SpellNumberTable[Index];
        }
    }
    return Result;
}

// NOTE(zoubir): a part of a spell's numbers as one line, damage and
// healing scaled to what the player would deal now on a fresh foe
internal void
AddSpellNumberStat(ability_tip_card *Card, spell_number *Part, float Dealt, float Healing,
                   float MaxHp)
{
    ability_tip_stat *Stat = 0;
    switch(Part->Kind)
    {
        case SpellNumber_Damage:
        case SpellNumber_Heal:
        {
            bool32 Heal = Part->Kind == SpellNumber_Heal;
            if ((Stat = AddTipStat(Card, Heal ? TIP_COLOR_HEAL : TIP_COLOR_DAMAGE)) != 0)
            {
                TipNumber(Stat->Value, sizeof(Stat->Value),
                          (float)(u32)(Part->Amount * (Heal ? Healing : Dealt) + 0.5f));
                char *Word = (char *)(Heal ? "healing" : "damage");
                if (Part->Hits > 1)
                {
                    snprintf(Stat->Label, sizeof(Stat->Label), "%s a hit, %u hits", Word, Part->Hits);
                }
                else if (Part->Seconds > 0.f)
                {
                    snprintf(Stat->Label, sizeof(Stat->Label), "%s over %.3g s", Word, Part->Seconds);
                }
                else
                {
                    snprintf(Stat->Label, sizeof(Stat->Label), "%s", Word);
                }
            }
        } break;
        case SpellNumber_Shield:
        {
            if ((Stat = AddTipStat(Card, TIP_COLOR_SHIELD)) != 0)
            {
                TipNumber(Stat->Value, sizeof(Stat->Value), Part->Amount);
                snprintf(Stat->Label, sizeof(Stat->Label), "damage absorbed");
            }
        } break;
        case SpellNumber_Stun:
        case SpellNumber_Freeze:
        case SpellNumber_Root:
        case SpellNumber_Slow:
        {
            if ((Stat = AddTipStat(Card, TIP_COLOR_CONTROL)) != 0)
            {
                snprintf(Stat->Value, sizeof(Stat->Value), "%.3g s", Part->Seconds);
                char *Word = (char *)(Part->Kind == SpellNumber_Stun ? "stun" :
                                      Part->Kind == SpellNumber_Freeze ? "freeze" :
                                      Part->Kind == SpellNumber_Root ? "held in place" : "slow");
                if (Part->Kind == SpellNumber_Slow && Part->Amount > 0.f)
                {
                    snprintf(Stat->Label, sizeof(Stat->Label), "slowed %.0f%%",
                             100.f * Part->Amount);
                }
                else
                {
                    snprintf(Stat->Label, sizeof(Stat->Label), "%s", Word);
                }
            }
        } break;
        case SpellNumber_DamageUp:
        case SpellNumber_DamageCut:
        {
            bool32 Up = Part->Kind == SpellNumber_DamageUp;
            if ((Stat = AddTipStat(Card, Up ? TIP_COLOR_BUFF : TIP_COLOR_SHIELD)) != 0)
            {
                snprintf(Stat->Value, sizeof(Stat->Value), "%s%.0f%%", Up ? "+" : "-",
                         100.f * Part->Amount);
                char *Word = (char *)(Up ? "damage dealt" : "damage taken");
                if (Part->Seconds > 0.f)
                {
                    snprintf(Stat->Label, sizeof(Stat->Label), "%s for %.3g s", Word,
                             Part->Seconds);
                }
                else
                {
                    snprintf(Stat->Label, sizeof(Stat->Label), "%s while it lasts", Word);
                }
            }
        } break;
        case SpellNumber_HealShare:
        {
            if ((Stat = AddTipStat(Card, TIP_COLOR_HEAL)) != 0)
            {
                TipNumber(Stat->Value, sizeof(Stat->Value),
                          (float)(u32)(Part->Amount * MaxHp * Healing + 0.5f));
                snprintf(Stat->Label, sizeof(Stat->Label), "healing, %.0f%% of your health",
                         100.f * Part->Amount);
            }
        } break;
        case SpellNumber_Lasts:
        {
            if ((Stat = AddTipStat(Card, TIP_COLOR_PLAIN)) != 0)
            {
                snprintf(Stat->Value, sizeof(Stat->Value), "%.3g s", Part->Seconds);
                snprintf(Stat->Label, sizeof(Stat->Label), "duration");
            }
        } break;
    }
}

// NOTE(zoubir): TalentEffectText's "2.0 s untouchable, speed 900" as one
// line a part, the figure in front taken out as the value when it leads
internal void
AddTalentEffectStats(ability_tip_card *Card, char *Text)
{
    char *At = Text;
    while (*At)
    {
        char Part[64];
        u32 Length = 0;
        while (At[Length] && !(At[Length] == ',' && At[Length + 1] == ' '))
        {
            ++Length;
        }
        Length = Minimum(Length, (u32)sizeof(Part) - 1);
        memcpy(Part, At, Length);
        Part[Length] = 0;
        At += Length;
        while (*At == ',' || *At == ' ') ++At;

        ability_tip_stat *Stat = AddTipStat(Card, TIP_COLOR_SHIELD);
        if (!Stat) break;
        char *Rest = Part;
        bool32 Leads = (*Rest >= '0' && *Rest <= '9') || *Rest == '+' || *Rest == '-';
        if (Leads)
        {
            while (*Rest && *Rest != ' ') ++Rest;
            // NOTE(zoubir): keep the unit with the figure ("2.0 s")
            if (Rest[0] == ' ' && Rest[1] == 's' && (Rest[2] == ' ' || Rest[2] == 0))
            {
                Rest += 2;
            }
            u32 ValueLength = Minimum((u32)(Rest - Part), (u32)sizeof(Stat->Value) - 1);
            memcpy(Stat->Value, Part, ValueLength);
            Stat->Value[ValueLength] = 0;
            while (*Rest == ' ') ++Rest;
        }
        else
        {
            // NOTE(zoubir): "Speed 520" reads as the others do, "520 speed"
            char *Space = strrchr(Part, ' ');
            if (Space && Space[1] >= '0' && Space[1] <= '9')
            {
                snprintf(Stat->Value, sizeof(Stat->Value), "%s", Space + 1);
                *Space = 0;
                Part[0] = (char)tolower((unsigned char)Part[0]);
            }
        }
        snprintf(Stat->Label, sizeof(Stat->Label), "%s", Rest);
    }
}

// NOTE(zoubir): the class meter a spell needs before it goes (ClassMeter,
// sent to clients, so this holds online too); 0 for none
internal u32
SpellMeterNeeded(char *Spell)
{
    u32 Result = 0;
    if (strcmp(Spell, "Whirlwind") == 0) Result = WHIRLWIND_RAGE;
    else if (strcmp(Spell, "Execute") == 0) Result = EXECUTE_MIN_RAGE;
    else if (strcmp(Spell, "Thunderclap") == 0) Result = (u32)THUNDERCLAP_MIN_CHARGE;
    else if (strcmp(Spell, "Eviscerate") == 0 || strcmp(Spell, "Deadly Throw") == 0) Result = 1;
    return Result;
}

// NOTE(zoubir): "3 combo points", "45 Rage", "1 Icicle"
internal void
FormatPoints(char *Out, u32 OutSize, u32 Count, char *Point)
{
    bool32 Counted = strcmp(Point, "combo point") == 0 || strcmp(Point, "Icicle") == 0;
    snprintf(Out, OutSize, "%u %s%s", Count, Point, (Counted && Count != 1) ? "s" : "");
}

// NOTE(zoubir): a spell that spends the class meter (ClassMeter, sent to
// clients, so this holds online too): its numbers with what the player
// holds now, or with the least it goes with when short, then what each
// point adds
internal void
AddSpendingStats(ability_tip_card *Card, player_slot *Slot, role_spell *Spell,
                 spell_numbers *Numbers, float Dealt, float Healing, float MaxHp)
{
    u32 Held = (u32)Slot->ClassMeter;
    u32 Least = SpellMeterNeeded(Spell->Name);
    u32 Spent = Maximum(Held, Least);
    char Points[48];
    FormatPoints(Points, sizeof(Points), Spent, Numbers->Point);
    bool32 Heals = false;
    for(u32 Part = 0; Part < SPELL_NUMBER_PARTS; Part++)
    {
        spell_number Copy = Numbers->Part[Part];
        u32 Index = Part + 1;
        if (Numbers->Grows == Index)
        {
            Copy.Amount += Numbers->PerPoint * (float)Spent;
            Heals = Copy.Kind == SpellNumber_Heal;
        }
        if (Numbers->Lengthens == Index)
        {
            Copy.Seconds *= (float)Maximum(Spent, 1u);
        }
        u32 Before = Card->StatCount;
        AddSpellNumberStat(Card, &Copy, Dealt, Healing, MaxHp);
        if (Card->StatCount == Before)
        {
            continue;
        }
        ability_tip_stat *Stat = &Card->Stat[Card->StatCount - 1];
        char Label[64];
        snprintf(Label, sizeof(Label), "%s", Stat->Label);
        if (Numbers->Grows == Index)
        {
            snprintf(Stat->Label, sizeof(Stat->Label), "%s %s %s", Label,
                     Held >= Least ? "now, with" : "with", Points);
        }
        else if (Numbers->Gated == Index && (float)Spent < Numbers->GatedFrom)
        {
            // NOTE(zoubir): it comes only on a big spend; say from what
            char From[48];
            FormatPoints(From, sizeof(From), (u32)Numbers->GatedFrom, Numbers->Point);
            snprintf(Stat->Label, sizeof(Stat->Label), "%s at %s", Label, From);
            Stat->Color = UI_COLOR_TEXT_MUTED;
        }
    }
    ability_tip_stat *Stat;
    if (Numbers->Grows == 0 && (Stat = AddTipStat(Card, TIP_COLOR_HEAL)) != 0)
    {
        // NOTE(zoubir): a line of its own (Rejuvenation's Bloom heals at once)
        Heals = true;
        TipNumber(Stat->Value, sizeof(Stat->Value),
                  (float)(u32)(Numbers->PerPoint * (float)Spent * Healing + 0.5f));
        snprintf(Stat->Label, sizeof(Stat->Label), "healing at once, with %s", Points);
    }
    if ((Stat = AddTipStat(Card, TIP_COLOR_PLAIN)) != 0)
    {
        float Each = Numbers->PerPoint * (Heals ? Healing : Dealt);
        snprintf(Stat->Value, sizeof(Stat->Value), "+%.2g", Each);
        snprintf(Stat->Label, sizeof(Stat->Label), "%s for each extra %s",
                 Heals ? "healing" : "damage", Numbers->Point);
    }
}

internal void
BuildRoleSpellCard(ability_tip_card *Card, player_slot *Slot, role_spell *Spell)
{
    Card->Name = Spell->Name;
    static char *AimWords[] = {"", ", foe target", ", ally target", ", ground target", ", line"};
    char *Aim = Spell->Aim == RoleAim_Foe ? AimWords[1] : Spell->Aim == RoleAim_Ally ? AimWords[2] :
        Spell->Aim == RoleAim_Ground ? AimWords[3] : Spell->Aim == RoleAim_Line ? AimWords[4] :
        AimWords[0];
    snprintf(Card->Kind, sizeof(Card->Kind), "%s spell%s", RoleTable[Slot->Role].Name, Aim);

    // NOTE(zoubir): Help reads "Name: what it does"; the card has the
    // name already, and a Help that opens on the cost ("30 Rage, spin...")
    // leaves it to the card's foot
    char *Description = Spell->Help;
    size_t NameLength = strlen(Spell->Name);
    if (Description && strncmp(Description, Spell->Name, NameLength) == 0 &&
        Description[NameLength] == ':')
    {
        Description += NameLength + 1;
        while (*Description == ' ') ++Description;
    }
    size_t CostLength = Spell->Cost ? strlen(Spell->Cost) : 0;
    if (Description && CostLength && strncmp(Description, Spell->Cost, CostLength) == 0 &&
        Description[CostLength] == ',')
    {
        Description += CostLength + 1;
        while (*Description == ' ') ++Description;
    }
    if (Description && Description[0])
    {
        snprintf(Card->DescriptionCopy, sizeof(Card->DescriptionCopy), "%s", Description);
        Card->DescriptionCopy[0] = (char)toupper((unsigned char)Card->DescriptionCopy[0]);
        Card->Description = Card->DescriptionCopy;
    }

    // NOTE(zoubir): as the simulation scales a hit on a monster at full
    // health (DungeonScaleDamage), so the card's damage is the player's
    world_entity Fresh = {};
    Fresh.Hp = Fresh.MaxHp = 1.f;
    float Dealt = GetRoleDef(Slot->Role)->DamageDealt * RoleTalentDealtScale(Slot, &Fresh);
    float Healing = 1.f + RoleStatShare(Slot, RoleStat_Healing);
    float MaxHp = Slot->Entity ? Slot->Entity->MaxHp : 0.f;
    spell_numbers *Numbers = FindSpellNumbers(Spell->Name);
    if (Numbers && Numbers->PerPoint > 0.f)
    {
        AddSpendingStats(Card, Slot, Spell, Numbers, Dealt, Healing, MaxHp);
    }
    else
    {
        for(u32 Part = 0; Numbers && Part < SPELL_NUMBER_PARTS; Part++)
        {
            AddSpellNumberStat(Card, &Numbers->Part[Part], Dealt, Healing, MaxHp);
        }
    }
    ability_tip_stat *Stat;
    if (Spell->Reach > 0.f && (Stat = AddTipStat(Card, TIP_COLOR_PLAIN)) != 0)
    {
        TipNumber(Stat->Value, sizeof(Stat->Value), (float)(u32)(Spell->Reach + 0.5f));
        snprintf(Stat->Label, sizeof(Stat->Label), "%s",
                 Spell->Aim == RoleAim_Ground ? "radius" :
                 Spell->Aim == RoleAim_None ? "reach" : "range");
    }

    if (Spell->Cost)
    {
        u32 Needed = SpellMeterNeeded(Spell->Name);
        if (Needed && (u32)Slot->ClassMeter < Needed)
        {
            snprintf(Card->Cost, sizeof(Card->Cost), "%s (have %u)", Spell->Cost,
                     (u32)Slot->ClassMeter);
            Card->CostColor = TIP_COLOR_BAD;
        }
        else
        {
            snprintf(Card->Cost, sizeof(Card->Cost), "%s", Spell->Cost);
            Card->CostColor = UI_COLOR_ACCENT;
        }
    }
}

internal void
BuildTalentAbilityCard(ability_tip_card *Card, player_slot *Slot, ability_slot_def *Def,
                       u32 Talent)
{
    talent_def *TalentDef = &TalentDefs[Talent];
    u32 Level = TalentLevel(Slot, Talent);
    snprintf(Card->Kind, sizeof(Card->Kind), "Ability, level %u of %u", Level,
             TalentDef->MaxLevel);
    Card->Description = TalentDef->Summary;
    char Effect[112];
    if (Level > 0 && TalentEffectText(Slot, Talent, Level, Effect, sizeof(Effect)))
    {
        AddTalentEffectStats(Card, Effect);
    }
    // NOTE(zoubir): a point can go in: what the next level makes it, and
    // the click that opens the tree on it (ability_bar.cpp)
    if (CanLearnTalent(Slot, Talent) == TalentRefusal_None && Level < TalentDef->MaxLevel)
    {
        char Next[96] = "";
        bool32 HasNext = TalentEffectText(Slot, Talent, Level + 1, Next, sizeof(Next));
        float Base = TalentBaseCooldown(Slot->Entity, Def->Button);
        if (Base > 0.f)
        {
            snprintf(Card->Next, sizeof(Card->Next), "Next level: %s%s%.3g s cooldown",
                     HasNext ? Next : "", HasNext ? ", " : "",
                     Base * CooldownScaleForLevel(Level + 1));
        }
        else if (HasNext)
        {
            snprintf(Card->Next, sizeof(Card->Next), "Next level: %s", Next);
        }
        snprintf(Card->Hint, sizeof(Card->Hint), "A point can go in: click to open it in the tree");
    }
}

// NOTE(zoubir): Full is the button's whole cooldown as it stands (after
// talents), 0 for none; Left is what is still to run
internal void
BuildAbilityTipCard(ability_tip_card *Card, app_state *AppState, world_entity *Player,
                    ability_slot_def *Def, float Full, float Left)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    Card->Name = Def->Name;
    Card->IconCell = (u32)(Def - AbilitySlotDefs);
    Card->CostColor = UI_COLOR_TEXT_MUTED;
    snprintf(Card->Cost, sizeof(Card->Cost), "No cost");

    // NOTE(zoubir): a dungeon role's spell on the key (sim/dungeon/
    // role_abilities.cpp) goes by its own name, icon and class table
    role_spell *RoleSpell = RoleSpellOnButton(AppState, Player, Def->Button);
    u32 Talent = TalentForButton(Def->Button);
    if (RoleSpell)
    {
        BuildRoleSpellCard(Card, Slot, RoleSpell);
        u32 RoleIcon = RoleIconIndex(Slot->Role, RoleKeyForButton(Def->Button));
        if (RoleIcon < ROLE_ICON_COUNT)
        {
            Card->IconCell = ABILITY_SLOT_DEF_COUNT + RoleIcon;
        }
        if (Full <= 0.f)
        {
            Full = RoleSpell->Cooldown;
        }
    }
    else if (Talent < Talent_Count)
    {
        BuildTalentAbilityCard(Card, Slot, Def, Talent);
    }

    Card->HasCooldown = Full > 0.f;
    if (Card->HasCooldown)
    {
        snprintf(Card->Cooldown, sizeof(Card->Cooldown), "%.3g s cooldown", Full);
    }
    else
    {
        snprintf(Card->Cooldown, sizeof(Card->Cooldown), "No cooldown");
    }
    if (Left > 0.f)
    {
        snprintf(Card->Left, sizeof(Card->Left), Left < 10.f ? "%.1f s left" : "%.0f s left",
                 Left);
    }
}
