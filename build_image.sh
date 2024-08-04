#!/usr/bin/env bash

set -euo pipefail
# set -x

# docker buildx create --driver docker-container --use

docker buildx build --push --platform linux/arm/v7,linux/arm64/v8,linux/amd64 --tag danwanderson/broken_shadows:latest --tag danwanderson/broken_shadows:$(date +"%Y%m%d") .

