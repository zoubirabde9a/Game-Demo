# Running the server on a Linux machine

One command from your computer, once the machine exists:

```
deploy/deploy.sh user@203.0.113.7
```

It sends the committed code, builds it on the server, installs it as the `game-demo` service, and checks that a player can join. If the check fails, the previous release goes back up and the command fails. Players connect to UDP port 27015.

What the machine needs: Ubuntu or Debian, ssh access for a user with sudo, and UDP 27015 open in the hosting provider's firewall. The install puts in `g++` itself if it is missing. The smallest plan is enough: with 8 players fighting, a tick takes 0.05 ms on average and 0.4 ms at worst of its 16.7 ms, and the server sends each player at most 24 KB/s (20 snapshots a second, each under 1200 bytes).

## On the server

| Task | Command |
|---|---|
| Is it up? | `systemctl status game-demo` or `/opt/game-demo/current/probe` |
| Who joined and left | `journalctl -u game-demo -f` |
| Change the port | edit `/etc/game-demo/server.env`, then `sudo systemctl restart game-demo` |
| Roll back by hand | `ls /opt/game-demo/releases`, then `sudo ln -sfn /opt/game-demo/releases/<name> /opt/game-demo/current && sudo systemctl restart game-demo` |

The service restarts the server 2 seconds after a crash and starts it at boot. Stopping it tells every connected player the server is closing.

Health checks from anywhere: `build/probe host:27015` exits 0 when a player could join, 2 when nothing answers, 3 when the server is full.
