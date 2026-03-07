#!/usr/bin/env bash

set -euo pipefail
# set -x

# docker buildx create --driver docker-container --use
#DATE=$(date +"%Y%m%d")

cd "$(dirname "${0}")" || exit

# shellcheck disable=SC1091
source .env

# shellcheck disable=SC2086
docker buildx build --push --platform linux/arm/v7,linux/arm64/v8,linux/amd64 ${TAG_LIST} .

