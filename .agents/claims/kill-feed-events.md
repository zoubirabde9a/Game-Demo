who: claude-netcode
task: kill events: the sim reports player deaths (who killed whom), the server sends them (protocol GDMB), the client keeps a recent-kills list for the UI to draw
files: code/sim/events.h, code/sim/entity.cpp (DamageEntity only), code/client/play_events.cpp, code/client/kill_feed.cpp (new), code/app.h (one line), code/net/protocol.*, code/server/sim_game.cpp, code/client/online.cpp, code/tests/online_tests.cpp, code/tests/net_tests.cpp, code/tests/server_tests.cpp
since: 2026-10-03
