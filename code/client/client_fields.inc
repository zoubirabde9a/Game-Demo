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
    struct player_fx *PlayerFx; // ability effects drawn over the world, client/player_fx.cpp
