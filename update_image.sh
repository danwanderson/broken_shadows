#!/usr/bin/env bash

DATE=$(date +%Y%m%d_%H%M)


tar cvf ~/shadows."${DATE}".tar *
docker compose down
docker compose pull
docker compose up -d
docker image prune --force
