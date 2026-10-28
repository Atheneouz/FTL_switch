#!/bin/bash
# Compilación de libHyperspace.a para FTL for iPad ejecutado en Nintendo Switch.
#   DEVKITPRO=... FTL_LIBCXX=... buildscripts/switch/build.sh [Release|Debug]
#
# Antes de ejecutar esto:
#   1. buildscripts/switch/setup.sh    (una sola vez)
#   2. buildscripts/switch/pre-generate.sh  (cada vez que cambian las definiciones ZHL)

set -e
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
BUILD_TYPE="${1:-Release}"
BUILD_DIR="$REPO_ROOT/build-switch-$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"

: "${DEVKITPRO:?set DEVKITPRO to the devkitPro installation (e.g. /opt/devkitpro)}"
: "${FTL_LIBCXX:?set FTL_LIBCXX to the libc++ build of the Switch port (see README.md)}"
export DEVKITPRO FTL_LIBCXX

# Verificamos que los archivos pre-generados existan
for f in generated/FTLGameiOSARM64.h generated/FTLGameiOSARM64.cpp \
         generated/Version.autogen.hpp generated/swigluarun.h; do
    if [ ! -f "$REPO_ROOT/$f" ]; then
        echo "ERROR: $f no encontrado. Ejecuta primero: buildscripts/switch/pre-generate.sh"
        exit 1
    fi
done

# Nos aseguramos de que las cabeceras de Boost estén aisladas en third_party/boost
if [ ! -f "$REPO_ROOT/third_party/boost/boost/version.hpp" ] && [ -d "/usr/include/boost" ]; then
    mkdir -p "$REPO_ROOT/third_party/boost"
    cp -a /usr/include/boost "$REPO_ROOT/third_party/boost/"
fi

NINJA_BIN=$(which ninja 2>/dev/null || which ninja-build 2>/dev/null || echo "ninja")

cd "$REPO_ROOT"
echo "Configurando CMake con Ninja para Nintendo Switch..."
cmake -DCMAKE_TOOLCHAIN_FILE="$REPO_ROOT/.devcontainer/toolchains/arm64-switch-ftl.cmake" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_MAKE_PROGRAM="$NINJA_BIN" \
    -DNINTENDO_SWITCH=ON \
    -S . -B "$BUILD_DIR" -G Ninja

echo "Compilando Hyperspace con $NINJA_BIN..."
"$NINJA_BIN" -C "$BUILD_DIR"
echo "Built: $BUILD_DIR/libHyperspace.a"
