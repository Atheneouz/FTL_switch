# FTL: Faster Than Light para Nintendo Switch

Port nativo de **FTL: Faster Than Light** (versión de iPad) para consolas Nintendo Switch con Custom Firmware (Atmosphère).

Este proyecto ejecuta de forma nativa el binario **ARM64** de iOS en la CPU de la Switch (Cortex-A57), traduciendo las llamadas del sistema y de renderizado mediante una capa de compatibilidad de alto nivel (HLE) sobre `libnx`, `EGL` y `SDL2`.

---

## Requisitos

1. **Nintendo Switch modificada** con Custom Firmware (Atmosphère).
2. **Copia propia de FTL para iPad v1.6.21** descifrada en formato `.ipa`.
3. Herramienta de instalación de paquetes en la consola como **DBI** (recomendado), Goldleaf o Tinfoil.

---

## Guía Rápida para Forks (Cómo crear tu versión lista para jugar)

Si has hecho un fork de este repositorio, estos son los pasos para generar y poner a punto tu juego:

### Paso 1: Obtener el ejecutable compilado (`ftl.nro`)

Tienes dos métodos disponibles:

- **Método A: Con GitHub Actions (Recomendado, no requiere instalar toolchains)**
  1. En tu repositorio de GitHub, entra en la pestaña **Actions**.
  2. Selecciona el workflow **Build Nintendo Switch Homebrew** y pulsa en **Run workflow**.
  3. Al terminar la compilación, descarga el artefacto **`ftl-switch-build`**.
  4. Extrae el archivo `ftl.nro` descargado en la carpeta `bin/` (o en la raíz del proyecto).

- **Método B: Compilación local con devkitPro (devkitA64)**
  ```bash
  cmake -G Ninja -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
    -DCMAKE_BUILD_TYPE=Release

  cmake --build build
  ```

### Paso 2: Extraer los recursos de tu juego (.ipa)

1. Coloca tu archivo `.ipa` de FTL (versión 1.6.21) dentro de la carpeta `ipa/`.
2. Ejecuta el asistente automatizado:
   ```bash
   python tools/prepare_sd_card.py
   ```
3. El script detectará tu `.ipa` y el binario `ftl.nro`, extrayendo automáticamente el ejecutable Mach-O descifrado, los gráficos, sonidos, fuentes y creando las carpetas necesarias en `sd_card/switch/ftl/`.

### Paso 3: Copiar a la consola e instalar el Forwarder

1. Conecta la tarjeta MicroSD al PC o usa el modo MTP Responder de DBI.
2. Copia la carpeta `switch` de `sd_card/` en la raíz de tu MicroSD (`sdmc:/switch/ftl/`).
3. Instala el paquete de acceso directo incluido:
   `instalacion/FTL_Forwarder_[0100F71016210000].nsp`
4. Pulsa el botón HOME en tu consola y verás el icono de **FTL: Faster Than Light** en el menú principal listo para jugar.

---

## Estructura en la Tarjeta MicroSD

La estructura final en la MicroSD queda configurada de la siguiente forma:

```text
sdmc:/
  └── switch/
        └── ftl/
              ├── ftl.nro               <-- Ejecutable de Switch Homebrew
              ├── FTL                   <-- Binario Mach-O ARM64 de iPad
              ├── FTL.app/              <-- Paquete de recursos (ftl.dat, fuentes, texturas)
              ├── save/                 <-- Partidas guardadas (creado automáticamente)
              ├── cache/                <-- Archivos temporales de ejecución
              └── logs/                 <-- Registro de depuración (ftl.log)
```

---

## Modos de Instalación: Forwarder vs Standalone

En la carpeta `instalacion/` dispones de información detallada:

1. **Forwarder NSP (Incluido por defecto, ~0.4 MB)**:
   - Instala un acceso directo oficial en el Menú HOME de la consola.
   - Ejecuta el juego desde `sdmc:/switch/ftl/ftl.nro`.
   - Permite actualizar el ejecutable `ftl.nro` en cualquier momento sin tener que reinstalar el juego.
   - No contiene archivos propietarios, por lo que se distribuye directamente en el repositorio.

2. **Standalone NSP Nativo (Opcional, ~246 MB)**:
   - Empaqueta el ejecutable NSO nativo en ExeFS y todos los datos del juego en RomFS dentro de un único NSP que no requiere archivos en la MicroSD (excepto la carpeta de partidas guardadas).
   - Para generarlo con tus propias claves:
     ```bash
     python tools/prepare_romfs.py
     python tools/build_nsp.py
     ```

---

## Controles

El juego está optimizado para mandos tradicionales (Joy-Con / Mando Pro) y para la pantalla táctil de Nintendo Switch.

### Mando (Joy-Cons / Mando Pro)

| Botón | Acción |
|---|---|
| **Stick Izquierdo** | Mueve el cursor en pantalla (se ralentiza y auto-adhiere a botones, salas y puertas) |
| **A** | Pulsar / Seleccionar / Mantener para arrastrar tripulantes o energía |
| **B / Plus (+)** | Volver / Menú de pausa del juego |
| **X** | Pausa rápida del combate espacial |
| **L / R (o ZL / ZR)** | Zoom alejar / Zoom acercar sobre la nave |
| **Stick Derecho** | Desplazar la cámara libremente cuando el zoom está activo |
| **D-Pad (Cruceta)** | Movimiento preciso del cursor |

### Pantalla Táctil

- **Toque y Arrastre**: Funciona exactamente igual que en la versión original de iPad.
- **Multitáctil**: Soporta gestos de pellizco para control del zoom y pulsación con dos dedos para pausar el combate espacial.

---

## Créditos y Agradecimientos

- **Subset Games**: Por crear la obra maestra FTL: Faster Than Light.
- **Sidd065**: Por el proyecto [ftl-android](https://github.com/Sidd065/ftl-android), base del cargador Mach-O y de la capa de adaptación de interfaz.
- **devkitPro & SwitchBrew**: Por la excelente cadena de herramientas `devkitA64` y la librería `libnx`.
