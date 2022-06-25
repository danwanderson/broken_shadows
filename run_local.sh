#!/usr/bin/env bash

set -euo pipefail
# set -x

docker run --rm -it --publish 4000:4000 --entrypoint /bin/bash danwanderson/broken_shadows:latest

sleep 10s

IPADDR=$(ip addr show | grep "inet " | grep -v "127.0.0.1" | awk '{print $2}' | awk -F '/' '{print $1}')

telnet "${IPADDR}" 4000
