# FTL: Faster Than Light for Nintendo Switch

[English](#english) | [Español](#español)

---

<a name="english"></a>
## English

Native port of **FTL: Faster Than Light** (iPad edition) for Nintendo Switch consoles running Custom Firmware (Atmosphère).

This project runs the iOS **ARM64** binary natively on the Switch's CPU (Cortex-A57), translating system calls, rendering, and input through a high-level compatibility layer (HLE) built on `libnx`, `EGL`, and `SDL2`.

### Requirements

1. **Nintendo Switch** with Custom Firmware (Atmosphère).
2. **Decrypted copy of FTL for iPad v1.6.21** in `.ipa` format.
3. Package installer on your console such as **DBI** (recommended), Goldleaf, or Tinfoil.

---

### Quick Start Guide for Forks (Build Your Playable Game)

If you have forked this repository, follow these steps to build and prepare your game:

#### Step 1: Obtain the Compiled Binary (`ftl.nro`)

Choose one of two methods:

- **Method A: Via GitHub Actions (Recommended, no toolchains required)**
  1. In your GitHub repository fork, go to the **Actions** tab.
  2. Select the workflow **Build Nintendo Switch Homebrew** and click **Run workflow**.
  3. Once completed, download the **`ftl-switch-build`** artifact.
  4. Extract `ftl.nro` into the `bin/` folder (or repository root).

- **Method B: Local Compilation with devkitPro (devkitA64)**
  ```bash
  cmake -G Ninja -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
    -DCMAKE_BUILD_TYPE=Release

  cmake --build build
  ```

#### Step 2: Extract Game Assets from your `.ipa`

1. Place your FTL iOS package (v1.6.21) into the `ipa/` folder (e.g. `ipa/FTL_1.6.21.ipa` or any `.ipa` filename).
2. Run the automated preparation tool:
   ```bash
   python tools/prepare_sd_card.py
   ```
3. The script automatically finds your `.ipa` and `ftl.nro`, unpacks the decrypted Mach-O binary, textures, audio, fonts, and sets up `sd_card/switch/ftl/`.

#### Step 3: Copy to MicroSD and Launch

1. Connect your Switch MicroSD card to your PC (or use DBI's MTP Responder).
2. Copy the `switch` folder from `sd_card/` into the root of your MicroSD card (`sdmc:/switch/ftl/`).
3. You can launch the game directly from the **Homebrew Menu** (`ftl.nro`).
4. *(Optional)* To create a shortcut icon on your Switch HOME Menu, generate and install the forwarder:
   ```bash
   python tools/build_nsp.py
   ```
   Then install the generated `instalacion/FTL_Forwarder_[0100F71016210000].nsp` with DBI.

---

### MicroSD Card Structure

Your MicroSD card will be laid out as follows:

```text
sdmc:/
  └── switch/
        └── ftl/
              ├── ftl.nro               <-- Switch Homebrew executable
              ├── FTL                   <-- iPad ARM64 Mach-O binary
              ├── FTL.app/              <-- Resource bundle (ftl.dat, fonts, audio)
              ├── save/                 <-- Game saves (created automatically)
              ├── cache/                <-- Runtime cache
              └── logs/                 <-- Debug log (ftl.log)
```

---

### Homebrew Forwarder (Optional HOME Menu Shortcut)

You can launch FTL via the Homebrew Menu (Album), or generate a custom forwarder to play directly from the HOME Menu:
- Run `python tools/build_nsp.py` to create `instalacion/FTL_Forwarder_[0100F71016210000].nsp`.
- Installs a direct shortcut on your HOME Menu pointing to `sdmc:/switch/ftl/ftl.nro`.
- Updating the `ftl.nro` binary on your SD card will not require reinstalling the forwarder.

---

### Controls

The game is adapted for both traditional controllers (Joy-Cons / Pro Controller) and the Nintendo Switch touchscreen.

#### Controller (Joy-Cons / Pro Controller)

| Button | Action |
|---|---|
| **Left Stick** | Moves screen cursor (slows and snaps to buttons, rooms, and doors) |
| **A** | Tap / Select / Hold to drag crew or power |
| **B / Plus (+)** | Back / Game Pause Menu |
| **X** | Space combat quick pause |
| **L / R (or ZL / ZR)** | Zoom out / Zoom in on your ship |
| **Right Stick** | Pan camera freely when zoomed in |
| **D-Pad** | Precise cursor adjustments |

#### Touchscreen

- **Tap and Drag**: Operates identically to the original iPad interface.
- **Multitouch**: Supports pinch-to-zoom and two-finger tap to pause combat.

---

### Mods & Modding

#### Simple / Data Mods (Supported)
Standard FTL 1.6 mods that modify game data, events, text, blueprints, ships, and graphical assets (XML/PNG) are compatible!
- **How it works:** FTL on this engine loads its assets from `FTL.app/ftl.dat`. Simple mods can be patched directly into `ftl.dat` on your PC using tools like **Slipstream Mod Manager (SMM)** or **ftlman** before copying the files to your Switch SD card (`sdmc:/switch/ftl/FTL.app/ftl.dat`).
- **Examples of compatible mods:** Balance mods (e.g. *Twinge's Balance & Bugfix*), custom ships, event tags, graphical enhancements (*HighRes Backgrounds*), and standard weapon packs that do not require engine bytecode modifications.

#### Hyperspace & Overhauls (Work in Progress)
- **Status:** In active development.
- **Why it is taking more time:** Frameworks like **Hyperspace** (required by massive overhauls like *Multiverse*) rely on deep native binary hooking, direct memory patching, and custom runtime code injection originally written for x86/x64 PC binaries. Porting this infrastructure to the ARM64 environment on the Switch requires substantial reverse engineering, custom memory management, and rewritten hooks. It is a work in progress, and we ask for your patience while we tackle this complexity.

---

### Settings & Configuration

Game options and port-specific configurations are handled through the game's internal data directory:
- **Save files & Settings:** Located under `sdmc:/switch/ftl/save/`.
- **Logs:** Diagnostic logs are written to `sdmc:/switch/ftl/logs/ftl.log`. Check this log if a modded `ftl.dat` causes crashes on boot.
- **Controls & UI Tweaks:** Traditional touch gestures (pinch-to-zoom, two-finger pause) and controller bindings work seamlessly out of the box with default mappings tuned for Switch handheld and docked modes.

---

### Credits & Acknowledgments

- **Subset Games**: For creating the masterpiece FTL: Faster Than Light.
- **Sidd065**: For the [ftl-android](https://github.com/Sidd065/ftl-android) project, which provided the Mach-O loader and UI adaptation foundation.
- **devkitPro & SwitchBrew**: For the `devkitA64` toolchain and `libnx`.

---
---

<a name="español"></a>
## Español

Port nativo de **FTL: Faster Than Light** (versión de iPad) para consolas Nintendo Switch con Custom Firmware (Atmosphère).

Este proyecto ejecuta de forma nativa el binario **ARM64** de iOS en la CPU de la Switch (Cortex-A57), traduciendo las llamadas del sistema y de renderizado mediante una capa de compatibilidad de alto nivel (HLE) sobre `libnx`, `EGL` y `SDL2`.

### Requisitos

1. **Nintendo Switch modificada** con Custom Firmware (Atmosphère).
2. **Copia propia de FTL para iPad v1.6.21** descifrada en formato `.ipa`.
3. Herramienta de instalación de paquetes en la consola como **DBI** (recomendado), Goldleaf o Tinfoil.

---

### Guía Rápida para Forks (Cómo crear tu versión lista para jugar)

Si has hecho un fork de este repositorio, estos son los pasos para generar y poner a punto tu juego:

#### Paso 1: Obtener el ejecutable compilado (`ftl.nro`)

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

#### Paso 2: Extraer los recursos de tu juego (.ipa)

1. Coloca tu archivo `.ipa` de FTL (versión 1.6.21) dentro de la carpeta `ipa/`.
2. Ejecuta el asistente automatizado:
   ```bash
   python tools/prepare_sd_card.py
   ```
3. El script detectará tu `.ipa` y el binario `ftl.nro`, extrayendo automáticamente el ejecutable Mach-O descifrado, los gráficos, sonidos, fuentes y creando las carpetas necesarias en `sd_card/switch/ftl/`.

#### Paso 3: Copiar a la consola y ejecutar
 
 1. Conecta la tarjeta MicroSD al PC o usa el modo MTP Responder de DBI.
 2. Copia la carpeta `switch` de `sd_card/` en la raíz de tu MicroSD (`sdmc:/switch/ftl/`).
 3. Puedes ejecutar el juego directamente desde el **Homebrew Menu** (`ftl.nro`).
 4. *(Opcional)* Si quieres tener un acceso directo en el Menú HOME, genera e instala el forwarder:
    ```bash
    python tools/build_nsp.py
    ```
    E instala `instalacion/FTL_Forwarder_[0100F71016210000].nsp` mediante DBI.

---

### Estructura en la Tarjeta MicroSD

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

### Acceso directo en el Menú HOME (Forwarder Opcional)

Puedes abrir FTL desde el Homebrew Menu o generar un acceso directo para el Menú HOME:
- Ejecuta `python tools/build_nsp.py` para compilar `instalacion/FTL_Forwarder_[0100F71016210000].nsp`.
- Instala un acceso directo directo en el Menú HOME que apunta a `sdmc:/switch/ftl/ftl.nro`.
- Las actualizaciones del binario `ftl.nro` no requieren reinstalar el forwarder.

---

### Controles

El juego está optimizado para mandos tradicionales (Joy-Con / Mando Pro) y para la pantalla táctil de Nintendo Switch.

#### Mando (Joy-Cons / Mando Pro)

| Botón | Acción |
|---|---|
| **Stick Izquierdo** | Mueve el cursor en pantalla (se ralentiza y auto-adhiere a botones, salas y puertas) |
| **A** | Pulsar / Seleccionar / Mantener para arrastrar tripulantes o energía |
| **B / Plus (+)** | Volver / Menú de pausa del juego |
| **X** | Pausa rápida del combate espacial |
| **L / R (o ZL / ZR)** | Zoom alejar / Zoom acercar sobre la nave |
| **Stick Derecho** | Desplazar la cámara libremente cuando el zoom está activo |
| **D-Pad (Cruceta)** | Movimiento preciso del cursor |

#### Pantalla Táctil

- **Toque y Arrastre**: Funciona exactamente igual que en la versión original de iPad.
- **Multitáctil**: Soporta gestos de pellizco para control del zoom y pulsación con dos dedos para pausar el combate espacial.

---

### Mods y Modding

#### Mods Sencillos / de Datos (Compatibles)
¡Los mods estándar de FTL 1.6 que modifican eventos, textos, naves, balance o recursos gráficos (XML/PNG) ya funcionan!
- **Cómo utilizarlos:** El juego carga sus recursos desde `FTL.app/ftl.dat`. Puedes aplicar mods sencillos parcheando previamente `ftl.dat` en tu PC con **Slipstream Mod Manager (SMM)** o **ftlman**, y luego copiando el archivo resultante a tu MicroSD (`sdmc:/switch/ftl/FTL.app/ftl.dat`).
- **Ejemplos de mods compatibles:** Mods de balance (*Twinge's Balance*), naves personalizadas, retoques gráficos (*HighRes Backgrounds*), nuevos eventos o armas que no requieran alterar la lógica interna del motor.

#### Hyperspace y Grandes Overhauls (En Desarrollo)
- **Estado:** En desarrollo activo.
- **Por qué requiere más tiempo:** Modificaciones como **Hyperspace** (necesario para *Multiverse*) no son simples paquetes de datos; dependen de inyección de código en memoria, enganches (*hooks*) nativos y parches binarios diseñados originalmente para PC (x86/x64). Adaptar todo ese ecosistema al binario ARM64 en el entorno de Switch es una tarea de alta complejidad técnica que llevará tiempo. Agradecemos vuestra paciencia mientras avanzamos en ello.

---

### Ajustes y Configuración

Las opciones del juego y las configuraciones del port se gestionan en:
- **Partidas guardadas y ajustes:** En la carpeta `sdmc:/switch/ftl/save/`.
- **Registros de depuración (Logs):** Se guardan en `sdmc:/switch/ftl/logs/ftl.log`. Es el primer lugar a consultar si un `ftl.dat` modificado genera algún fallo al arrancar.
- **Control y visualización:** Dispone de soporte completo táctil y mando con asignaciones preconfiguradas para modo portátil y sobremesa.

---

### Créditos y Agradecimientos

- **Subset Games**: Por crear la obra maestra FTL: Faster Than Light.
- **Sidd065**: Por el proyecto [ftl-android](https://github.com/Sidd065/ftl-android), base del cargador Mach-O y de la capa de adaptación de interfaz.
- **devkitPro & SwitchBrew**: Por la excelente cadena de herramientas `devkitA64` y la librería `libnx`.
