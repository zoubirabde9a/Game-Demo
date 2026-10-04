// The screens drawn over the world each frame, in this order (later ones on
// top). Included inside AppUpdateAndRender in app.cpp, between UIBegin and
// UIEnd, so each line can use RenderContext, AppState, UIContext, Input,
// Window, TextureProgram and CameraOffset.
//
// Keep each entry to ONE line: the file merges line by line (merge=union
// in .gitattributes), so two agents adding a screen at once both keep
// theirs without a conflict. Put the logic in your own file and call it
// from here.
DrawMonsterTelegraphs(RenderContext, &AppState->World, CameraOffset);
DrawMonsterCastTells(RenderContext, AppState, CameraOffset, Input->DeltaTime);
DrawPlayerAbilityFx(RenderContext, AppState, CameraOffset, Input->DeltaTime);
DrawClickMoveMarker(RenderContext, CameraOffset);
DrawLandmarkPointer(RenderContext, AppState, CameraOffset, Window);
DrawHud(RenderContext, AppState, CameraOffset, Input->DeltaTime);
DrawMinimap(RenderContext, AppState, Window->Width);
DrawControlsPanel(RenderContext, AppState, Input, Window->Height);
DrawRespawnCountdown(RenderContext, AppState, Window->Width, Window->Height);
if (Input->TabButton.EndedDown) DrawScoreboard(RenderContext, AppState, Window->Width, Window->Height);
DoTileEditor(RenderContext, AppState, UIContext, Input, Window, TextureProgram, CameraOffset);
DoConnectScreen(RenderContext, AppState, UIContext, Input, Window->Width, Window->Height);
