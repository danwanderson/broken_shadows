#!/usr/bin/env bash

set -euo pipefail
# set -x

docker run --rm -it --publish 4000:4000 \
  --volume ${PWD}/area:/srv/shadows/area \
  --volume ${PWD}/gods:/srv/shadows/gods \
  --volume ${PWD}/log:/srv/shadows/log \
  --volume ${PWD}/notes:/srv/shadows/notes \
  --volume ${PWD}/player:/srv/shadows/player \
  --volume ${PWD}/core:/srv/shadows/core \
  --entrypoint /bin/bash danwanderson/broken_shadows:latest

#sleep 10

#IPADDR=$(ip addr show | grep "inet " | grep -v "127.0.0.1" | awk '{print $2}' | awk -F '/' '{print $1}')

#telnet "${IPADDR}" 4000
