cmake_minimum_required(VERSION 3.20)
project(ftl_switch C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Opciones de compilación
set(LIBCXX_DIR "" CACHE PATH "Directorio de libc++ con Apple ABI para ARM64")
option(FTL_HARNESS "Compilar el arnés de pruebas para Linux aarch64" OFF)

set(CORE_SOURCES
    src/loader/macho.c
    src/loader/symbols.c
    src/loader/symbols_cxx.c
    src/objc/objc_runtime.c
    src/objc/objc_api.c
    src/objc/objc_msgSend.S
    src/darwin/libsystem.c
    src/darwin/darwin_asm.S
    src/fw/foundation.c
    src/fw/uikit.c
    src/fw/quartz_eagl.c
    src/fw/present.c
    src/fw/pvrtc.c
    src/fw/gamecontroller.c
    src/fw/audio_misc.c
    src/port_main.c
    src/patches.c
    src/hyperspace.c
    src/ux/ux.c
    src/ux/pad.c
    src/ux/targets.c
    src/ux/overlay.c
    src/ux/mods.c
    src/ux/png.c
)

set(COMMON_FLAGS
    -fno-omit-frame-pointer
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -fvisibility=hidden
    -ffunction-sections
    -fdata-sections
)

# Librerías de soporte Apple ABI para C++ si se especifican
if(LIBCXX_DIR)
    set(CXX_LIBS
        ${LIBCXX_DIR}/lib/libc++.a
        ${LIBCXX_DIR}/lib/libc++abi.a
        ${LIBCXX_DIR}/lib/libunwind.a
    )
else()
    set(CXX_LIBS "")
endif()

# Comprobamos si el entorno es Nintendo Switch (devkitA64 / libnx)
if(DEFINED SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "Switch" OR DEFINED ENV{DEVKITPRO})
    message(STATUS "Configurando compilación nativa para Nintendo Switch")

    add_definitions(-D__SWITCH__=1)

    set(SWITCH_SOURCES
        ${CORE_SOURCES}
        src/platform/plat_switch.c
    )

    add_executable(ftl ${SWITCH_SOURCES})

    target_include_directories(ftl PRIVATE
        include
        src
        $ENV{PORTLIBS_PREFIX}/include
        $ENV{DEVKITPRO}/libnx/include
        $ENV{DEVKITPRO}/portlibs/switch/include
    )

    target_compile_options(ftl PRIVATE
        ${COMMON_FLAGS}
        -O2
        -g
        -march=armv8-a+crc+crypto
        -mtune=cortex-a57
        -mtp=soft
        -fPIE
    )

    target_link_options(ftl PRIVATE
        -pie
        -Wl,--gc-sections
        -Wl,--export-dynamic-symbol=free
        -Wl,--export-dynamic-symbol=realloc
        -Wl,--export-dynamic-symbol=malloc
        -Wl,--export-dynamic-symbol=calloc
        -Wl,--export-dynamic-symbol=__nx_stack_size
        -Wl,--undefined=__nx_stack_size
        -Wl,--allow-multiple-definition
    )

    target_link_libraries(ftl PRIVATE
        ${CXX_LIBS}
        -lEGL
        -lGLESv2
        -lglapi
        -ldrm_nouveau
        -lSDL2
        -lnx
        -lstdc++
        -lm
        -lz
    )

    # Configuración de Hyperspace (librería estática opcional para mods)
    option(ENABLE_HYPERSPACE "Vincular soporte para Hyperspace en Nintendo Switch" ON)
    set(HYPERSPACE_LIB "" CACHE FILEPATH "Ruta manual a libHyperspace.a")
    set(HYPERSPACE_VCPKG_DIR "" CACHE PATH "Ruta al directorio vcpkg_installed/arm64-switch-ftl")

    if(ENABLE_HYPERSPACE)
        set(HS_FOUND_LIB "")
        if(HYPERSPACE_LIB AND EXISTS "${HYPERSPACE_LIB}")
            set(HS_FOUND_LIB "${HYPERSPACE_LIB}")
        else()
            set(HS_CANDIDATE_PATHS
                "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build-switch-release/libHyperspace.a"
                "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build-switch-debug/libHyperspace.a"
                "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build/libHyperspace.a"
                "${CMAKE_CURRENT_SOURCE_DIR}/lib/libHyperspace.a"
            )
            foreach(cand ${HS_CANDIDATE_PATHS})
                if(EXISTS "${cand}")
                    set(HS_FOUND_LIB "${cand}")
                    break()
                endif()
            endforeach()
        endif()

        if(HS_FOUND_LIB)
            message(STATUS "Hyperspace detectado para Nintendo Switch: ${HS_FOUND_LIB}")
            add_definitions(-DHAVE_HYPERSPACE=1)

            target_include_directories(ftl PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/src
                ${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/src/switch
                ${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/src/android
            )

            if(NOT HYPERSPACE_VCPKG_DIR)
                set(VCPKG_CANDIDATES
                    "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build-switch-release/vcpkg_installed/arm64-switch-ftl"
                    "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build-switch-debug/vcpkg_installed/arm64-switch-ftl"
                )
                foreach(cand ${VCPKG_CANDIDATES})
                    if(EXISTS "${cand}")
                        set(HYPERSPACE_VCPKG_DIR "${cand}")
                        break()
                    endif()
                endforeach()
            endif()

            set(HS_EXTRA_LIBS "")
            set(LUA_SWITCH_CANDIDATES
                "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build-switch-release/liblua_switch.a"
                "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build-switch-debug/liblua_switch.a"
                "${CMAKE_CURRENT_SOURCE_DIR}/hyperspace-src/build-switch-release/liblua.a"
            )
            foreach(cand ${LUA_SWITCH_CANDIDATES})
                if(EXISTS "${cand}")
                    set(HS_EXTRA_LIBS "${cand}")
                    message(STATUS "Librería Lua de Hyperspace detectada: ${cand}")
                    break()
                endif()
            endforeach()

            if(HYPERSPACE_VCPKG_DIR AND EXISTS "${HYPERSPACE_VCPKG_DIR}")
                message(STATUS "Dependencias de Hyperspace: ${HYPERSPACE_VCPKG_DIR}")
                target_include_directories(ftl PRIVATE "${HYPERSPACE_VCPKG_DIR}/include")
                target_link_directories(ftl PRIVATE "${HYPERSPACE_VCPKG_DIR}/lib")
            endif()

            target_link_libraries(ftl PRIVATE
                -Wl,--whole-archive
                ${HS_FOUND_LIB}
                -Wl,--no-whole-archive
                ${HS_EXTRA_LIBS}
            )
        else()
            message(STATUS "Hyperspace no encontrado en rutas predeterminadas (se puede compilar en hyperspace-src)")
        endif()
    endif()

    # Generación de metadatos NACP y ejecutable NRO
    set(APP_TITLE "FTL: Faster Than Light")
    set(APP_AUTHOR "Subset Games / Sidd065 / FTL-Switch")
    set(APP_VERSION "1.6.21")

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.jpg")
        set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon.jpg")
        set(OPTIONAL_ICON ICON "${APP_ICON}")
    else()
        set(APP_ICON "")
        set(OPTIONAL_ICON "")
    endif()

    set(ROMFS_DIR "" CACHE PATH "Directorio opcional con recursos RomFS para incrustar en el NRO")
    if(ROMFS_DIR)
        set(OPTIONAL_ROMFS ROMFS "${ROMFS_DIR}")
        set(OPTIONAL_ELF2NRO_ROMFS "--romfsdir=${ROMFS_DIR}")
    else()
        set(OPTIONAL_ROMFS "")
        set(OPTIONAL_ELF2NRO_ROMFS "")
    endif()

    if(COMMAND nx_create_nro)
        # Macro nativa de devkitPro CMake
        nx_create_nro(ftl
            NAME "${APP_TITLE}"
            AUTHOR "${APP_AUTHOR}"
            VERSION "${APP_VERSION}"
            ${OPTIONAL_ICON}
            ${OPTIONAL_ROMFS}
        )
    else()
        # Fallback mediante comandos directos de devkitPro
        add_custom_command(TARGET ftl POST_BUILD
            COMMAND nacptool --create "${APP_TITLE}" "${APP_AUTHOR}" "${APP_VERSION}" ftl.nacp
            COMMAND elf2nro ftl.elf ftl.nro --nacp=ftl.nacp
                $<$<BOOL:${APP_ICON}>:--icon=${APP_ICON}>
                $<$<BOOL:${ROMFS_DIR}>:${OPTIONAL_ELF2NRO_ROMFS}>
            COMMENT "Generando ftl.nro para Nintendo Switch"
        )
    endif()

elseif(FTL_HARNESS)
    # Arnés de pruebas para Linux / QEMU
    message(STATUS "Configurando arnés de pruebas para Linux aarch64")
    add_executable(ftlrun
        ${CORE_SOURCES}
        ftl-android-upstream/native/src/platform/plat_linux.c
    )
    target_include_directories(ftlrun PRIVATE include src)
    target_compile_options(ftlrun PRIVATE ${COMMON_FLAGS} -g)
    target_link_options(ftlrun PRIVATE
        -fuse-ld=lld
        -Wl,--gc-sections
        -Wl,--export-dynamic-symbol=free
        -Wl,--export-dynamic-symbol=realloc
        -Wl,--export-dynamic-symbol=malloc
        -Wl,--export-dynamic-symbol=calloc
    )
    target_link_libraries(ftlrun PRIVATE ${CXX_LIBS} EGL GLESv2 z pthread dl m)
else()
    message(STATUS "Configuración por defecto: configure con toolchain de Switch o -DFTL_HARNESS=ON")
endif()
