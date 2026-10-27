if(NOT _FTL_TOOLCHAIN)
set(_FTL_TOOLCHAIN 1)

# Hyperspace para Nintendo Switch (FTL for iPad ejecutado por el port Switch).
# Compilamos como librería estática (libHyperspace.a) enlazada en ftl.nro.
#
# Requiere devkitA64 de devkitPro y libnx.
#
# DEVKITPRO:   raíz de devkitPro (normalmente /opt/devkitpro)
# DEVKITARM64: raíz de devkitA64 (normalmente $DEVKITPRO/devkitA64)
# FTL_LIBCXX:  salida de libcxx/build.sh switch (include/ y lib/)

foreach(var DEVKITPRO FTL_LIBCXX)
    if(NOT DEFINED ENV{${var}})
        message(FATAL_ERROR "${var} is not set (see buildscripts/switch/README.md)")
    endif()
endforeach()

set(DEVKITPRO "$ENV{DEVKITPRO}")
if(DEFINED ENV{DEVKITARM64})
    set(DEVKITARM64 "$ENV{DEVKITARM64}")
else()
    set(DEVKITARM64 "${DEVKITPRO}/devkitA64")
endif()

# Compilador cruzado de devkitA64
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "aarch64")

set(CMAKE_C_COMPILER "${DEVKITARM64}/bin/aarch64-none-elf-gcc")
set(CMAKE_CXX_COMPILER "${DEVKITARM64}/bin/aarch64-none-elf-g++")
set(CMAKE_AR "${DEVKITARM64}/bin/aarch64-none-elf-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${DEVKITARM64}/bin/aarch64-none-elf-ranlib" CACHE FILEPATH "Ranlib")

# Flags de arquitectura Switch (Cortex-A57, ARMv8-A)
set(_switch_arch_flags "-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIC")

# Donde están las cabeceras de libnx y portlibs
set(_switch_sysroot_flags
    "-isystem ${DEVKITPRO}/libnx/include -isystem ${DEVKITPRO}/portlibs/switch/include")

set(FTL_LIBCXX "$ENV{FTL_LIBCXX}" CACHE PATH "The Switch port's libc++ build (include/ and lib/)")
set(_ftl_libcxx "${FTL_LIBCXX}")

# Donde Apple's arm64 ABI difiere de la estándar y la diferencia cruza al juego:
# char es signed; constructores devuelven `this` con cookie ARM
# (-fc++-abi=arm, como el libc++ del port está compilado);
# RTTI y tablas ctype usan la disposición de Apple.
string(CONCAT _ftl_c_flags
    " -fsigned-char"
    " -fPIC"
    " -D__SWITCH__=1"
    " ${_switch_arch_flags}"
    " ${_switch_sysroot_flags}"
)
string(CONCAT _ftl_cxx_flags
    " -nostdinc++ -isystem ${_ftl_libcxx}/include/c++/v1"
    " -fc++-abi=arm"
    " -D_LIBCPP_TYPEINFO_COMPARISON_IMPLEMENTATION=3"
    " -D_LIBCPP_DARWIN_CTYPE_ABI"
)

foreach(lang C CXX)
    set(CMAKE_${lang}_FLAGS_DEBUG_INIT "-DDEBUG -g")
    set(CMAKE_${lang}_FLAGS_RELEASE_INIT "-DNDEBUG -O2")
    set(CMAKE_${lang}_FLAGS_MINSIZEREL_INIT "-DNDEBUG -Os")
    set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO_INIT "-DNDEBUG -O2 -g")
    string(APPEND CMAKE_${lang}_FLAGS "${_ftl_c_flags}")
endforeach()
string(APPEND CMAKE_CXX_FLAGS "${_ftl_cxx_flags}")

# Linkamos con la libc++ del port en lugar de la estándar
foreach(kind EXE SHARED MODULE STATIC)
    string(APPEND CMAKE_${kind}_LINKER_FLAGS
        " -L${DEVKITPRO}/libnx/lib -L${DEVKITPRO}/portlibs/switch/lib")
endforeach()

# Búsqueda de paquetes solo dentro de sysroot/portlibs
list(APPEND CMAKE_PREFIX_PATH
    "${DEVKITPRO}/portlibs/switch"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
list(APPEND CMAKE_FIND_ROOT_PATH
    "${DEVKITPRO}/portlibs/switch"
    "${DEVKITPRO}/libnx"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

endif()
