#!/usr/bin/env bash

set -euo pipefail

# Script to build and optionally push Docker image to Docker Hub
# USAGE: ./build_image.sh [OPTIONS]
# ENVIRONMENT FILES:
#   .env           Required: TAG_LIST for local registry
#   .env.docker    Used with --docker: TAG_LIST for local + Docker Hub registries

usage() {
    cat << EOF
Usage: $(basename "$0") [OPTION]

Build Docker image for multiple platforms using buildx.

OPTIONS:
    --docker            Push to Docker Hub (sources .env.docker instead of .env)
    --platform PLAT     Override target platforms (comma-separated)
                        Default: linux/arm/v7,linux/arm64/v8,linux/amd64
    --args ARGS         Override additional buildx arguments
    --debug             Enable debug tracing (set -x)
    -h, --help          Show this help message

ENVIRONMENT FILES:
    .env                Required. Local registry configuration:
                        - TAG_LIST: Docker image tags for local registry

    .env.docker         Required when using --docker. Docker Hub configuration:
                        - TAG_LIST: Docker image tags for local + Docker Hub registries

EXAMPLES:
    ./build_image.sh                                    # Build for local registry
    ./build_image.sh --docker                           # Build and push to Docker Hub
    ./build_image.sh --platform linux/amd64,linux/arm64 # Override platforms
    ./build_image.sh --docker --args "--no-cache"       # Build and push with extra args
    ./build_image.sh --debug                            # Build with debug output

EOF
    exit 0
}

# Initialize variables
PUSH_TO_HUB=false
BUILDER_PLATFORM=""
BUILDX_ARGS=""
DEBUG=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --docker)
            PUSH_TO_HUB=true
            shift
            ;;
        --debug)
            DEBUG=true
            set -x
            shift
            ;;
        --platform)
            if [[ -z "${2:-}" ]]; then
                echo "Error: --platform requires an argument"
                exit 1
            fi
            BUILDER_PLATFORM="$2"
            shift 2
            ;;
        --args)
            if [[ -z "${2:-}" ]]; then
                echo "Error: --args requires an argument"
                exit 1
            fi
            BUILDX_ARGS="$2"
            shift 2
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

# Change to script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# Verify docker is available
if ! command -v docker &> /dev/null; then
    echo "Error: docker command not found. Please install Docker."
    exit 1
fi

# Verify docker buildx support
if ! docker buildx version &> /dev/null 2>&1; then
    echo "Error: docker buildx not available. Please enable buildx:"
    echo "  docker buildx create --driver docker-container --use"
    exit 1
fi

# Determine which env file to use
if [[ "$PUSH_TO_HUB" == true ]]; then
    ENV_FILE=".env.docker"
else
    ENV_FILE=".env"
fi

# Check for required env file
if [[ ! -f "$ENV_FILE" ]]; then
    echo "Error: $ENV_FILE file not found in $SCRIPT_DIR"
    if [[ "$PUSH_TO_HUB" == true ]]; then
        echo "Please create .env.docker with TAG_LIST for Docker Hub push."
    else
        echo "Please create .env with TAG_LIST for local registry."
    fi
    exit 1
fi

# Source build configuration
echo "Loading build configuration from $ENV_FILE..."
# shellcheck disable=SC1090
source "$ENV_FILE"

# Verify required TAG_LIST is set
if [[ -z "${TAG_LIST:-}" ]]; then
    echo "Error: TAG_LIST variable not set in $ENV_FILE"
    echo "Example: TAG_LIST='-t myrepo/myimage:v1.0 -t myrepo/myimage:latest'"
    exit 1
fi

# Set defaults for platform and args if not overridden
# Remove armv7 by default due to build time and compatibility issues, but leave it as an option for users who need it
# BUILDER_PLATFORM="${BUILDER_PLATFORM:-linux/arm/v7,linux/arm64/v8,linux/amd64}"
BUILDER_PLATFORM="${BUILDER_PLATFORM:-linux/arm64/v8,linux/amd64}"
BUILDX_ARGS="${BUILDX_ARGS:-}"

# Add --debug to buildx args if debug mode is enabled
if [[ "$DEBUG" == true ]]; then
    BUILDX_ARGS="$BUILDX_ARGS --debug"
fi

# Display build configuration
echo ""
echo "Build Configuration:"
echo "  Config File: $ENV_FILE"
echo "  Platform(s): $BUILDER_PLATFORM"
echo "  Tag(s): $TAG_LIST"
[[ -n "$BUILDX_ARGS" ]] && echo "  Extra Args: $BUILDX_ARGS"
echo ""

# Build the image
echo "Starting Docker buildx build..."
# shellcheck disable=SC2086
docker buildx build --push --platform "$BUILDER_PLATFORM" $BUILDX_ARGS $TAG_LIST .

BUILD_STATUS=$?
if [[ $BUILD_STATUS -eq 0 ]]; then
    echo ""
    echo "✓ Build completed successfully!"
    if [[ "$PUSH_TO_HUB" == false ]]; then
        echo "Hint: Use '--docker' flag to push to Docker Hub:"
        echo "  $0 --docker"
    fi
else
    echo "✗ Build failed with exit code $BUILD_STATUS"
    exit $BUILD_STATUS
fi

