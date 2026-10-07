/* The talent panel's stats sidebar: what the talents add up to for the
   local player. Level and points, run speed, the fireball, respawn and the
   ward, then every ability the player has with its level and cooldown,
   its icon on the left. In a dungeon run a key the player's role has
   taken shows the role's spell and its cooldown after the role's talents
   instead (sim/dungeon/role_abilities.cpp). Numbers come from the same functions the
   simulation uses, so they are what the game plays. */

#define TALENT_STATS_ROW 22.f

// NOTE(zoubir): a label on the left and a value on the right
internal void
DrawStatRow(render_context *RenderContext, font *Font, float X, float Width, float Y,
            char *Label, char *Value, u32 ValueColor)
{
    UIText(RenderContext, Font, X, Y, Label, UI_COLOR_TEXT_MUTED);
    UIText(RenderContext, Font, X + Width, Y, Value, ValueColor, UIAlign_Right);
}

internal void
DrawTalentStats(render_context *RenderContext, app_state *AppState, talent_panel_layout *L)
{
    player_slot *Slot = &AppState->Players[AppState->LocalPlayerIndex];
    world_entity *Player = Slot->Entity;
    talent_panel *Panel = GetTalentPanel(AppState);
    font *Strong = AppState->Fonts.Strong ? AppState->Fonts.Strong : AppState->Fonts.Body;
    font *Small = AppState->Fonts.Small;
    float X = L->SidebarX;
    float Width = TALENT_SIDEBAR_WIDTH;
    DrawRoundRect(RenderContext, X, L->ColumnTop, Width, L->ColumnHeight,
                  UI_RGBA(255, 255, 255, 10));
    DrawRoundOutline(RenderContext, X, L->ColumnTop, Width, L->ColumnHeight,
                     UI_RGBA(255, 255, 255, 26));
    float Left = X + 14.f;
    float Inner = Width - 28.f;
    float Y = L->ColumnTop + 12.f;
    UIText(RenderContext, Strong, Left, Y, "Your stats", UI_COLOR_TEXT);
    Y += UILineHeight(Strong) + 8.f;

    char Value[48];
    u32 Spent = TalentPointsSpent(Slot);
    snprintf(Value, sizeof(Value), "%u spent, %u left", Spent, TalentPointsLeft(Slot));
    DrawStatRow(RenderContext, Small, Left, Inner, Y, "Points", Value, UI_COLOR_TEXT);
    Y += TALENT_STATS_ROW;
    float Run = PlayerStats.RunSpeed * RunSpeedScale(AppState, Player);
    snprintf(Value, sizeof(Value), "%.0f", Run);
    DrawStatRow(RenderContext, Small, Left, Inner, Y, "Run speed", Value,
                Run > PlayerStats.RunSpeed ? UI_COLOR_GOOD : UI_COLOR_TEXT);
    Y += TALENT_STATS_ROW;
    float Fireball = FireballSpeedScale(AppState, Player);
    snprintf(Value, sizeof(Value), "%.0f fast, %.0f far%s", PlayerStats.FireballSpeed * Fireball,
             PlayerStats.FireballRange * Fireball, Slot->Ranks[Talent_TwinFlame] ? ", x2" : "");
    DrawStatRow(RenderContext, Small, Left, Inner, Y, "Fireball", Value,
                Fireball > 1.f || Slot->Ranks[Talent_TwinFlame] ? UI_COLOR_GOOD : UI_COLOR_TEXT);
    Y += TALENT_STATS_ROW;
    snprintf(Value, sizeof(Value), "%.1f s, %.1f s shield", RespawnSeconds(Slot),
             RespawnShieldSeconds(Slot));
    DrawStatRow(RenderContext, Small, Left, Inner, Y, "Respawn", Value,
                Slot->Ranks[Talent_SecondWind] ? UI_COLOR_GOOD : UI_COLOR_TEXT);
    Y += TALENT_STATS_ROW;
    u32 WardColor = UI_COLOR_TEXT_MUTED;
    if (!Slot->Ranks[Talent_Ward])
    {
        snprintf(Value, sizeof(Value), "none");
    }
    else if (Slot->WardReady)
    {
        snprintf(Value, sizeof(Value), "up");
        WardColor = UI_RGBA(255, 214, 110, 255);
    }
    else
    {
        snprintf(Value, sizeof(Value), "spent");
        WardColor = UI_RGBA(255, 150, 120, 255);
    }
    DrawStatRow(RenderContext, Small, Left, Inner, Y, "Ward", Value, WardColor);
    Y += TALENT_STATS_ROW + 6.f;

    DrawFilledRectangle(RenderContext, Left, Y, Inner, 1.f, UI_RGBA(255, 255, 255, 30), 0.f);
    Y += 8.f;
    UIText(RenderContext, Strong, Left, Y, "Abilities", UI_COLOR_TEXT);
    Y += UILineHeight(Strong) + 6.f;

    // NOTE(zoubir): every ability the player has, in tree order, as long
    // as there is room
    float Bottom = L->ColumnTop + L->ColumnHeight - 8.f;
    float Icon = 18.f;
    for(u32 Talent = 0; Talent < Talent_Count && Y + TALENT_STATS_ROW < Bottom; Talent++)
    {
        talent_def *Def = &TalentDefs[Talent];
        u32 Level = TalentLevel(Slot, Talent);
        if (!Def->Button || Level == 0)
        {
            continue;
        }
        role_spell *Spell = RoleSpellOnButton(AppState, Player, Def->Button);
        if (Spell)
        {
            u32 Key = RoleKeyForButton(Def->Button);
            UIText(RenderContext, Small, Left + Icon + 6.f, Y, Spell->Name,
                   TalentBranchAccent(Slot, TalentBranch_Role));
            snprintf(Value, sizeof(Value), "%s  %.1f s", ActionKeyLabel(Def->Button),
                     RoleSpellCooldown(Slot, Key));
            UIText(RenderContext, Small, Left + Inner, Y, Value, UI_COLOR_TEXT, UIAlign_Right);
            Y += TALENT_STATS_ROW;
            continue;
        }
        DrawTexturedQuad(RenderContext, Panel->Atlas, Left, Y - 1.f, Icon, Icon,
                         TalentIconUvs(TalentIconCell(Slot, Talent)), 0xFFFFFFFF);
        UIText(RenderContext, Small, Left + Icon + 6.f, Y, Def->Name, UI_COLOR_TEXT);
        float Cooldown = TalentBaseCooldown(Player, Def->Button) * CooldownScaleForLevel(Level);
        snprintf(Value, sizeof(Value), "%.1f s", Cooldown);
        UIText(RenderContext, Small, Left + Inner, Y, Value,
               Level > 1 ? UI_COLOR_GOOD : UI_COLOR_TEXT, UIAlign_Right);
        // NOTE(zoubir): its level as small gold bars before the cooldown
        float BarsX = Left + Inner - 46.f - 9.f * (float)Def->MaxLevel;
        for(u32 Rank = 0; Rank < Def->MaxLevel; Rank++)
        {
            DrawFilledRectangle(RenderContext, BarsX + 9.f * (float)Rank, Y + 5.f, 6.f, 8.f,
                                Rank < Level ? UI_RGBA(255, 206, 90, 255) :
                                UI_RGBA(60, 62, 74, 255), 0.f);
        }
        Y += TALENT_STATS_ROW;
    }
}
