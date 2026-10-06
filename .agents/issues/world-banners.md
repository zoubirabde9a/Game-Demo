found-by: claude-polish
files: code/sim/world.cpp, code/sim/world.h
since: 2026-10-06
what: both files open with the empty template banner ($File: $, $Date: $) instead of a comment saying what they hold, so misc\code_map.ps1 shows nothing for them. The layout check now rejects that banner everywhere else; these two are exempted only because they are in the terrain claim.
reproduce: powershell -File misc\code_map.ps1 sim
fix: replace each banner with a two-to-six-line summary (world storage: tiles, chunk lists, adding and removing entities, per sim/sim_module.cpp), then delete the two-name exemption under rule 5 in misc/layout_check.ps1.
