# Reddit Post Draft: FTL for Nintendo Switch

## Title Suggestions
- [Release] FTL: Faster Than Light - Native Port for Nintendo Switch
- FTL: Faster Than Light native port for Nintendo Switch is now available

---

## Post Content

Hi everyone,

I'm excited to share a native port of **FTL: Faster Than Light** (iPad Edition) for the Nintendo Switch running Atmosphère custom firmware.

This port runs natively rather than via emulation, performance and battery efficiency are excellent.

---

### Key Features

- Native execution with full hardware-accelerated rendering.
- Complete touchscreen support (tap, drag, pinch-to-zoom, two-finger pause) matching the iPad experience.
- Full gamepad controls (Joy-Cons / Pro Controller) with cursor snapping to UI elements, rooms, and doors.
- Optional HOME Menu forwarder generator script included (`tools/build_nsp.py`).
- Automated asset extraction and preparation script.

---

### Controls Overview

#### Controller (Joy-Cons / Pro Controller)
- Left Stick: Move cursor (snaps to buttons, rooms, and doors)
- D-Pad: Fine cursor positioning
- A: Tap / Select / Drag crew or power
- B / Plus (+): Back / Game Menu
- X: Quick pause in combat
- L / R (or ZL / ZR): Zoom out / Zoom in
- Right Stick: Pan camera while zoomed in

#### Touchscreen
- Standard touch: Drag, drop, and interact just like on iPad.
- Multitouch: Pinch-to-zoom and two-finger tap to pause.

---

### Requirements

1. A Nintendo Switch running Atmosphère custom firmware.
2. A decrypted copy of FTL for iPad (v1.6.21) in .ipa format (you must provide your own legally dumped game files).

---

### Quick Setup Guide

1. Download the latest `ftl.nro` from Releases (or clone and build the repo).
2. Place your decrypted `FTL_1.6.21.ipa` file into the `ipa/` folder.
3. Run the automated preparation tool:
   ```bash
   python tools/prepare_sd_card.py
   ```
   This unpacks the Mach-O binary, textures, audio, fonts, and sets up `sd_card/switch/ftl/`.
4. Copy the `switch` folder from `sd_card/` into the root of your MicroSD card (`sdmc:/switch/ftl/`).
5. Launch FTL from your **Homebrew Menu** (Album).
6. *(Optional)* If you prefer launching directly from the HOME Menu, generate and install the forwarder:
   ```bash
   python tools/build_nsp.py
   ```
   and install the generated NSP using DBI.

---

### MicroSD Layout

```text
sdmc:/
  └── switch/
        └── ftl/
              ├── ftl.nro               <-- Switch executable
              ├── FTL                   <-- iPad ARM64 binary
              ├── FTL.app/              <-- Game assets (ftl.dat, audio, fonts)
              ├── save/                 <-- Game saves
              ├── cache/                <-- Runtime cache
              └── logs/                 <-- Debug logs (ftl.log)
```

---

### Source Code and Downloads

- GitHub Repository: https://github.com/eortas/FTL_switch
- Releases: https://github.com/eortas/FTL_switch/releases

---

### Support the Project

If you enjoy the port and would like to support ongoing development:
- Starring the repository on GitHub is greatly appreciated and helps more people discover the project!
- If you'd like to buy me a coffee: [PayPal Donation](https://paypal.me/edarment) (completely optional, but welcome).

---

### Credits & Acknowledgments

- Subset Games: For creating FTL: Faster Than Light.
- Sidd065: For the ftl-android project and Mach-O loader foundation.
- devkitPro & SwitchBrew: For devkitA64 and libnx.

