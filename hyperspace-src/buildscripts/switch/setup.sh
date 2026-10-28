#!/bin/bash
# Preparación del entorno para compilar Hyperspace para Nintendo Switch.
# Instala vcpkg, copia el toolchain y el triplet de Switch.
#
# Requisitos previos:
#   - devkitPro con devkitA64 y libnx instalados
#   - cmake, ninja-build
#   - swig, lua5.3+ con lpeg y lfs (para pre-generar, ver pre-generate.sh)
#   - FTL_LIBCXX: la compilación de libc++ del port Switch
#
# Uso: buildscripts/switch/setup.sh

set -e
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)

echo "=== Setup de Hyperspace para Nintendo Switch ==="

# 1. Verificamos que devkitPro esté disponible
if [ -z "$DEVKITPRO" ]; then
    echo "ERROR: DEVKITPRO no está definido. Instala devkitPro y exporta la variable."
    echo "  export DEVKITPRO=/opt/devkitpro"
    exit 1
fi

if [ ! -d "$DEVKITPRO/devkitA64" ]; then
    echo "ERROR: devkitA64 no encontrado en $DEVKITPRO/devkitA64"
    echo "  Instálalo con: (dkp-)pacman -S devkitA64 libnx switch-tools switch-mesa switch-zlib"
    exit 1
fi

echo "[1/3] devkitPro encontrado en $DEVKITPRO"

# 2. Clonamos vcpkg si no existe
if [ ! -d "$REPO_ROOT/vcpkg" ]; then
    echo "[2/3] Clonando vcpkg ..."
    git clone https://github.com/microsoft/vcpkg.git "$REPO_ROOT/vcpkg"
else
    echo "[2/3] vcpkg ya existe"
fi
(cd "$REPO_ROOT/vcpkg" && ./bootstrap-vcpkg.sh -disableMetrics)

# 3. Copiamos el toolchain y el triplet de Switch a vcpkg
echo "[3/3] Instalando toolchain y triplet de Switch en vcpkg ..."
mkdir -p "$REPO_ROOT/vcpkg/scripts/toolchains"
mkdir -p "$REPO_ROOT/vcpkg/triplets"
cp -f "$REPO_ROOT/.devcontainer/toolchains/arm64-switch-ftl.cmake" \
   "$REPO_ROOT/vcpkg/scripts/toolchains/"
cp -f "$REPO_ROOT/.devcontainer/triplets/arm64-switch-ftl.cmake" \
   "$REPO_ROOT/vcpkg/triplets/"

echo ""
echo "=== Setup completado ==="
echo "Próximos pasos:"
echo "  1. Pre-genera los archivos: buildscripts/switch/pre-generate.sh"
echo "  2. Compila: FTL_LIBCXX=<ruta> buildscripts/switch/build.sh [Release|Debug]"
