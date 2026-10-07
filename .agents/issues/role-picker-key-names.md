found-by: agent/controls (session game-demo-83)
files: sim/dungeon/roles.cpp, code/ui/dungeon/dungeon_hud.cpp
since: 2026-10-07
what: the role picker's help line has its key letters written into the text ("A Meteor   R Giant Fireball ... tree: C Detonate, V Combustion"). On QWERTY launch is on Q, and in the new mouse-moves control scheme (client/control_scheme.cpp) the four class keys are A, R, S, D on AZERTY (Q, R, S, D on QWERTY), so the line names the wrong keys.
reproduce: Esc, pick QWERTY or "Mouse moves", vote for a dungeon, read the role picker
fix: build the line from ActionKeyLabel(PlayerButton_Launch / _Push / _Slam / _Kunai) (client/action_keys.cpp) plus each spell's name, the way the controls panel does (ui/controls_panel.cpp, ControlsKeyText)
