#!/usr/bin/env bash

set -euo pipefail
# set -x

# docker buildx create --driver docker-container --use
DATE=$(date +"%Y%m%d")

docker buildx build --push --platform linux/arm/v7,linux/arm64/v8,linux/amd64 --tag danwanderson/broken_shadows:latest --tag danwanderson/broken_shadows:"${DATE}" --tag registry.dwa.dev:443/broken_shadows:latest --tag registry.dwa.dev:443/broken_shadows:"${DATE}" .

