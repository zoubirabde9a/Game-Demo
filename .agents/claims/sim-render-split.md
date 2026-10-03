who: claude-loop
task: plan step 3, split simulation from rendering: SimulateTick updates every entity with no drawing, sound or asset access; client draws in its own pass
files: code/app.cpp, code/entity.cpp, code/entity.h, code/sim/*, code/tests/sim_tests.cpp
since: 2026-10-03
