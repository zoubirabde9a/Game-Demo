// app_state fields that only the player's machine uses: one pointer per
// client or UI feature, each made by its feature (usually on first use).
// Included inside struct app_state in app.h, so code still writes
// AppState->Minimap. One field per line with its note on the same line:
// the file merges line by line (merge=union in .gitattributes), so two
// agents adding a feature at once both keep their line.
    struct online_session *Online; // connection to a dedicated server, client/online.cpp
    struct kill_feed *KillFeed; // recent player deaths for the UI, client/kill_feed.cpp
    struct connect_screen *ConnectScreen; // address and name form, ui/connect_screen.cpp
    struct minimap *Minimap; // ui/minimap.cpp, made on first draw
    struct controls_panel *ControlsPanel; // key list and its intro timer, ui/controls_panel.cpp
    struct player_fx *PlayerFx; // ability effects drawn over the world, client/player_fx.cpp
    struct fx_bursts *FxBursts; // bursts the simulation asked for, client/fx_bursts.cpp
    struct body_poses *BodyPoses; // squash and stretch per unit, client/body_pose.cpp
    struct hud_pulses *HudPulses; // cooldown bars flashing as they come ready, ui/hud.cpp
    struct monster_cast_tells *CastTells; // what monsters are casting, client/monster_cast_tells.cpp
    struct ability_bar *AbilityBar; // health and ability slots at the bottom, ui/ability_bar.cpp
    bool32 CameraPlaced; // the camera has been put on the player once; until then it cuts, client/camera.cpp
