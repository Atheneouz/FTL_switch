# Plan de Portabilidad de FTL: Hyperspace a Nintendo Switch

Este documento detalla la hoja de ruta técnica para portar **FTL: Hyperspace** (rama `android-arm64` de Sidd065) a Nintendo Switch, permitiendo la ejecución de mods avanzados como *FTL: Multiverse*.

---

## 1. Contexto y Arquitectura Actual

- **Base de partida:** El repositorio `hyperspace-src` (`android-arm64`) ya contiene la adaptación de firmas, estructuras y offsets de memoria para el binario **FTL iOS 1.6.21 (ARM64)**, que es exactamente el mismo ejecutable que corre en Switch a través de nuestro loader Mach-O.
- **Diferencia de plataforma:**
  - Android: Se compila como `libHyperspace.so` dinámico, utiliza `dlopen()`, `mprotect()`, lectura de `/proc/self/maps` y `__android_log_print()`.
  - Switch (Horizon OS): No se apoya en `dlopen()` dinámico estándar. Debe compilarse como **librería estática (`libHyperspace.a`)** enlazada en el binario final `ftl.nro`, adaptando los permisos de memoria a las syscalls y funciones de `libnx`.

---

## 2. Fases de Trabajo

### Fase 1: Entorno de Compilación y Dependencias (devkitA64) - [COMPLETADA]
- **1.1. Gestión de dependencias y toolchain Switch:**
  - Creado toolchain de compilación cruzada `hyperspace-src/.devcontainer/toolchains/arm64-switch-ftl.cmake` adaptado a devkitA64 y Apple ARM64 ABI.
  - Creado triplete vcpkg `hyperspace-src/.devcontainer/triplets/arm64-switch-ftl.cmake` para vincular dependencias estáticas (arm64, generic, static CRT).
- **1.2. Generación de código SWIG / ZHL:**
  - Creado script `buildscripts/switch/pre-generate.sh` para pre-generar `FTLGameiOSARM64.{h,cpp}`, `Version.autogen.hpp` y `swigluarun.h` en el host.
  - Adaptado `cmake/GeneratedSources.cmake` con fallback automático a fuentes pre-generadas si no hay intérprete Lua/lpeg en el entorno de compilación.
- **1.3. Configuración de `CMakeLists.txt` para Switch:**
  - Añadida opción `NINTENDO_SWITCH` en `hyperspace-src/CMakeLists.txt` para compilar como librería estática (`STATIC`), excluyendo SDL3, Discord RPC y detectores de crash.
  - Añadido identificador de plataforma `__SWITCH__` en `HSVersion.h` (`" Switch"`).
  - Creados scripts `setup.sh`, `build.sh` y guía `README.md` en `buildscripts/switch/`.

---

### Fase 2: Adaptación del Hooking de Memoria (`Arm64Detour` y ZHL) - [COMPLETADA]
Hyperspace inyecta saltos (trampolines de 128 MB relativos) en el código del juego. En Switch, el kernel Horizon protege estrictamente las páginas de memoria ejecutable.

- **2.1. Sustitución de `/proc/self/maps` y asignación de pool:**
  - Eliminada la dependencia de `/proc/self/maps` en Switch. Las protecciones de segmentos se leen en tiempo de ejecución directamente desde las cabeceras Mach-O (`MachO::SegmentProtection`).
  - Implementado `MakePoolNear` con soporte dual: asignación de memoria virtual de código (`virtmemFindCodeMemory`) y mapeo de heap con `svcMapProcessCodeMemory` en Switch real, con fallback a `memalign` para emuladores.
  - Añadido `Arm64Detour::SetNearPool(void *base, size_t size)` para permitir al loader pasar un pool contiguo al binario.
- **2.2. Permisos de memoria (RWX / W^X):**
  - Sustituido `mprotect()` por `svcSetProcessMemoryPermission()` respetando el modelo W^X de Horizon OS (`Perm_Rw` durante la escritura temporal, restaurando de inmediato a `Perm_Rx`).
  - `PALMemoryProtection.h` adaptado para que todas las modificaciones de VTable y parches de Hyperspace utilicen esta transición segura.
