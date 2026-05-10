#!/usr/bin/env bash
# This script is for a development with rpi related hardware backends.
# Rapberry Pi 3A+ is not as fast as a modern ARM64 Laptop and has
# rpi in the Cube has limited RAM (512MB)

set -euo pipefail

# ── Configuration ──────────────────────────────────────────────────────────
CUBE_HOST="${CUBE_LOCAL:-cub.local}"
CUBE_USER="${CUBE_USER:-cube}"
BUILDER_IMAGE="ghcr.io/bjoernh/matrixserver-builder:latest"
CCACHE_VOLUME="matrixserver-builder-ccache"
REMOTE_DEPLOY_DIR="/tmp/matrixserver-deploy"
MATRIX_SERVER_FOLDER="server"
MATRIX_SERVER_NAME="matrix_server"

# ── Helpers ────────────────────────────────────────────────────────────────
log()  { printf '[build-deploy] %s\n' "$*"; }
die()  { printf '[build-deploy] ERROR: %s\n' "$*" >&2; exit 1; }

require_cmd() {
    command -v "$1" >/dev/null 2>&1 || die "'$1' not found in PATH"
}

# ── Arguments ──────────────────────────────────────────────────────────────
CLEANUP_ONLY=false
if [[ "${1:-}" == "--cleanup" ]]; then
    CLEANUP_ONLY=true
fi

# ── Prerequisites ──────────────────────────────────────────────────────────
require_cmd scp
require_cmd ssh

if [ "$CLEANUP_ONLY" = true ]; then
    log "Removing deployed artifacts from ${CUBE_USER}@${CUBE_HOST} …"
    ssh -i ~/.ssh/private "${CUBE_USER}@${CUBE_HOST}" bash -s << 'REMOTE_SCRIPT'
set -x
sudo systemctl stop matrix_server.service || true
sudo systemctl disable matrix_server.service || true
sudo rm -f /usr/bin/matrix_server /usr/bin/MainMenu
sudo rm -f /usr/lib/libmatrixapplication.so*
sudo rm -f /usr/lib/libFPGAFTDIRenderer.so*
sudo rm -f /usr/lib/libFPGASPIRenderer.so*
sudo rm -f /usr/lib/libRGBMatrixRenderer.so*
sudo rm -f /usr/lib/libImu.so*
sudo rm -f /usr/lib/libwiringPi.so*
sudo rm -f /usr/lib/libwiringPiDev.so*
sudo rm -f /lib/systemd/system/matrix_server.service
sudo rm -f /etc/default/matrix_server
sudo rm -rf /tmp/matrixserver-deploy
sudo systemctl daemon-reload
sudo ldconfig
echo "Cleanup complete"
REMOTE_SCRIPT
    log "Cleanup successful"
    exit 0
fi

require_cmd docker

# ── Repo root ──────────────────────────────────────────────────────────────
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

# ── Submodule check ────────────────────────────────────────────────────────
RGB_MATRIX_SUBMOD="renderer/RGBMatrixRenderer/rpi-rgb-led-matrix"
if [ ! -f "${RGB_MATRIX_SUBMOD}/Makefile" ]; then
    log "Submodule not initialized — running 'git submodule update --init --recursive'"
    git submodule update --init --recursive
else
    log "Submodules already initialized"
fi

# ── Builder image ──────────────────────────────────────────────────────────
if ! docker image inspect "$BUILDER_IMAGE" >/dev/null 2>&1; then
    log "Builder image not found locally — pulling ${BUILDER_IMAGE}"
    docker pull "$BUILDER_IMAGE"
else
    log "Builder image present: ${BUILDER_IMAGE}"
fi

# ── Build inside Docker ────────────────────────────────────────────────────
log "Building ARM64 variant (all 3 hardware backends) inside Docker"

# Mock Pi detection, configure, and build
docker run --rm \
    --volume "${REPO_ROOT}:/app" \
    --volume "${CCACHE_VOLUME}:/ccache" \
    --workdir /app \
    --env CCACHE_DIR=/ccache \
    "$BUILDER_IMAGE" \
    bash -c '
        set -euo pipefail
        # Mock Raspberry Pi detection
        mkdir -p /boot && touch /boot/LICENCE.broadcom

        # Configure
        cmake -S . -B build-pi -DCMAKE_BUILD_TYPE=Release \
            -DENABLE_FPGA_FTDI=ON \
            -DENABLE_FPGA_RPISPI=ON \
            -DENABLE_RGB_MATRIX=ON \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

        # Build
        cmake --build build-pi -- -j$(nproc)
        ccache -s
    '

