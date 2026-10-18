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

    add_executable(ftl.elf ${SWITCH_SOURCES})

    target_include_directories(ftl.elf PRIVATE
        include
        src
        $ENV{PORTLIBS_PREFIX}/include
        $ENV{DEVKITPRO}/libnx/include
        $ENV{DEVKITPRO}/portlibs/switch/include
    )

    target_compile_options(ftl.elf PRIVATE
        ${COMMON_FLAGS}
        -O2
        -g
        -march=armv8-a+crc+crypto
        -mtune=cortex-a57
        -mtp=soft
        -fPIE
    )

    target_link_options(ftl.elf PRIVATE
        -pie
        -Wl,--gc-sections
        -Wl,--export-dynamic-symbol=free
        -Wl,--export-dynamic-symbol=realloc
        -Wl,--export-dynamic-symbol=malloc
        -Wl,--export-dynamic-symbol=calloc
    )

    target_link_libraries(ftl.elf PRIVATE
        ${CXX_LIBS}
        -lEGL
        -lGLESv2
        -lglapi
        -ldrm_nouveau
        -lSDL2
        -lnx
        -lm
        -lz
    )

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

    if(COMMAND nx_create_nro)
        # Macro nativa de devkitPro CMake
        nx_create_nro(ftl.elf
            NAME "${APP_TITLE}"
            AUTHOR "${APP_AUTHOR}"
            VERSION "${APP_VERSION}"
            ${OPTIONAL_ICON}
        )
    else()
        # Fallback mediante comandos directos de devkitPro
        add_custom_command(TARGET ftl.elf POST_BUILD
            COMMAND nacptool --create "${APP_TITLE}" "${APP_AUTHOR}" "${APP_VERSION}" ftl.nacp
            COMMAND elf2nro ftl.elf ftl.nro --nacp=ftl.nacp
                $<$<BOOL:${APP_ICON}>:--icon=${APP_ICON}>
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
