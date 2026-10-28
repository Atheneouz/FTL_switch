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

# Compilador cruzado de devkitA64 y sistema destino
set(CMAKE_SYSTEM_NAME "Generic" CACHE STRING "")
set(CMAKE_SYSTEM_PROCESSOR "aarch64" CACHE STRING "")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY CACHE STRING "")

if(DEFINED ENV{DEVKITPRO})
    set(DEVKITPRO "$ENV{DEVKITPRO}")
else()
    set(DEVKITPRO "/opt/devkitpro")
endif()

if(DEFINED ENV{DEVKITARM64})
    set(DEVKITARM64 "$ENV{DEVKITARM64}")
else()
    set(DEVKITARM64 "${DEVKITPRO}/devkitA64")
endif()

if(DEFINED ENV{FTL_LIBCXX})
    set(FTL_LIBCXX "$ENV{FTL_LIBCXX}" CACHE PATH "The Switch port's libc++ build (include/ and lib/)")
elseif(NOT DEFINED FTL_LIBCXX)
    set(FTL_LIBCXX "/tmp/libcxx-out" CACHE PATH "The Switch port's libc++ build (include/ and lib/)")
endif()
set(_ftl_libcxx "${FTL_LIBCXX}")

# Usamos Clang-19 para compatibilidad con Apple ARM64 ABI (-fc++-abi=arm) y libc++
if(EXISTS "/usr/bin/clang-19")
    set(CMAKE_C_COMPILER "/usr/bin/clang-19" CACHE FILEPATH "C Compiler")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++-19" CACHE FILEPATH "CXX Compiler")
    set(CMAKE_ASM_COMPILER "/usr/bin/clang-19" CACHE FILEPATH "ASM Compiler")
else()
    find_program(CLANG_C_BIN NAMES clang-19 clang)
    find_program(CLANG_CXX_BIN NAMES clang++-19 clang++)
    set(CMAKE_C_COMPILER "${CLANG_C_BIN}" CACHE FILEPATH "C Compiler")
    set(CMAKE_CXX_COMPILER "${CLANG_CXX_BIN}" CACHE FILEPATH "CXX Compiler")
    set(CMAKE_ASM_COMPILER "${CLANG_C_BIN}" CACHE FILEPATH "ASM Compiler")
endif()

set(CMAKE_AR "${DEVKITARM64}/bin/aarch64-none-elf-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${DEVKITARM64}/bin/aarch64-none-elf-ranlib" CACHE FILEPATH "Ranlib")

# Variables estándar de CMake para Clang cross-compilation
set(CMAKE_C_COMPILER_TARGET "aarch64-none-elf" CACHE STRING "")
set(CMAKE_CXX_COMPILER_TARGET "aarch64-none-elf" CACHE STRING "")
set(CMAKE_ASM_COMPILER_TARGET "aarch64-none-elf" CACHE STRING "")

set(CMAKE_SYSROOT "${DEVKITARM64}/aarch64-none-elf" CACHE PATH "")
set(CMAKE_C_COMPILER_EXTERNAL_TOOLCHAIN "${DEVKITARM64}" CACHE PATH "")
set(CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN "${DEVKITARM64}" CACHE PATH "")

# Flags de arquitectura Switch y target para Clang
set(_switch_arch_flags "-march=armv8-a+crc+crypto -mtune=cortex-a57 -fPIC")
set(_switch_sysroot_flags "-isystem ${DEVKITPRO}/libnx/include -isystem ${DEVKITPRO}/portlibs/switch/include")

# Flags C y C++ comunes
set(_ftl_common_flags "-fsigned-char -fPIC -D__SWITCH__=1 -D_GNU_SOURCE=1 -D_DEFAULT_SOURCE=1 -DBOOST_PP_LIMIT_MAG=256 -DBOOST_PP_LIMIT_VARIADIC=256 -DBOOST_PP_LIMIT_TUPLE=256 -DBOOST_PP_LIMIT_SEQ=256 ${_switch_arch_flags} ${_switch_sysroot_flags}")

# Flags específicas de C++ (Apple ABI + libc++ de FTL)
set(_ftl_cxx_only_flags "-nostdinc++ -isystem ${_ftl_libcxx}/include/c++/v1 -fc++-abi=arm -D_LIBCPP_TYPEINFO_COMPARISON_IMPLEMENTATION=3 -D_LIBCPP_DARWIN_CTYPE_ABI -D_LIBCPP_PROVIDES_DEFAULT_RUNE_TABLE -D_LIBCPP_HAS_THREAD_API_PTHREAD")

set(CMAKE_C_FLAGS_INIT "${_ftl_common_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_ftl_common_flags} ${_ftl_cxx_only_flags}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT}" CACHE STRING "C Flags" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT}" CACHE STRING "CXX Flags" FORCE)

foreach(lang C CXX)
    set(CMAKE_${lang}_FLAGS_DEBUG_INIT "-DDEBUG -g")
    set(CMAKE_${lang}_FLAGS_RELEASE_INIT "-DNDEBUG -O2")
    set(CMAKE_${lang}_FLAGS_MINSIZEREL_INIT "-DNDEBUG -Os")
    set(CMAKE_${lang}_FLAGS_RELWITHDEBINFO_INIT "-DNDEBUG -O2 -g")
endforeach()

# Rutas de búsqueda de librerías y cabeceras
list(APPEND CMAKE_PREFIX_PATH
    "${DEVKITPRO}/portlibs/switch"
    "${DEVKITPRO}/libnx")
list(APPEND CMAKE_FIND_ROOT_PATH
    "${DEVKITPRO}/portlibs/switch"
    "${DEVKITPRO}/libnx")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

endif()
