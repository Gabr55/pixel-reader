#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TOOLCHAIN_DIR="$ROOT_DIR/cross-compile/miyoo-mini/union-miyoomini-toolchain"
IMAGE_NAME="miyoomini-toolchain"
VERSION="${1:-dev}"

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

command -v docker >/dev/null 2>&1 || fail "Docker is not installed or not in PATH."
docker info >/dev/null 2>&1 || fail "Docker is not running. Start Docker Desktop and try again."

if [ ! -f "$TOOLCHAIN_DIR/Makefile" ]; then
    fail "Miyoo toolchain submodule is missing. Run: git submodule update --init --recursive"
fi

echo "==> Building/checking Docker toolchain image"
if docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
    echo "    Docker image already exists: $IMAGE_NAME"
else
    TMP_DIR=$(mktemp -d)
    trap 'rm -rf "$TMP_DIR"' EXIT
    PATCHED_DOCKERFILE="$TMP_DIR/Dockerfile"

    awk '
        /RUN apt-get -y update/ {
            print "RUN sed -i '\''s|http://deb.debian.org/debian|http://archive.debian.org/debian|g'\'' /etc/apt/sources.list \\";
            print "\t&& sed -i '\''s|http://security.debian.org/debian-security|http://archive.debian.org/debian-security|g'\'' /etc/apt/sources.list \\";
            print "\t&& sed -i '\''/buster-updates/d'\'' /etc/apt/sources.list \\";
            print "\t&& echo '\''Acquire::Check-Valid-Until \"false\";'\'' > /etc/apt/apt.conf.d/99archive";
        }
        { print }
    ' "$TOOLCHAIN_DIR/Dockerfile" > "$PATCHED_DOCKERFILE"

    docker build -t "$IMAGE_NAME" -f "$PATCHED_DOCKERFILE" "$TOOLCHAIN_DIR"
fi

echo "==> Building Pixel Reader Miyoo package, version: $VERSION"
docker run --rm \
    -v "$ROOT_DIR":/root/workspace/pixel-reader \
    -w /root/workspace/pixel-reader \
    "$IMAGE_NAME" \
    /bin/bash -lc ". /root/setup-env.sh && ./cross-compile/miyoo-mini/create_packages.sh '$VERSION'"

echo ""
echo "Done."
echo "OnionOS package:"
echo "  $ROOT_DIR/build/pixel_reader_onion_v${VERSION}.zip"
echo ""
echo "Put this zip on the SD card root and unzip it there."
