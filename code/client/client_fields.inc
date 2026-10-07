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
    struct rewind_fx *RewindFx; // time rewinds' trails, post-process and overlays, client/rewind_fx/
    float WorldZoom; // window pixels per world unit, client/camera.cpp
    bool32 CameraPlaced; // the camera has been put on the player once; until then it cuts, client/camera.cpp
    v2 CameraVelocity; // world units per second, the camera spring's speed, client/camera.cpp
    v2 CameraLook; // eased lean, lead and killer drift added to the player, client/camera.cpp
    struct talent_requests *TalentRequests; // talents clicked, on their way to the simulation, client/talent_requests.cpp
    struct talent_panel *TalentPanel; // the talent tree screen (N), ui/talent_panel/
    struct xp_bar *XpBar; // experience bar, level badge and level-up banner, ui/talent_panel/xp_bar.cpp
    float ScreenEdgeLastHp; // the local player's health last frame, to spot hits, client/screen_edge.cpp
    float ScreenEdgeHurt; // red flash at the screen edge after a hit, 1 fading to 0, client/screen_edge.cpp
    float ScreenEdgeClock; // seconds, drives the low-health pulse, client/screen_edge.cpp
    u32 ScreenEdgeLastKills; // the local player's kills last frame plus one (0: not seen yet), client/screen_edge.cpp
    float ScreenEdgeKill; // gold flash at the screen edge after a kill, 1 fading to 0, client/screen_edge.cpp
    struct world_bloom *WorldBloom; // the glow's quarter-size textures, client/world_bloom.cpp
    struct world_grade *WorldGrade; // the world pass's render target for the glow and colour grade, client/world_grade.cpp
    u32 KeyboardLayout; // keyboard_layout, AZERTY or QWERTY, client/keyboard_layout.cpp
    bool32 OptionsOpen; // the Esc options menu is showing, ui/options_menu.cpp
    u32 VoteRequest; // a map vote request not yet sent (map_vote_request), client/vote_requests.cpp
    u32 VoteHolding; // the vote field held in the input now, 0 for none, client/vote_requests.cpp
    float VoteHoldLeft; // seconds the vote field stays held, client/vote_requests.cpp
    float VoteGapLeft; // seconds the vote field stays empty before the next request, client/vote_requests.cpp
    struct ground_cracks *GroundCracks; // broken ground where Launch and slams hit, client/ground_cracks.cpp
    struct cursor_targeting *CursorTargeting; // the unit the cursor is on and its brackets, client/targeting.cpp
    struct cast_targeting *CastTargeting; // quick or standard cast, and the ability being aimed, client/cast_targeting.cpp