log "Build complete"

# ── Gather artifacts into a staging directory ─────────────────────────────
STAGING_DIR="${REPO_ROOT}/build-pi/deploy-staging"
rm -rf "$STAGING_DIR"
mkdir -p "${STAGING_DIR}/usr/bin" "${STAGING_DIR}/usr/lib" "${STAGING_DIR}/lib/systemd/system" "${STAGING_DIR}/etc/default"

# Binaries
cp build-pi/${MATRIX_SERVER_FOLDER}/${MATRIX_SERVER_NAME} "${STAGING_DIR}/usr/bin/"
cp build-pi/MainMenu/MainMenu "${STAGING_DIR}/usr/bin/"

# Systemd service and default config
cp "${REPO_ROOT}/scripts/matrix_server.service" "${STAGING_DIR}/lib/systemd/system/"
cp "${REPO_ROOT}/scripts/matrix_server.default" "${STAGING_DIR}/etc/default/matrix_server"

# Application shared libraries
find build-pi/application -name "libmatrixapplication.so*" \
    -exec cp {} "${STAGING_DIR}/usr/lib/" \;

# Renderer shared libraries
find build-pi/renderer -name "*.so" \
    -exec cp {} "${STAGING_DIR}/usr/lib/" \;

# WiringPi libraries — extract from builder image
log "Extracting WiringPi libraries from builder image"
docker run --rm \
    --volume "${STAGING_DIR}/usr/lib:/out" \
    --entrypoint sh \
    "$BUILDER_IMAGE" \
    -c 'cp /usr/local/lib/libwiringPi.so* /usr/local/lib/libwiringPiDev.so* /out/'

# Fallback: if the exact versioned names don't exist, glob-copy from the image
if ! ls "${STAGING_DIR}/usr/lib/libwiringPi.so"* >/dev/null 2>&1; then
    log "Versioned WiringPi names not found — using glob extraction"
    docker run --rm \
        --volume "${STAGING_DIR}/usr/lib:/out" \
        --entrypoint sh \
        "$BUILDER_IMAGE" \
        -c 'cp /usr/local/lib/libwiringPi.so* /usr/local/lib/libwiringPiDev.so* /out/'
fi

# Create symlinks for unversioned .so names (ldconfig convention)
cd "${STAGING_DIR}/usr/lib"
for f in libwiringPi.so.* libwiringPiDev.so.*; do
    [ -f "$f" ] || continue
    base="${f%%.*}"
    [ -L "$base.so" ] || ln -sf "$f" "${base}.so"
done
cd "$REPO_ROOT"

log "Staging directory ready: ${STAGING_DIR}"

# ── Deploy to Pi ───────────────────────────────────────────────────────────
log "Deploying to ${CUBE_USER}@${CUBE_HOST} …"

# Create remote staging directory
ssh -i ~/.ssh/private "${CUBE_USER}@${CUBE_HOST}" "mkdir -p ${REMOTE_DEPLOY_DIR}"

# SCP the entire usr tree so paths are preserved remotely
scp -i ~/.ssh/private -r "${STAGING_DIR}/usr" "${CUBE_USER}@${CUBE_HOST}:${REMOTE_DEPLOY_DIR}/"

# Install remotely with sudo
ssh -i ~/.ssh/private "${CUBE_USER}@${CUBE_HOST}" bash -s << REMOTE_SCRIPT
set -euo pipefail
sudo cp -f ${REMOTE_DEPLOY_DIR}/usr/bin/* /usr/bin/
sudo cp -f ${REMOTE_DEPLOY_DIR}/usr/lib/* /usr/lib/
sudo ldconfig
sudo chmod +x /usr/bin/${MATRIX_SERVER_NAME} /usr/bin/MainMenu
echo "Remote installation complete"
REMOTE_SCRIPT

log "Deployed successfully to ${CUBE_USER}@${CUBE_HOST}"

# ── Cleanup ────────────────────────────────────────────────────────────────
rm -rf "$STAGING_DIR"
log "Staging directory cleaned up"
