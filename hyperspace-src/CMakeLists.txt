cmake_minimum_required(VERSION 3.16)
project(Hyperspace C CXX)

# Nintendo Switch: compilamos como librería estática enlazada en ftl.nro
option(NINTENDO_SWITCH "Build Hyperspace as a static library for Nintendo Switch" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# compile_commands.json for editors and clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Rules for generated/: hook definitions, swig runtime, version header
include(GeneratedSources)

if(APPLE)
    # Prefer static libraries for dependencies on macOS
    set(BUILD_SHARED_LIBS OFF)
endif()

if(NINTENDO_SWITCH)
    # Nintendo Switch: resolvemos dependencias de forma directa sin módulos problemáticos.
    # Aislamos las cabeceras de Boost en third_party/boost para evitar polucionar el entorno
    # con cabeceras glibc x86_64 del host (/usr/include/bits/libc-header-start.h).
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost/boost/version.hpp")
        set(Boost_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost")
    elseif(EXISTS "${DEVKITPRO}/portlibs/switch/include/boost/version.hpp")
        set(Boost_INCLUDE_DIRS "${DEVKITPRO}/portlibs/switch/include")
    elseif(EXISTS "/usr/include/boost/version.hpp")
        set(Boost_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/boost")
    else()
        message(FATAL_ERROR "No se encontraron las cabeceras de Boost (ejecuta buildscripts/switch/setup.sh)")
    endif()

    if(NOT TARGET Boost::boost)
        add_library(Boost::boost INTERFACE IMPORTED)
        target_include_directories(Boost::boost INTERFACE "${Boost_INCLUDE_DIRS}")
    endif()
    if(NOT TARGET Boost::filesystem)
        add_library(Boost::filesystem INTERFACE IMPORTED)
        target_include_directories(Boost::filesystem INTERFACE "${Boost_INCLUDE_DIRS}")
    endif()

    if(NOT TARGET ZLIB::ZLIB)
        if(EXISTS "${DEVKITPRO}/portlibs/switch/lib/libz.a")
            add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
            set_target_properties(ZLIB::ZLIB PROPERTIES
                IMPORTED_LOCATION "${DEVKITPRO}/portlibs/switch/lib/libz.a"
                INTERFACE_INCLUDE_DIRECTORIES "${DEVKITPRO}/portlibs/switch/include"
            )
        else()
            find_package(ZLIB REQUIRED)
        endif()
    endif()

    # minizip no se utiliza en Switch (módulo crash-detection excluido)
    if(NOT TARGET minizip::minizip)
        add_library(minizip::minizip INTERFACE IMPORTED)
    endif()

    # Compilamos Lua 5.3 para Switch con la misma arquitectura y ABI
    set(LUA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/lua-5.3.6/src")
    if(EXISTS "${LUA_DIR}")
        file(GLOB LUA_CORE_SRCS "${LUA_DIR}/*.c")
        list(FILTER LUA_CORE_SRCS EXCLUDE REGEX "(/lua\\.c$|/luac\\.c$|/linit\\.c$)")
        add_library(lua_switch STATIC ${LUA_CORE_SRCS})
        target_include_directories(lua_switch PUBLIC "${LUA_DIR}")
        target_compile_options(lua_switch PRIVATE -fsigned-char -fPIC -O2)
        set(LUA_LIBRARIES lua_switch)
        set(LUA_INCLUDE_DIR "${LUA_DIR}")
    else()
        find_package(Lua REQUIRED)
    endif()
else()
    # Let vcpkg handle Boost automatically
    find_package(Boost REQUIRED COMPONENTS filesystem system)

    # Let vcpkg handle Lua automatically
    find_package(Lua REQUIRED)

    # Let vcpkg handle minizip automatically
    find_package(minizip REQUIRED)
    find_package(ZLIB REQUIRED)
endif()

# SDL3: ni el iPad game en Android ni en Switch usan SDL
if(NOT ANDROID AND NOT NINTENDO_SWITCH)
    find_package(SDL3 CONFIG REQUIRED)
endif()

if(WIN32)
    find_package(OpenGL REQUIRED)
endif()

# pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# SWIG
find_package(SWIG REQUIRED COMPONENTS lua)
include(UseSWIG)

# Setup SWIG *.i files
set(
    swigfiles
        lua/modules/defines.i
        lua/modules/graphics.i
        lua/modules/hyperspace.i
        lua/modules/rapidxml.i
)
set_property(SOURCE ${swigfiles} PROPERTY USE_TARGET_INCLUDE_DIRECTORIES TRUE)
set_property(SOURCE ${swigfiles} PROPERTY CPLUSPLUS ON)
if(ANDROID OR NINTENDO_SWITCH)
    # SWIG reads the Linux struct definitions; this leaves out fields the iPad game lacks
    set_property(SOURCE ${swigfiles} APPEND PROPERTY COMPILE_DEFINITIONS HS_ANDROID)
endif()

# Name the binary as Hyperspace.dll instead of libHyperspace.dll
set(CMAKE_SHARED_LIBRARY_PREFIX_CXX "")

if(NINTENDO_SWITCH)
    set(_hs_lib_type STATIC)
else()
    set(_hs_lib_type SHARED)
endif()

swig_add_library(
    Hyperspace
    TYPE ${_hs_lib_type}
    LANGUAGE lua
    SOURCES ${swigfiles}
)

# Swig runs as part of this target and includes the generated headers
if(TARGET hs_generated)
    add_dependencies(Hyperspace hs_generated)
endif()

target_compile_features(Hyperspace PRIVATE cxx_std_11)

# src/game mirrors FTL's source folders; its headers are included by name, so names must be unique
file(GLOB_RECURSE GAME_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/game/*.h")
set(GAME_INCLUDE_DIRS "")
set(GAME_HEADER_NAMES "")
foreach(header ${GAME_HEADERS})
    get_filename_component(header_dir ${header} DIRECTORY)
    get_filename_component(header_name ${header} NAME)
    if(header_name IN_LIST GAME_HEADER_NAMES)
        message(FATAL_ERROR "More than one header under src/game is named ${header_name}")
    endif()
    list(APPEND GAME_HEADER_NAMES ${header_name})
    list(APPEND GAME_INCLUDE_DIRS ${header_dir})
endforeach()
list(REMOVE_DUPLICATES GAME_INCLUDE_DIRS)

target_include_directories(
    Hyperspace PRIVATE

    ${Boost_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}

    # Include roots; include from a root and not with "../"
    ${GAME_INCLUDE_DIRS}   # "CustomCrew.h", from every src/game folder
    src/zhl                # "zhl.h", "SigScan.h"
    src                    # "Global.h", "helpers/PALMemoryProtection.h", "features/discord/DiscordIntegration.h"
    src/switch             # "SwitchHost.h"
    src/android            # "HyperspaceHost.h", "MachO.h"
    lua                    # "LuaScriptInit.h"
    generated              # "Version.autogen.hpp", FTLGame<Platform>.*
    ${HS_SWIG_RUNTIME_DIR} # "swigluarun.h"
    third_party/detours    # "detours.h", "hde.h"

    # Note: Don't vcpkg-fy rapidxml. It's already more than a decade old and we've customized the code to avoid its bug
    #       that leads to a compilation failure. See https://sourceforge.net/p/rapidxml/bugs/16/ for more info.
    third_party/rapidxml   # "rapidxml.hpp"
)
target_link_libraries(
    Hyperspace PRIVATE
    ${LUA_LIBRARIES}
    minizip::minizip
    ZLIB::ZLIB
    Boost::filesystem
    Threads::Threads
)
if(NOT ANDROID AND NOT NINTENDO_SWITCH)
    target_link_libraries(Hyperspace PRIVATE SDL3::SDL3)
endif()

# Sources belong in src/
file(GLOB ROOT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
if(ROOT_SOURCES)
    message(FATAL_ERROR "Move these files from the repository root into src/game: ${ROOT_SOURCES}")
endif()

# All src/ sources
file(GLOB_RECURSE HS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Placed explicitly below
list(FILTER HS_SOURCES EXCLUDE REGEX "/src/(zhl/|main\\.cpp$)")
if(ANDROID)
    # SDL test feature: the iPad game has no SDL. Crash reports: the app keeps its own
    # crash notice, and saves Hyperspace's logs with the rest.
    list(FILTER HS_SOURCES EXCLUDE REGEX "/src/features/(sdl-test|crash-detection)/")
    list(FILTER HS_SOURCES EXCLUDE REGEX "/src/switch/")
elseif(NINTENDO_SWITCH)
    # Switch: sin SDL, sin crash-detection propia, sin Discord
    # Reutilizamos src/android/ (offsets iOSARM64, TouchToLua, MachO, etc.)
    # pero excluimos AndroidHost.cpp (sustituido por SwitchHost.cpp)
    list(FILTER HS_SOURCES EXCLUDE REGEX "/src/features/(sdl-test|crash-detection|discord)/")
    list(FILTER HS_SOURCES EXCLUDE REGEX "/src/android/AndroidHost\\.cpp$")
else()
    # Entry point for the Android port
    list(FILTER HS_SOURCES EXCLUDE REGEX "/src/android/")
    list(FILTER HS_SOURCES EXCLUDE REGEX "/src/switch/")
endif()

# Hooking engine
file(GLOB ZHL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zhl/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zhl/*.h"
)

# Generated hook definitions for this platform
if(WIN32)
    set(FTLGAME_PLATFORM Win32)
elseif(ANDROID OR NINTENDO_SWITCH)
    # El juego es FTL for iPad 1.6.21 (arm64 iOS build) tanto en Android como Switch
    set(FTLGAME_PLATFORM iOSARM64)
elseif(APPLE)
    set(FTLGAME_PLATFORM MacOSAMD64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FTLGAME_PLATFORM ELF64)
else()
    set(FTLGAME_PLATFORM ELF32)
endif()

# Include lua sources
file(GLOB LUA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/*.c"
)

# Enable testing - controlled by ENABLE_TESTING option
option(ENABLE_TESTING "Enable Hyperspace automated testing" OFF)

if(ENABLE_TESTING)
    # Automatically include all test files recursively
    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h"
    )
    target_compile_definitions(Hyperspace PRIVATE HYPERSPACE_TESTING)
    message(STATUS "Hyperspace Testing: ENABLED")
else()
    set(TEST_SOURCES "")
    message(STATUS "Hyperspace Testing: DISABLED")
endif()

target_sources(
    Hyperspace PRIVATE

    # Hook definitions for this platform
    generated/FTLGame${FTLGAME_PLATFORM}.h
    generated/FTLGame${FTLGAME_PLATFORM}.cpp

    # Test sources first (if enabled) - ensures TestScript hooks into the game loop first
    ${TEST_SOURCES}

    # All src/ sources
    ${HS_SOURCES}

    # all lua files
    ${LUA_SOURCES}

    # Generated files
    ${HS_SWIG_RUNTIME}
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/Version.autogen.hpp

    ${ZHL_SOURCES}

    # Calls ZHL::Init() at load on Linux/macOS, so after every hook and definition
    src/main.cpp
)

if(NOT ANDROID AND NOT NINTENDO_SWITCH)
    # Vendored x86 instruction lengths, for the x86 hooking engine
    target_sources(Hyperspace PRIVATE third_party/detours/hde.cpp)

    # Enable SSE2
    target_compile_options(Hyperspace PRIVATE -msse -msse2 -mfpmath=sse)
endif()

target_compile_options(
    Hyperspace PRIVATE

    # Error on narrowing conversion
    -Werror=narrowing

    # Error on missing return
    -Werror=return-type

    # No warning for this == nullptr (can be removed once ShipSystem::GetPowerCap workaround is replaced)
    -fno-delete-null-pointer-checks

    # Warn everything
    # TODO: Check if it's meaningful
    # -Wall
)

target_compile_options(Hyperspace PRIVATE -std=c++11)
if(APPLE)
    target_compile_options(Hyperspace PRIVATE -Wno-deprecated-declarations)
endif()

target_compile_definitions(Hyperspace PRIVATE _REENTRANT)
if(NINTENDO_SWITCH)
    set_target_properties(Hyperspace PROPERTIES OUTPUT_NAME "Hyperspace")
    target_compile_definitions(Hyperspace PRIVATE __SWITCH__=1 HS_ANDROID _GNU_SOURCE=1 _DEFAULT_SOURCE=1)
    # HS_ANDROID reutiliza las rutas de código de Android (offsets iOSARM64, Mach-O helpers)
    # __SWITCH__ guarda las diferencias específicas de Switch (memoria, logging, host)
elseif(ANDROID)
    set_target_properties(Hyperspace PROPERTIES PREFIX "lib" OUTPUT_NAME "Hyperspace")
    target_link_libraries(Hyperspace PRIVATE log)
    target_link_options(Hyperspace PRIVATE
        # The port's libunwind, which can also walk the game's Mach-O frames
        -unwindlib=none "${FTL_LIBCXX}/lib/libunwind.a"
        # Keep the static libraries' symbols (libc++ included) private
        -Wl,--exclude-libs,ALL
    )
elseif(UNIX)
    if (APPLE)
        # SDL3 loads OpenGL dynamically on macOS. 
        target_link_libraries(Hyperspace PRIVATE "-framework OpenGL")
    endif()

    # Naming the binary
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(name_suffix "amd64")
    else()
        set(name_suffix "x86")
    endif()

    if(STEAM_1_6_13_BUILD)
        target_compile_definitions(Hyperspace PRIVATE STEAM_1_6_13_BUILD)
        set_target_properties(Hyperspace PROPERTIES OUTPUT_NAME "Hyperspace.1.6.13.${name_suffix}")
    else()
        set_target_properties(Hyperspace PROPERTIES OUTPUT_NAME "Hyperspace.1.6.12.${name_suffix}")
    endif()
elseif(WIN32)
    # OpenGL, just raw OpenGL goodness
    target_link_libraries(Hyperspace PRIVATE OpenGL::GL)

    target_compile_definitions(Hyperspace PRIVATE BUILD_DLL)
endif()

# Discord Rich Presence, on by default on Windows, desactivado en Switch
if(NINTENDO_SWITCH)
    set(_discord_default OFF)
else()
    set(_discord_default ${WIN32})
endif()
option(USE_DISCORD "Enable the Discord Rich Presence integration" ${_discord_default})

if(USE_DISCORD)
    find_package(DiscordRpc REQUIRED) # From vcpkg & our custom find module

    target_compile_definitions(Hyperspace PRIVATE USE_DISCORD)
    target_link_libraries(Hyperspace PRIVATE DiscordRpc::DiscordRpc)
endif()

