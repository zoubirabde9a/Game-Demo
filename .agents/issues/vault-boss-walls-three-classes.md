found-by: agent/wildtrees (narrowed by agent/classtrees)
files: code/server/bots/duelist.cpp, code/server/bots/berserker.cpp, code/server/bots/bot_dangers.cpp, code/sim/dungeon/vault_encounters.cpp
since: 2026-10-09
what: the Rimeheart Throne part is fixed (22d0ebf and after): bots now focus the Rime Champion, which erupted on the party while they stayed on the boss. With PROBE_MAP=vault, 12 seeds, the vault boss now costs 0.2 to 1.2 wipes a kill for every damage class probed (Fire Mage, Stormcaller, Berserker, Duelist, Ranger). Still open: the Shiver Hall walls the melee parties. Duelist party 6.7 wipes a kill there, Berserker 3.1, against 0.25 to 0.75 for Fire Mage, Stormcaller and Ranger. The melee bot dies in the middle of the big elite pack (elite brutes and bats, toads, an elite warden).
reproduce: set PROBE_MAP=vault& set GAME_BOT_DAMAGE=duelist& misc\balance.bat 300 3 2 12 (likewise berserker); PROBE_DEATHS=1 PROBE_VERBOSE=1 with 1 seed lists who killed them
fix: unknown; melee bots may need to wait for the tank to gather the pack, or keep to its edge
