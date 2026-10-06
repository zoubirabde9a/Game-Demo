#!/bin/bash
# One-time setup of the download site the launcher updates from:
#   deploy/setup_downloads.sh user@host game.example.com
# Run it again after changing deploy/web/Caddyfile; it is safe to repeat.
#
# The machine's ports 80 and 443 belong to another project's Caddy (on
# vps-eu: work-app-caddy, config /opt/work-app/deploy/Caddyfile). This
# script does not touch that project's containers. It
#   1. starts a small file server container, game-demo-web, that serves
#      /opt/game-demo/web on that Caddy's Docker network, and
#   2. appends one site block for the domain to that Caddyfile (after a
#      dated backup) and reloads it, which gets the HTTPS certificate.
# The domain's DNS must already point at the machine: a Caddy that keeps
# failing to get a certificate counts against Let's Encrypt's limits.
# The names below can be overridden for another machine.
set -euo pipefail

TARGET="${1:?usage: deploy/setup_downloads.sh user@host domain}"
DOMAIN="${2:?usage: deploy/setup_downloads.sh user@host domain}"
EDGE_CONTAINER="${EDGE_CONTAINER:-work-app-caddy}"
EDGE_CADDYFILE="${EDGE_CADDYFILE:-/opt/work-app/deploy/Caddyfile}"
EDGE_NETWORK="${EDGE_NETWORK:-work-app_default}"
cd "$(dirname "$0")/.."

HOST_IP="$(ssh "$TARGET" "curl -4 -s --max-time 10 https://api.ipify.org || hostname -I | cut -d' ' -f1")"
DNS_IP="$(ssh "$TARGET" "getent ahostsv4 '$DOMAIN' | head -1 | cut -d' ' -f1" || true)"
if [ "$DNS_IP" != "$HOST_IP" ]; then
    echo "[downloads] $DOMAIN resolves to '${DNS_IP:-nothing}', not this machine ($HOST_IP)." >&2
    echo "[downloads] add a DNS A record $DOMAIN -> $HOST_IP, wait for it to show up, and run this again." >&2
    exit 1
fi

SUDO=""
[ "$(ssh "$TARGET" id -u)" = 0 ] || SUDO="sudo"
ssh "$TARGET" "$SUDO mkdir -p /opt/game-demo/web/files /opt/game-demo/web/manifests /opt/game-demo/web-caddy"
ssh "$TARGET" "$SUDO tee /opt/game-demo/web-caddy/Caddyfile > /dev/null" < deploy/web/Caddyfile

ssh "$TARGET" "$SUDO bash -s -- '$DOMAIN' '$EDGE_CONTAINER' '$EDGE_CADDYFILE' '$EDGE_NETWORK'" <<'EOF'
set -euo pipefail
DOMAIN="$1"; EDGE="$2"; EDGE_FILE="$3"; NETWORK="$4"
say() { echo "[downloads] $*"; }

# The file server. Recreated each run so a changed Caddyfile applies.
docker rm -f game-demo-web >/dev/null 2>&1 || true
docker run -d --name game-demo-web --restart unless-stopped \
    --network "$NETWORK" --memory 64m \
    -v /opt/game-demo/web:/srv:ro \
    -v /opt/game-demo/web-caddy/Caddyfile:/etc/caddy/Caddyfile:ro \
    caddy:2 >/dev/null
say "file server game-demo-web is up on $NETWORK"

if grep -q "^$DOMAIN {" "$EDGE_FILE"; then
    say "$EDGE_FILE already serves $DOMAIN"
else
    BACKUP="$EDGE_FILE.bak-$(date +%Y%m%d-%H%M%S)"
    cp "$EDGE_FILE" "$BACKUP"
    # Appended, never rewritten: the file is bind-mounted into the Caddy
    # container, and replacing it (sed -i) would leave Caddy on the old copy.
    cat >> "$EDGE_FILE" <<BLOCK

# Game Demo downloads: the launcher (GameDemo.exe) and the game builds it
# installs. Served by the game-demo-web container from /opt/game-demo/web;
# added by deploy/setup_downloads.sh in the Game-Demo repository. To remove:
# delete this block, reload, and docker rm -f game-demo-web.
$DOMAIN {
	encode zstd gzip
	reverse_proxy game-demo-web:80
}
BLOCK
    if ! docker exec "$EDGE" caddy validate --config /etc/caddy/Caddyfile >/dev/null 2>&1; then
        cat "$BACKUP" > "$EDGE_FILE"
        say "the edited Caddyfile did not validate; put the old one back" >&2
        exit 1
    fi
    say "added $DOMAIN to $EDGE_FILE (backup: $BACKUP)"
fi
docker exec "$EDGE" caddy reload --config /etc/caddy/Caddyfile
say "reloaded $EDGE"
EOF

for _ in 1 2 3 4 5 6; do
    if curl -sS -o /dev/null "https://$DOMAIN/" 2>/dev/null; then
        echo "[downloads] https://$DOMAIN/ answers"
        exit 0
    fi
    sleep 5
done
echo "[downloads] https://$DOMAIN/ does not answer yet (the certificate can take a minute); check: docker logs $EDGE_CONTAINER" >&2
