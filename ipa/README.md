# FTL iOS Package Directory / Archivos de juego iOS

[English](#english) | [Español](#español)

---

<a name="english"></a>
## English

Place your decrypted copy of **FTL: Faster Than Light for iPad (version 1.6.21)** in `.ipa` format inside this directory (e.g. `FTL_1.6.21_ios_8.0.ipa` or any `.ipa` file).

### How to Use

Once your `.ipa` file is placed here:

1. Run the preparation tool:
   ```bash
   python tools/prepare_sd_card.py
   ```
2. The script will automatically detect the `.ipa` package and extract the ARM64 executable (`FTL`), the resource folder (`FTL.app`), fonts, and audio into `sd_card/switch/ftl/`.
3. Copy the resulting `switch` folder to the root of your MicroSD card (`sdmc:/`).

> **Legal notice:** For copyright and intellectual property reasons, this repository does not include or distribute proprietary material from Subset Games. Each user must provide their own legitimately acquired copy.

---

<a name="español"></a>
## Español

Coloca en esta carpeta tu copia descifrada de **FTL: Faster Than Light para iPad (versión 1.6.21)** en formato `.ipa` (por ejemplo `FTL_1.6.21_ios_8.0.ipa` o cualquier archivo con extensión `.ipa`).

### Cómo usarlo

Una vez colocado el archivo `.ipa` en esta carpeta:

1. Ejecutamos el asistente de preparación:
   ```bash
   python tools/prepare_sd_card.py
   ```
2. El script detectará automáticamente el archivo `.ipa` y extraerá el ejecutable ARM64 (`FTL`), la carpeta de recursos (`FTL.app`), fuentes y gráficos en la carpeta `sd_card/switch/ftl/`.
3. Copiamos la carpeta `switch` resultante en la raíz de la tarjeta MicroSD (`sdmc:/`).

> **Aviso legal:** Por motivos de derechos de autor y propiedad intelectual, este repositorio no incluye ni distribuye material protegido de Subset Games. Cada usuario debe utilizar su propia copia adquirida de forma legítima.