- **2.3. Sincronización de cachés:**
  - Implementada sincronización explícita en `FlushCode()` mediante `armDCacheFlush()` y `armICacheInvalidate()` de `libnx` para garantizar coherencia en la arquitectura Harvard de la CPU Cortex-A57.
- **2.4. Escáner de firmas y gestión de errores:**
  - Habilitado el escáner de firmas Mach-O (`SetGameImage`, `IsKnownGame`) en `SigScan.h` y `SigScan.cpp` bajo `__SWITCH__`.
  - Modificado `zhl.cpp` para reportar errores de inicialización de forma controlada sin forzar `exit(1)`.

---

### Fase 3: Capa Host de Plataforma (`SwitchHost.cpp`) - [COMPLETADA]
Sustituido [AndroidHost.cpp](file:///c:/Users/ortas/OneDrive/Documentos/FTL/hyperspace-src/src/android/AndroidHost.cpp) por el host específico de Switch [SwitchHost.cpp](file:///c:/Users/ortas/OneDrive/Documentos/FTL/hyperspace-src/src/switch/SwitchHost.cpp):

- **3.1. Logging y Diagnóstico:**
  - Redirección de logs (`zhl.log`, `FTL_HS.log`) al callback `hs_log` del port y rotación automática de `.bak` en `sdmc:/switch/ftl/`.
  - Diagnóstico de llamadas ARM64 `LogCallers()` y trampas controladas para funciones no implementadas en la versión iPad (`MakeMissingStub`).
- **3.2. Rutas del Sistema:**
  - Ajustado el directorio de trabajo (`data_dir`) a `sdmc:/switch/ftl/` en `hs_switch_init` para carga de `settings_hs.ini`, `hyperspace.xml` y mods.
- **3.3. Enlace directo y eventos:**
  - Creados [SwitchHost.h](file:///c:/Users/ortas/OneDrive/Documentos/FTL/hyperspace-src/src/switch/SwitchHost.h) y [SwitchHost.cpp](file:///c:/Users/ortas/OneDrive/Documentos/FTL/hyperspace-src/src/switch/SwitchHost.cpp) exponiendo `hs_switch_init` como punto de entrada estático sin `dlopen`.
  - Exportadas funciones `hs_switch_wants_right_click` y `hs_switch_right_button` en `TouchToLua.cpp` para eventos de clic secundario desde botones Joy-Con o gestos táctiles.
  - Actualizado [PALMessageBox.h](file:///c:/Users/ortas/OneDrive/Documentos/FTL/hyperspace-src/src/helpers/PALMessageBox.h) para desacoplar de SDL3 y canalizar errores vía `PAL_HostShowError`.
  - Configurado `CMakeLists.txt` para incluir `SwitchHost.cpp` y excluir `AndroidHost.cpp` en Switch.

---

### Fase 4: Integración en el Port Principal (`ftl-switch`) - [COMPLETADA]
- **4.1. Conexión en `hyperspace.c` y símbolos débiles:**
  - Actualizado [src/hyperspace.c](file:///c:/Users/ortas/OneDrive/Documentos/FTL/src/hyperspace.c) con declaraciones de símbolos débiles (`__attribute__((weak))`) para `hs_switch_init`, `hs_switch_wants_right_click` y `hs_switch_right_button`.
  - Pasamos la estructura `hs_host_info` con la dirección base del Mach-O (`img->base`), el slide ASLR (`img->slide`), el tamaño de imagen (`img->map_size`), los directorios de datos y log, y el callback de desenrollado de pila `__ftl_find_macho_unwind`.
  - Si el ejecutable se compila sin `libHyperspace.a`, el puntero `hs_switch_init` es NULL y el juego continúa en modo vainilla sin fallar.
- **4.2. Detección automática en `plat_switch.c`:**
  - Añadida comprobación en [src/platform/plat_switch.c](file:///c:/Users/ortas/OneDrive/Documentos/FTL/src/platform/plat_switch.c) para detectar la existencia de `sdmc:/switch/ftl/hyperspace.xml` o el indicador `enable_hyperspace`.
  - Al detectarse, activa automáticamente `cfg.hyperspace = "builtin"`.
- **4.3. Eventos e Input (Clic derecho):**
  - Mapeado el gatillo `ZR` (`K_R2` en `plat_switch.c`) a la acción `ACT_RIGHT_CLICK` en [src/ux/pad.c](file:///c:/Users/ortas/OneDrive/Documentos/FTL/src/ux/pad.c).
  - Ambos (botón físico `ZR` y gestos de doble toque en pantalla táctil en [src/ux/ux.c](file:///c:/Users/ortas/OneDrive/Documentos/FTL/src/ux/ux.c)) se reenvían directamente a `hyperspace_right_button()` y de ahí a `hs_switch_right_button()` en `TouchToLua.cpp`.
- **4.4. Vinculación en `CMakeLists.txt` del port:**
  - Añadida la opción `ENABLE_HYPERSPACE` y detección automática de `libHyperspace.a` en `hyperspace-src/build-switch-release/libHyperspace.a` o rutas personalizadas.
  - Vinculación con `-Wl,--whole-archive` para asegurar la inclusión de todas las tablas de hooks y constructores estáticos, enlazando dependencias (`-lminizip`, `-llua`, `-lboost_filesystem`, `-lboost_system`).

---

### Fase 5: Pruebas, Validación y Despliegue de Mods (Multiverse) - [GUÍA OPERATIVA]

#### 5.1. Estructura de archivos en la tarjeta SD (`sdmc:/switch/ftl/`)
Para ejecutar *FTL: Multiverse* u otros mods basados en Hyperspace en la consola:
```text
sdmc:/
  └── switch/
      └── ftl/
          ├── ftl.nro                 <- Ejecutable compilado con Hyperspace
          ├── ftl.dat                 <- Datos de FTL (o assets extraídos)
          ├── hyperspace.xml          <- Archivo principal de definición de Hyperspace
          ├── settings_hs.ini         <- Configuración de Hyperspace (resolución, interfaz, etc.)
          ├── data/                   <- Archivos XML modificados por Multiverse
          │   ├── blueprints.xml.append
          │   ├── events.xml.append
          │   └── ...
          ├── img/                    <- Sprites y naves adicionales de Multiverse
          ├── audio/                  <- Efectos de sonido y pistas musicales del mod
          ├── mod-appendix/           <- Metadatos o scripts adicionales
          └── FTL_HS.log              <- Log generado automáticamente por Hyperspace
```

#### 5.2. Procedimiento de compilación completa
1. **Compilar `libHyperspace.a`:**
   ```bash
   cd hyperspace-src
   bash buildscripts/switch/setup.sh
   bash buildscripts/switch/pre-generate.sh
   bash buildscripts/switch/build.sh Release
   cd ..
   ```
2. **Compilar `ftl.nro`:**
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
         -DLIBCXX_DIR=/ruta/a/libcxx-build \
         -DCMAKE_BUILD_TYPE=Release ..
   make -j$(nproc)
   ```
   CMake detectará automáticamente `hyperspace-src/build-switch-release/libHyperspace.a` e incrustará Hyperspace en `ftl.nro`.

#### 5.3. Checklist de validación
- [x] **Arranque base sin mods:** El juego inicia normalmente; `hs_switch_init` no se ejecuta si no existe `hyperspace.xml`.
- [x] **Activación con `hyperspace.xml`:** Detecta el mod y ejecuta los hooks en memoria; la pantalla de título muestra el indicador de versión `FTL: Hyperspace 1.x.x (Switch)`.
- [x] **Memoria W^X:** Verificación de que `Arm64Detour` modifica las páginas de código pasando temporalmente por `Perm_Rw` y restableciendo a `Perm_Rx` sin provocar pánicos de kernel Horizon.
- [x] **Carga de Multiverse:** Extracción de recursos con Slipstream Mod Manager a la carpeta `sdmc:/switch/ftl/` y comprobación de consumo de RAM (< 2.5 GB).
