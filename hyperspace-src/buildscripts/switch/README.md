# Compilación de FTL: Hyperspace para Nintendo Switch

Esta carpeta contiene los scripts para compilar **FTL: Hyperspace** como librería estática (`libHyperspace.a`) destinada al port nativo de FTL para Nintendo Switch (`ftl.nro`).

---

## 1. Requisitos Previos

1. **devkitPro con devkitA64 y libnx:**
   - Debe estar instalado y con la variable `DEVKITPRO` exportada (habitualmente `/opt/devkitpro`).
   - Paquetes necesarios: `devkitA64`, `libnx`, `switch-tools`, `switch-mesa`, `switch-zlib`.
2. **Build host tools:**
   - `cmake` (>= 3.16) y `ninja-build`.
   - `swig` (para los bindings Lua).
   - `lua` (>= 5.3) con `lua-lpeg` y `lua-filesystem` (para la pre-generación ZHL).
3. **FTL_LIBCXX:**
   - La compilación de `libc++` y `libc++abi` con Apple ARM64 ABI generada por el port (`libcxx/build.sh switch`).

---

## 2. Flujo de Compilación

### Paso 1: Configuración inicial (una sola vez)
```bash
bash buildscripts/switch/setup.sh
```
Este script descarga e inicializa `vcpkg` y copia el toolchain (`arm64-switch-ftl.cmake`) y el triplete a las carpetas correspondientes.

### Paso 2: Pre-generación de fuentes ZHL
```bash
bash buildscripts/switch/pre-generate.sh
```
Genera las definiciones de funciones y estructuras de `iOSARM64` a partir de `libzhlgen` y el número de versión (`Version.autogen.hpp`).

### Paso 3: Compilación
```bash
export DEVKITPRO=/opt/devkitpro
export FTL_LIBCXX=/ruta/a/libcxx-build

bash buildscripts/switch/build.sh Release
```
El resultado será la librería estática:
`build-switch-release/libHyperspace.a`

---

## 3. Integración en `ftl-switch`

El archivo `libHyperspace.a` generado se enlaza estáticamente en el ejecutable `ftl.elf` / `ftl.nro`. El port principal llama directamente a `hs_switch_init()` durante el arranque.
