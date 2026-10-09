who: agent/netrobust
task: online play on bad links (200 ms, packet loss): inputs resent until the server has them, a deeper server input queue, replicas that ride out runs of lost snapshots, deaths sent twice
files: code/net/protocol.h, code/net/protocol.cpp, code/net/client.h, code/net/client.cpp, code/server/input_queue.cpp, code/server/event_relay.cpp, code/client/replica_smoothing.cpp, code/client/online_pacing.cpp, code/client/online.cpp, code/client/online/world_tick.cpp, code/tests/motion_tests.cpp, code/tests/lossy_link.h, code/tests/net_tests.cpp, code/tests/server_game_tests.cpp
since: 2026-10-09
