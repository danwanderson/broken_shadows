#!/usr/bin/env bash

set -euo pipefail

FULL=false

usage() {
    cat << EOF
Usage: $(basename "$0") [OPTIONS]

Update the Broken Shadows server.

OPTIONS:
    --full      Full update: backup, pull new image, restart container
                (default: copyover update — pull binary only, no restart)
    -h, --help  Show this help message

DEFAULT (copyover):
    Pulls the latest binary from the registry and drops it into ./bin/shadows.
    Trigger a hot reboot in-game with the 'reboot' command — no disconnect.

--full:
    Creates a tar backup, pulls the new image, restarts the container.
    Players will be disconnected during the restart.

EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --full)
            FULL=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Error: Unknown option '$1'"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

REGISTRY_BASE=$(docker compose config | awk '/^\s+image:/{print $2; exit}' | cut -d: -f1)
if [[ -z "$REGISTRY_BASE" ]]; then
    echo "Error: could not determine image from docker-compose config"
    exit 1
fi
REGISTRY_IMAGE="${REGISTRY_BASE}:latest"

if [[ "$FULL" == true ]]; then
    DATE=$(date +%Y%m%d_%H%M)
    echo "Creating backup: ~/shadows.${DATE}.tar ..."
    tar cvf ~/shadows."${DATE}".tar *

    echo "Pulling new image and restarting container..."
    docker compose down
    docker compose pull
    docker compose up -d
    docker image prune --force
    echo "✓ Full update complete."
else
    echo "Pulling binary from $REGISTRY_IMAGE ..."
    docker pull "$REGISTRY_IMAGE"
    CONTAINER_ID=$(docker create "$REGISTRY_IMAGE")
    docker cp "${CONTAINER_ID}:/srv/shadows/bin/shadows" ./bin/shadows
    docker rm "$CONTAINER_ID"
    chmod +x ./bin/shadows
    echo "✓ Binary updated at ./bin/shadows"
    echo ""
    echo "Trigger the hot reboot in-game with:  reboot"
fi
