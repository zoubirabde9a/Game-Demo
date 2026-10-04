# Running the server on a Linux machine

One command from your computer, once the machine exists:

```
deploy/deploy.sh user@203.0.113.7
deploy/deploy.sh vps-eu          # an alias from ~/.ssh/config works too
```

The live server today is `vps-eu` (152.53.147.77, ARM, Debian 13), next to other apps' Docker containers; the game uses only its own service, user, folder and UDP port.

It sends the committed code, builds it on the server, installs it as the `game-demo` service, and checks that a player can join. If the check fails, the previous release goes back up and the command fails. Players connect to UDP port 27015.

What the machine needs: Ubuntu or Debian on x86 or ARM, ssh access as root or a user with sudo, and UDP 27015 open in the hosting provider's firewall (the install opens it in ufw). The install puts in `g++` itself if it is missing. The smallest plan is enough. Measured on vps-eu with 8 bot players fighting over the internet: ticks average 0.36 ms and peak 4.3 ms of their 16.7 ms, no late ticks, every player gets 19.9 of 20 snapshots a second, and the server sends about 7 KB/s per player (at most 24 KB/s: 20 snapshots of under 1200 bytes).

## Before a deploy

Run `misc\linux_check.ps1` (needs Docker). It builds the server with `build_server.sh` in a Linux container, as the deploy does, and runs the network, server and soak tests there under AddressSanitizer. The content id it prints must match the Windows build's. About a minute and a half.

## Testing a live server from your computer

Build the tools with `build_server.bat` (Windows) or `build_server.sh`. The content id is in the server's first log line, `journalctl -u game-demo | grep listening`; a game client from the same commit has the same one.

| Check | Command |
|---|---|
| Who is playing, without joining | `build\probe.exe --info 152.53.147.77:27015` (build, map, player count and names) |
| A player can join | `build\probe.exe 152.53.147.77:27015` |
| A game build with this content id is let in | `build\probe.exe 152.53.147.77:27015 <content-id>` (exit 5: different build) |
| Load: 8 players for 75 s | `build\bots.exe 152.53.147.77:27015 <content-id> 8 75`, then read the stats line in the server log |

What a healthy run looks like (local debug server, Old Arena, 2026-10-03, protocol GDMA): all 8 bots stay connected at 20 snapshots a second; the minute's stats line reads about `tick avg 2.2 ms max 12.5 ms of 16.7, 0 late`, `0 bad` packets in, and about 70 KB/s out (9 KB/s per player; 78 before GDMF left out zero height and velocity). Clients send one input packet per frame, so inbound grows with their frame rate (about 41 KB/s for 8 players at 60 fps). The live server is built with `-O2`, so its ticks should be several times faster; late ticks or bad packets there are worth a look.

## On the server

| Task | Command |
|---|---|
| Is it up? | `systemctl status game-demo` or `/opt/game-demo/current/probe` |
| Who joined and left | `journalctl -u game-demo -f` |
| Change the port | edit `/etc/game-demo/server.env`, then `sudo systemctl restart game-demo` |
| Bots, or another map | add `SERVER_ARGS=--bots 4` (or `--map keep --bots 2`) to `/etc/game-demo/server.env`, then `sudo systemctl restart game-demo`; the service file needs to be the one from this commit or later |
| Name the server | add `--name EU-1` to `SERVER_ARGS` (no spaces: the service splits the line on them); players see it in the server list and HUD instead of the name in their own list |
| Which build is live | `journalctl -u game-demo \| grep listening \| tail -1` (content id) and `readlink /opt/game-demo/current` (commit) |
| Remove it completely | `systemctl disable --now game-demo; rm -rf /opt/game-demo /etc/game-demo /etc/systemd/system/game-demo.service; userdel gameserver; ufw delete allow 27015/udp` |
| Roll back by hand | `ls /opt/game-demo/releases`, then `sudo ln -sfn /opt/game-demo/releases/<name> /opt/game-demo/current && sudo systemctl restart game-demo` |

The service restarts the server 2 seconds after a crash and starts it at boot. Stopping it tells every connected player the server is closing.

Health checks from anywhere: `build/probe host:27015` exits 0 when a player could join, 2 when nothing answers, 3 when the server is full, 5 when it runs a different game build than the content id given.
