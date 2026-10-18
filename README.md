# FTL: Faster Than Light para Nintendo Switch

Port nativo de **FTL: Faster Than Light** (versión de iPad) para consolas Nintendo Switch con Custom Firmware (Atmosphère).

Este proyecto ejecuta de forma nativa el binario **ARM64** de iOS en la CPU de la Switch (Cortex-A57), traduciendo las llamadas del sistema y de renderizado mediante una capa de compatibilidad de alto nivel (HLE) sobre `libnx`, `EGL` y `SDL2`.

---

## Requisitos

1. **Nintendo Switch modificada** con Custom Firmware (Atmosphère) y acceso al Homebrew Menu (modo título / full RAM recomendado manteniendo R al abrir un juego).
2. **Copia de FTL para iPad v1.6.21** descifrada (decrypted IPA o carpeta `FTL.app`).
3. El archivo Homebrew `ftl.nro` compilado.

---

## Instalación en la Tarjeta MicroSD

1. Copia el archivo `ftl.nro` en la carpeta `switch/ftl/` de tu tarjeta SD.
2. De tu copia descifrada de FTL de iPad (versión 1.6.21), extrae el contenido de `Payload/FTL.app/` y colócalo en `switch/ftl/`:
   - El ejecutable `FTL` (binario Mach-O).
   - Los archivos de recursos: `data.dat`, `resource.dat`, fuentes y sonido.
3. La estructura en la tarjeta MicroSD debe quedar así:

```text
sdmc:/
  └── switch/
        └── ftl/
              ├── ftl.nro               <-- Ejecutable de Homebrew
              ├── FTL                   <-- Binario Mach-O descifrado de iPad
              ├── FTL.app/              <-- (Opcional, si mantienes la estructura de paquete)
              ├── data.dat              <-- Datos del juego
              ├── resource.dat          <-- Gráficos y audio
              └── save/                 <-- Partidas guardadas (se genera automáticamente)
```

4. Inserta la MicroSD en tu consola, entra en el **Homebrew Menu** y ejecuta **FTL: Faster Than Light**.

---

## Controles

El juego está adaptado tanto para mandos tradicionales como para la pantalla táctil de la consola.

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

- **Toque / Arrastre**: Funciona exactamente igual que en el iPad original.
- **Multitáctil**: Soporta gestos de pellizco para zoom y dos dedos para pausar el combate.

---

## Compilación

### Método 1: GitHub Actions (Recomendado)
Sube este repositorio a tu cuenta de GitHub. En la pestaña **Actions**, ejecuta el workflow `Build Nintendo Switch Homebrew`. Al finalizar, descarga el artefacto `ftl-switch-build` que contiene el archivo `ftl.nro` listo para usar.

### Método 2: En local con devkitPro (devkitA64)
Si dispones de devkitPro instalado con el toolchain de Switch:

```bash
cmake -G Ninja -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=$DEVKITPRO/cmake/Switch.cmake \
  -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

---

## Créditos y Agradecimientos

- **Subset Games**: Desarrolladores del fantástico juego original FTL: Faster Than Light.
- **Sidd065**: Creador de [ftl-android](https://github.com/Sidd065/ftl-android), de cuyo cargador Mach-O y capa de adaptación de interfaz deriva este port.
- **devkitPro & SwitchBrew**: Por la cadena de herramientas `devkitA64` y la librería `libnx`.
