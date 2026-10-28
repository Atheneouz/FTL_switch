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

echo "[1/3] devkitPro encontrado en $DEVKITPRO"

# 2. Descargamos las fuentes de Lua 5.3.6 si no están presentes
echo "[2/3] Verificando fuentes de Lua 5.3 para Switch..."
mkdir -p "$REPO_ROOT/third_party"
if [ ! -d "$REPO_ROOT/third_party/lua-5.3.6" ]; then
    echo "  Descargando Lua 5.3.6..."
    curl -sL https://www.lua.org/ftp/lua-5.3.6.tar.gz | tar -xz -C "$REPO_ROOT/third_party"
    echo "  -> Lua 5.3.6 preparado en third_party/lua-5.3.6"
else
    echo "  -> Lua 5.3.6 ya está disponible en third_party/lua-5.3.6"
fi

# 3. Verificamos cabeceras de Boost aisladas (sin polucionar con cabeceras glibc del host)
echo "[3/3] Verificando cabeceras de Boost para Switch..."
mkdir -p "$REPO_ROOT/third_party/boost"
if [ ! -f "$REPO_ROOT/third_party/boost/boost/version.hpp" ]; then
    if [ -d "/usr/include/boost" ]; then
        echo "  Copiando cabeceras de Boost desde /usr/include/boost a third_party/boost/boost..."
        cp -a /usr/include/boost "$REPO_ROOT/third_party/boost/"
    elif [ -d "$DEVKITPRO/portlibs/switch/include/boost" ]; then
        echo "  Copiando cabeceras de Boost desde portlibs..."
        cp -a "$DEVKITPRO/portlibs/switch/include/boost" "$REPO_ROOT/third_party/boost/"
    else
        echo "  Descargando cabeceras de Boost..."
        curl -sL https://archives.boost.io/release/1.83.0/source/boost_1_83_0.tar.gz | tar -xz -C /tmp
        mv /tmp/boost_1_83_0/boost "$REPO_ROOT/third_party/boost/"
        rm -rf /tmp/boost_1_83_0
    fi
    echo "  -> Cabeceras de Boost preparadas en third_party/boost"
else
    echo "  -> Cabeceras de Boost ya disponibles en third_party/boost"
fi

echo ""
echo "=== Setup completado con éxito ==="
echo "Próximos pasos:"
echo "  1. Pre-genera los archivos: buildscripts/switch/pre-generate.sh"
echo "  2. Compila: FTL_LIBCXX=<ruta> buildscripts/switch/build.sh [Release|Debug]"
