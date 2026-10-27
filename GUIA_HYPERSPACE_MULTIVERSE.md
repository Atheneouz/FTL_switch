# Guía de Instalación y Uso: FTL Hyperspace y Multiverse en Nintendo Switch

En esta guía explicamos el procedimiento completo para compilar, instalar y ejecutar **FTL: Hyperspace** y el mod **FTL: Multiverse** en Nintendo Switch utilizando el port nativo (`ftl.nro`).

---

## 1. Fundamentos Técnicos

- **Compatibilidad Binaria:** Tanto el port nativo de Switch como el port de Android ejecutan el mismo ejecutable de base: **FTL for iPad 1.6.21 (ARM64 Mach-O)**. Esto significa que las estructuras internas de datos, clases de C++, offsets de vtable y firmas de funciones son 100% idénticas.
- **Enlace Estático y W^X:** A diferencia de Android (donde Hyperspace se carga mediante `dlopen("libHyperspace.so")`), el sistema operativo de Nintendo Switch (Horizon OS) no permite bibliotecas dinámicas estándar en aplicaciones Homebrew y aplica una política estricta de **W^X** (páginas de memoria no pueden ser simultáneamente grabables y ejecutables).
  - Hyperspace se compila como **librería estática (`libHyperspace.a`)** que se vincula directamente dentro del ejecutable `ftl.elf`/`ftl.nro`.
  - El motor de hooking `Arm64Detour` conmuta los permisos de las páginas de código temporalmente mediante `svcSetProcessMemoryPermission()` a `Perm_Rw` al instalar el salto y las devuelve de inmediato a `Perm_Rx`, sincronizando las cachés L1/L2 de la CPU Cortex-A57 (`armDCacheFlush` y `armICacheInvalidate`).
- **Símbolos Débiles (Zero-Overhead):** El port principal define los puntos de entrada de Hyperspace como símbolos débiles. Si se compila el ejecutable sin `libHyperspace.a`, el juego funciona con normalidad en modo vainilla. Si se incluye la librería y se detecta `hyperspace.xml` en la tarjeta SD, Hyperspace se inicializa automáticamente al arrancar.

---

## 2. Compilación del Proyecto

### Opción 1: Compilación Automática en GitHub Actions (Recomendado)

