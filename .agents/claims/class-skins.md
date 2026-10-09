who: agent/classskins
task: a skin per class (Fire Mage, Bulwark, Mender first), two looks each (drawn in code, LPC pack), picked after the class in the Antechamber; skins are drawing only, sent to every client
files: code/art/heroes/* (new folder), code/client/heroes/* (new), code/ui/dungeon/skin_picker.cpp (new), web/heroes/* (new), code/sim/dungeon/dungeon_slot_fields.inc, code/net/protocol.h, code/net/protocol.cpp, code/server/sim_game/dungeon.cpp, code/client/dungeon/dungeon_net.cpp, code/client/draw_entities.cpp, code/ui/dungeon/dungeon_hud.cpp
since: 2026-10-09
note: one include line each in code/art/art_module.cpp and code/client/client_module.cpp; the net format changes (new protocol id), so the server needs a redeploy when this lands
