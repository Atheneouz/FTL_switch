#!/bin/bash
# Preparación del entorno para compilar Hyperspace para Nintendo Switch.
# Descarga e instala Lua 5.3 y verifica el entorno de compilación.
#
# Requisitos previos:
#   - devkitPro con devkitA64 y libnx instalados
#   - cmake, ninja-build
#   - swig, lua5.3 con lpeg y lfs
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

echo "[1/2] devkitPro encontrado en $DEVKITPRO"

# 2. Descargamos las fuentes de Lua 5.3.6 si no están presentes
echo "[2/2] Verificando fuentes de Lua 5.3 para Switch..."
mkdir -p "$REPO_ROOT/third_party"
if [ ! -d "$REPO_ROOT/third_party/lua-5.3.6" ]; then
    echo "  Descargando Lua 5.3.6..."
    curl -sL https://www.lua.org/ftp/lua-5.3.6.tar.gz | tar -xz -C "$REPO_ROOT/third_party"
    echo "  -> Lua 5.3.6 preparado en third_party/lua-5.3.6"
else
    echo "  -> Lua 5.3.6 ya está disponible en third_party/lua-5.3.6"
fi

echo ""
echo "=== Setup completado con éxito ==="
echo "Próximos pasos:"
echo "  1. Pre-genera los archivos: buildscripts/switch/pre-generate.sh"
echo "  2. Compila: FTL_LIBCXX=<ruta> buildscripts/switch/build.sh [Release|Debug]"
