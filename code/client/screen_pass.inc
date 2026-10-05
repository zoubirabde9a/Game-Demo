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
DrawWardShells(RenderContext, AppState, CameraOffset); // the Ward talent's shell round players, client/talent_fx.cpp
DrawRewindFx(RenderContext, AppState, CameraOffset); // time rewinds' sigils, ghosts and flashes, client/rewind_fx/
DrawLandmarkPointer(RenderContext, AppState, CameraOffset, &View);
RenderSetProjection(RenderContext, &ProjectionMatrix); // screens below draw in window pixels
DrawRewindHud(RenderContext, AppState, Window->Width); // tape counter while time runs back, client/rewind_fx/
DrawHud(RenderContext, AppState, CameraOffset, Input->DeltaTime);
DrawAbilityBar(RenderContext, AppState, Input, Window->Width, Window->Height);
DrawXpOverlays(RenderContext, AppState, Window->Width, Window->Height); // XP gains and the level-up banner, ui/talent_panel/xp_bar.cpp
DrawMinimap(RenderContext, AppState, Window->Width);
DrawControlsPanel(RenderContext, AppState, Input, Window->Height);
DrawRespawnCountdown(RenderContext, AppState, Window->Width, Window->Height);
DrawTalentPanel(RenderContext, AppState, Input, Window->Width, Window->Height); // the talent tree (N), ui/talent_panel/
if (Input->TabButton.EndedDown) DrawScoreboard(RenderContext, AppState, Window->Width, Window->Height);
DoTileEditor(RenderContext, AppState, UIContext, Input, Window, TextureProgram, CameraOffset);
DoConnectScreen(RenderContext, AppState, UIContext, Input, Window->Width, Window->Height);
DrawShaderErrors(RenderContext, AppState, Window->Width);
