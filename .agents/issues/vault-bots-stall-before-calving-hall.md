found-by: agent/vault-check
files: code/server/bots/bot_paths.cpp, code/sim/maps/vault.cpp
since: 2026-10-08
what: In the Rimeheart Vault, after the Shiver Hall is cleared, the bots sometimes stand at the mouth of the corridor north to the Calving Hall (about 1180, 840, room 2) for the rest of the run and never walk in. Two seeds of sixteen did it; the same happened once before the Rimeheart Throne. People are not affected; it skews the balance probe's counts.
reproduce: PROBE_MAP=vault build\dungeon_balance.exe 30 3 2 16, look for "between fights after 1750 s, the Calving Hall waits" (seeds 2 and 10 at commit 70c4f08 plus the vault retune)
fix: unknown; the corridor (layout rows 20-25, columns 37-40) is four tiles wide with gate B in it, so check how the bot path grid treats the gate tiles
