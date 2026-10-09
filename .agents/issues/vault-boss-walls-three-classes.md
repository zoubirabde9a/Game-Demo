found-by: agent/wildtrees
files: code/sim/dungeon/role_kits/stormcaller*, code/sim/dungeon/role_kits/duelist*, code/sim/dungeon/role_kits/ranger*, code/server/bots/stormcaller.cpp, code/server/bots/duelist.cpp, code/server/bots/ranger.cpp
since: 2026-10-09
what: in the balance probe a party whose damage bot is a Stormcaller or a Duelist almost never kills the Rimeheart Throne (the vault's boss): over 24 seeds the Stormcaller party wiped 2624 times for 2 kills, the Duelist party never got there (stuck at Shiver Hall in 8 seeds, 11-16 wipes a kill at the Throne of Embers). A Ranger party kills it but wipes 47-75 times a kill. Fire Mage, Frost Mage and Shadowblade parties wipe 1-8 times a kill there. Measured before 1b45f9f (Duelist tuning), with class talents only.
reproduce: set PROBE_TREE=class& set PROBE_LEVELS=4& set GAME_BOT_DAMAGE=stormcaller& misc\balance.bat 400 3 2 24 (likewise duelist, ranger)
fix: unknown; boss times for these three are also 1.5-2x the Fire Mage's at the first two bosses, so it may be kit damage on single targets or the bots standing in the boss's attacks