Disponemos de un workflow específico en GitHub Actions ([build_switch_hyperspace.yml](file:///.github/workflows/build_switch_hyperspace.yml)) que realiza todo el proceso en la nube (compilación de LLVM libc++ con Apple ABI, compilación de `libHyperspace.a` con devkitA64 y generación del binario final `ftl.nro`):

1. En el repositorio de GitHub, entramos en la pestaña **Actions**.
2. En el panel lateral izquierdo, seleccionamos el flujo **Build Nintendo Switch Homebrew with Hyperspace (ftl-hyperspace.nro)**.
3. Pulsamos en el botón **Run workflow** (rama `main` o `master`) y seleccionamos el tipo de compilación (`Release`).
4. Al finalizar la ejecución (unos minutos), descargamos los artefactos generados al final de la página:
   - **`ftl-switch-hyperspace-build`**: Contiene `ftl.nro`, `ftl-hyperspace.nro`, el forwarder NSP y la documentación.
   - **`ftl-switch-hyperspace-sdcard-ready`**: Si colocaste tu archivo `.ipa` en la carpeta `ipa/`, este paquete descargable viene completamente listo para descomprimir en la raíz de la tarjeta SD.

---

### Opción 2: Compilación Local Manual

Si prefieres compilar en tu propia máquina Linux o contenedor con devkitPro:

#### Requisitos Previos en el Host
- Entorno **devkitPro** configurado (`DEVKITPRO=/opt/devkitpro`) con paquetes `devkitA64`, `libnx`, `switch-tools`, `switch-mesa`, `switch-zlib`.
- Herramientas del host: `cmake` (>= 3.20), `ninja` o `make`, `swig`, `lua` (>= 5.3).
- Directorio de `libcxx` compilado con soporte Apple ABI (`libcxx/build.sh switch`).

#### Paso A: Compilación de `libHyperspace.a`
Dentro del subdirectorio `hyperspace-src`:

```bash
cd hyperspace-src

# 1. Inicializamos dependencias y triplete vcpkg
bash buildscripts/switch/setup.sh

# 2. Pre-generamos tablas de enganche ZHL y cabeceras
bash buildscripts/switch/pre-generate.sh

# 3. Compilamos la librería estática
bash buildscripts/switch/build.sh Release

cd ..
```

Esto generará el archivo `hyperspace-src/build-switch-release/libHyperspace.a`.

### Paso B: Compilación de `ftl.nro` con Hyperspace
Desde la raíz del repositorio:

```bash
mkdir -p build && cd build

cmake -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
      -DLIBCXX_DIR=/ruta/a/libcxx-build \
      -DCMAKE_BUILD_TYPE=Release ..

make -j$(nproc)
```

El script de CMake detectará automáticamente la presencia de `libHyperspace.a` en `hyperspace-src/build-switch-release/` y creará el ejecutable final `ftl.nro` con Hyperspace integrado.

---

## 3. Preparación de la Tarjeta SD (`sdmc:/switch/ftl/`)

Copiamos `ftl.nro` y los recursos a la carpeta del juego en la tarjeta SD:

```text
sdmc:/
  └── switch/
      └── ftl/
          ├── ftl.nro                 <- Binario compilado
          ├── ftl.dat                 <- Archivo de recursos base de FTL
          ├── hyperspace.xml          <- Definición principal de Hyperspace
          ├── settings_hs.ini         <- Opciones de Hyperspace (resolución, UI)
          ├── data/                   <- XMLs parcheados del mod
          │   ├── blueprints.xml.append
          │   ├── events.xml.append
          │   ├── hyperspace.xml
          │   └── ...
          ├── img/                    <- Sprites y naves adicionales de Multiverse
          ├── audio/                  <- Audio y música extendida de Multiverse
          └── mod-appendix/           <- Archivos complementarios
```

> **Consejo para instalar FTL: Multiverse:**
> 1. Descargamos la versión oficial de *FTL: Multiverse* y sus archivos `.ftl` o `.zip`.
> 2. En PC, podemos extraer los contenidos o utilizar *Slipstream Mod Manager* para volcar los archivos parcheados (carpetas `data/`, `img/`, `audio/`, y el archivo `hyperspace.xml`) directamente dentro de `sdmc:/switch/ftl/`.
> 3. Al existir `hyperspace.xml` en dicho directorio, el juego arrancará automáticamente en modo Hyperspace.

---

## 4. Controles y Clic Secundario (Right Click)

Los scripts en Lua de Hyperspace y las habilidades especiales de tripulación de Multiverse dependen frecuentemente del clic derecho:

- **Gatillo ZR (Joy-Con Derecho / Mando Pro):** Asignado a la acción de clic secundario (`ACT_RIGHT_CLICK`), transmitiendo las coordenadas del cursor virtual hacia `hs_switch_right_button()`.
- **Pantalla Táctil:** El toque simultáneo con dos dedos se detecta como clic derecho en el punto de contacto, activando las habilidades o cancelaciones pertinentes.
- **Sticks Analógicos:** Control del cursor y navegación por los menús del juego.

---

## 5. Diagnóstico y Logs

Hyperspace almacena toda la información de inicio y enganche en la tarjeta SD:
- `sdmc:/switch/ftl/zhl.log`: Registro detallado de firmas escaneadas, trampolines aplicados en memoria y comprobación de integridad.
- `sdmc:/switch/ftl/FTL_HS.log`: Registro del runtime de Hyperspace, carga de `hyperspace.xml`, scripts Lua de Multiverse y posibles advertencias de recursos faltantes.

Si se produce cualquier anomalía durante el arranque, ambos archivos permiten identificar de inmediato si algún archivo XML contiene errores de sintaxis o si falta algún asset.
