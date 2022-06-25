#!/usr/bin/env bash

set -euo pipefail
# set -x

IPADDR=$(ip addr show | grep "inet " | grep -v "127.0.0.1" | awk '{print $2}' | awk -F '/' '{print $1}')

telnet "${IPADDR}" 4000
