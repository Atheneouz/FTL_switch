#!/bin/bash
# Pre-generación de archivos fuente en host (PC/Linux/macOS) para la compilación
# cruzada de Hyperspace para Nintendo Switch.
#
# Estos archivos normalmente los genera CMake durante la compilación, pero como
# el toolchain cruzado de devkitA64 no dispone de lua, swig ni bash con las
# dependencias necesarias, los generamos aquí en la máquina host y luego la
# compilación cruzada los consume como archivos ya existentes.
#
# Uso: buildscripts/switch/pre-generate.sh
# Requisitos en host: lua5.3+ con lpeg y lfs, swig, bash, git

set -e
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)

echo "=== Pre-generación de fuentes para Hyperspace (Switch) ==="

# 1. Generamos las definiciones de hooks ZHL para todas las plataformas
# (solo necesitamos iOSARM64, pero el script genera todas a la vez)
echo "[1/3] Generando definiciones ZHL (FTLGame*.h / FTLGame*.cpp) ..."
cd "$REPO_ROOT"
export LUA_PATH="/usr/share/lua/5.3/?.lua;/usr/share/lua/5.3/?/init.lua;/usr/local/share/lua/5.3/?.lua;/usr/local/share/lua/5.3/?/init.lua;./?.lua;$LUA_PATH;;"
export LUA_CPATH="/usr/lib/x86_64-linux-gnu/lua/5.3/?.so;/usr/lib/lua/5.3/?.so;/usr/local/lib/lua/5.3/?.so;./?.so;$LUA_CPATH;;"
LUA="${LUA:-lua}" bash libzhlgen/parsefuncs.sh
echo "      -> generated/FTLGameiOSARM64.{h,cpp} listo"

# 2. Generamos Version.autogen.hpp
echo "[2/3] Generando Version.autogen.hpp ..."
export HS_BUILD_HASH="${HS_BUILD_HASH:-$(git rev-parse HEAD 2>/dev/null || echo "switch")}"
export HS_BUILD_BRANCH="${HS_BUILD_BRANCH:-switch}"
bash buildscripts/tools/generateVersion.sh
echo "      -> generated/Version.autogen.hpp listo"

# 3. Generamos swigluarun.h (runtime de SWIG para Lua)
# Lo colocamos en generated/ dentro del repositorio para que la compilación
# cruzada lo encuentre sin necesidad de ejecutar swig en Switch.
echo "[3/3] Generando swigluarun.h (runtime SWIG) ..."
SWIG_OUT_DIR="$REPO_ROOT/generated"
mkdir -p "$SWIG_OUT_DIR"
swig -c++ -lua -external-runtime "$SWIG_OUT_DIR/swigluarun.h.temp1"
bash buildscripts/tools/processSwigRuntime.sh "$SWIG_OUT_DIR"
echo "      -> generated/swigluarun.h listo"

echo ""
echo "=== Pre-generación completada ==="
echo "Ahora puedes compilar con buildscripts/switch/build.sh"
