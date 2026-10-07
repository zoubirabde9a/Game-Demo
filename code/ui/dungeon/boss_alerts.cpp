/* Boss alerts (dungeon_hud.cpp): a banner across the screen when a boss
   fight turns, so the party sees it happen instead of noticing later that
   the boss is red and hitting harder.

   - A boss crossing into its enraged phase (sim/monster_abilities/
     phases.cpp): "<name> enrages", and under it the attacks it only uses
     from now on, read from their PhaseMask.
   - The boss's enrage timer running out (sim/dungeon/boss_clock.cpp).

   Read from PhaseFlash, which snapshots carry, and the run's Shown clock
   stage, so it shows online too. One banner at a time; a new one replaces
   the old. Entry point: DrawBossAlerts, once a frame from screen_pass.inc. */

#define BOSS_ALERT_SECONDS 2.8f
#define BOSS_ALERT_FADE_IN 0.15f
#define BOSS_ALERT_FADE_OUT 0.7f
#define BOSS_ALERT_TOP_SHARE 0.27f
#define BOSS_ALERT_BAND_COLOR UI_RGBA(20, 6, 8, 170)
#define BOSS_ALERT_TITLE_COLOR UI_RGBA(255, 120, 80, 255)

struct boss_alerts
{
    // NOTE(zoubir): the boss whose enrage was announced, so a flash is
    // announced once
    u32 AnnouncedId;
    u32 LastClockStage;
    float Born;
    bool32 Showing;
    char Title[64];
    char Detail[128];
};

internal void
StartBossAlert(boss_alerts *Alerts, float Now, char *Title, char *Detail)
{
    Alerts->Showing = true;
    Alerts->Born = Now;
    snprintf(Alerts->Title, sizeof(Alerts->Title), "%s", Title);
    snprintf(Alerts->Detail, sizeof(Alerts->Detail), "%s", Detail);
}

// NOTE(zoubir): "Faster, and now: Grave Lunge" from the abilities only
// the enraged phase allows
internal void
EnragedDetail(monster_def *Def, char *Out, u32 Size)
{
    u32 Used = (u32)snprintf(Out, Size, "Faster and fiercer");
    bool32 First = true;
    for(u32 Index = 0; Index < Def->AbilityCount && Used < Size; Index++)
    {
        monster_ability *Ability = &Def->Abilities[Index];
        if (Ability->PhaseMask == PHASE_ENRAGED)
        {
            Used += (u32)snprintf(Out + Used, Size - Used, "%s%s",
                                  First ? ", and now: " : ", ", Ability->Name);
            First = false;
        }
    }
}

internal void
WatchBossAlerts(app_state *AppState, boss_alerts *Alerts, float Now)
{
    world *World = &AppState->World;
    for(u32 Index = 0; Index < World->EntityCount; Index++)
    {
        world_entity *Entity = &World->Entities[Index];
        if (!Entity->IsPresent || Entity->Type != EntityType_Monster ||
            Entity->PhaseFlash <= 0.f || Entity->ID == Alerts->AnnouncedId)
        {
            continue;
        }
        monster_def *Def = GetMonsterDef(Entity->MonsterKind);
        if (!Def || Def->SpawnWeight != 0 || Def->EnrageHpShare <= 0.f)
        {
            continue;
        }
        Alerts->AnnouncedId = Entity->ID;
        char Title[64];
        char Detail[128];
        snprintf(Title, sizeof(Title), "%s enrages", Def->Name);
        EnragedDetail(Def, Detail, sizeof(Detail));
        StartBossAlert(Alerts, Now, Title, Detail);
    }
    u32 Stage = AppState->Dungeon->Clock.ShownStage;
    if (Stage == BossClock_Enraged && Alerts->LastClockStage != BossClock_Enraged)
    {
        StartBossAlert(Alerts, Now, "Out of time",
                       "The boss goes berserk: every hit grows harder");
    }
    Alerts->LastClockStage = Stage;
}

internal void
DrawBossAlerts(render_context *RenderContext, app_state *AppState,
               u32 WindowWidth, u32 WindowHeight)
{
    if (!AppState->Dungeon)
    {
        return;
    }
    if (!AppState->BossAlerts)
    {
        AppState->BossAlerts = AllocateStruct(&AppState->MemoryArena, boss_alerts);
        *AppState->BossAlerts = {};
    }
    boss_alerts *Alerts = AppState->BossAlerts;
    float Now = GetFxClock(AppState);
    WatchBossAlerts(AppState, Alerts, Now);
    float Age = Now - Alerts->Born;
    if (!Alerts->Showing || Age < 0.f || Age >= BOSS_ALERT_SECONDS)
    {
        Alerts->Showing = false;
        return;
    }

    float Alpha = Minimum(Clamp01(Age / BOSS_ALERT_FADE_IN),
                          Clamp01((BOSS_ALERT_SECONDS - Age) / BOSS_ALERT_FADE_OUT));
    font *Title = AppState->Fonts.Title;
    font *Body = AppState->Fonts.Body;
    float TitleHeight = UILineHeight(Title);
    float BodyHeight = UILineHeight(Body);
    float BandHeight = TitleHeight + BodyHeight + 3.f * UI_GAP_SMALL;
    float Top = BOSS_ALERT_TOP_SHARE * (float)WindowHeight;
    float CenterX = 0.5f * (float)WindowWidth;
    // NOTE(zoubir): the title lands with a small drop, as if slammed down
    float Drop = 10.f * (1.f - Clamp01(Age / (2.f * BOSS_ALERT_FADE_IN)));
    DrawFilledRectangle(RenderContext, 0.f, Top, (float)WindowWidth, BandHeight,
                        WithAlpha(BOSS_ALERT_BAND_COLOR, Alpha), 0.f);
    UIText(RenderContext, Title, CenterX, Top + UI_GAP_SMALL - Drop, Alerts->Title,
           WithAlpha(BOSS_ALERT_TITLE_COLOR, Alpha), UIAlign_Center);
    UIText(RenderContext, Body, CenterX, Top + 2.f * UI_GAP_SMALL + TitleHeight,
           Alerts->Detail, WithAlpha(UI_COLOR_TEXT, Alpha), UIAlign_Center);
}
