// The screens drawn over the world each frame, in this order (later ones on
// top). Included inside AppUpdateAndRender in app.cpp, between UIBegin and
// UIEnd, so each line can use RenderContext, AppState, UIContext, Input,
// Window, TextureProgram and CameraOffset.
//
// Keep each entry to ONE line: the file merges line by line (merge=union
// in .gitattributes), so two agents adding a screen at once both keep
// theirs without a conflict. Put the logic in your own file and call it
// from here.
RenderSetProjection(RenderContext, &WorldProjection); // world overlays below draw zoomed, like the world
DrawMonsterTelegraphs(RenderContext, &AppState->World, CameraOffset);
DrawMonsterCastTells(RenderContext, AppState, CameraOffset, Input->DeltaTime);
DrawPlayerAbilityFx(RenderContext, AppState, CameraOffset, Input->DeltaTime);
DrawCastPreview(RenderContext, AppState, CameraOffset); // range and area of the ability being aimed or wound up, client/cast_targeting/previews.cpp
DrawTalentFx(RenderContext, AppState, CameraOffset); // frost on slowed and frozen units, the Ward talent's shell, client/talent_fx.cpp
DrawDungeonFx(RenderContext, AppState, CameraOffset); // dungeon gates, sanctuaries, shield wall, wards and revives, client/dungeon/dungeon_fx.cpp
DrawRewindFx(RenderContext, AppState, CameraOffset); // time rewinds' sigils, ghosts and flashes, client/rewind_fx/
DrawLandmarkPointer(RenderContext, AppState, CameraOffset, &View);
DrawCursorTarget(RenderContext, AppState, CameraOffset); // brackets on the unit the cursor is on, client/targeting.cpp
DrawThreatPointers(RenderContext, AppState, CameraOffset, &View); // close enemies off screen, client/threat_pointers.cpp
RenderSetProjection(RenderContext, &ProjectionMatrix); // screens below draw in window pixels
DrawScreenEdge(RenderContext, AppState, Window->Width, Window->Height, Input->DeltaTime); // vignette and hurt tint, client/screen_edge.cpp
DrawRewindHud(RenderContext, AppState, Window->Width); // tape counter while time runs back, client/rewind_fx/
DrawHud(RenderContext, AppState, CameraOffset, Input->DeltaTime);
DrawAbilityBar(RenderContext, AppState, Input, Window->Width, Window->Height);
DrawStatusStrip(RenderContext, AppState, Window->Width, Window->Height); // the local player's running status effects, ui/status_strip.cpp
DoCastModeToggle(RenderContext, AppState, Input, Window->Width); // quick cast checkbox at the top, ui/cast_mode_toggle.cpp
DrawXpOverlays(RenderContext, AppState, Window->Width, Window->Height); // XP gains and the level-up banner, ui/talent_panel/xp_bar.cpp
DrawMinimap(RenderContext, AppState, Window->Width);
DrawKillFeed(RenderContext, AppState, Window->Width); // recent kills under the minimap, ui/kill_feed_view.cpp
DrawControlsPanel(RenderContext, AppState, Input, Window->Height);
DrawDeathPlate(RenderContext, AppState, Window->Width, Window->Height); // who killed you and the respawn countdown, ui/kill_feed_view.cpp
OpenTalentsForRoundBreak(AppState); // the talent panel opens for the break between rounds, ui/round_break_view.cpp
DrawTalentPanel(RenderContext, AppState, Input, Window->Width, Window->Height); // the talent tree (N), ui/talent_panel/
DrawRoundBreak(RenderContext, AppState, Window->Width, Window->Height); // countdown to the next round, over the talent panel, ui/round_break_view.cpp
DrawMapVotePlate(RenderContext, AppState, Window->Width); // a map vote is open: who wants which map, Esc to answer, ui/map_vote_view.cpp
DoDungeonHud(RenderContext, AppState, Input, Window->Width, Window->Height); // a dungeon run's objective, boss bar and role picker, ui/dungeon/dungeon_hud.cpp
DrawBossAlerts(RenderContext, AppState, Window->Width, Window->Height); // a banner when a boss enrages or its timer runs out, ui/dungeon/boss_alerts.cpp
if (Input->TabButton.EndedDown) DrawScoreboard(RenderContext, AppState, Window->Width, Window->Height);
DoTileEditor(RenderContext, AppState, UIContext, Input, Window, TextureProgram, CameraOffset);
DoConnectScreen(RenderContext, AppState, UIContext, Input, Window->Width, Window->Height);
DoOptionsMenu(RenderContext, AppState, Input, Window->Width, Window->Height); // Esc: keyboard layout, ui/options_menu.cpp
DrawTargetNotice(RenderContext, AppState); // "No target" or "Out of range" by the cursor, client/targeting.cpp
DrawShaderErrors(RenderContext, AppState, Window->Width);
