# Nokia 2.3 Linux Drivers (`nokia-drivers`)

Staging and documentation repository for Linux kernel driver work on the Nokia 2.3 retail model `TA-1206` (codename `Ironman_00WW` / `IRM_sprout`).

This project is intentionally a working archive and reference set for kernel driver bring-up, not a completed or verified Linux build. The vendored driver sources are preserved as-is from the Nokia 2.3 V3.311 BSP, while the remaining files are bring-up helpers and documentation scaffolding.

---

## Target Hardware Profile

* SoC: MediaTek MT6761 (Helio A22), model string `MT6761V/WBB`
* Kernel base: Linux 4.19.127 from the Nokia 2.3 official source drop `V3.311`
* Device target: Nokia 2.3 / `TA-1206` / `Ironman_00WW` / `IRM_sprout`
* Android build: `00WW_3_31J` / `00WW_3_310_SP09`
* Board ID: `T99621AA1_4`

## Current Status

| Subsystem | Component / IC | Interface / Architecture | Status |
|---|---|---|---|
| Touch | Himax `HX83102D` TDDI | SPI2 (`CS0`, 10 MHz), INT on GPIO 0 | Vendored real driver in `touch/hxchipset/` |
| PMIC / Charger | MT6357 PMIC + `ETA6937` charger | I²C charger (`bq24157_charger.c`), PMIC PWR/AP side | Vendored real driver in `PMIC/bq24157/` |
| Wi‑Fi / BT | MediaTek `MT6631` combo (Gen4M) | CONSYS (`consys@18002000`) | Not working; firmware-probe stub only, no vendor driver source in-tree |
| Audio | MT6357 codec and jack detect | Shared `common_int` machine driver (`mt-snd-card`) | Not working; stub/probe only, no verified audio port |

### Important notes

* `touch/hxchipset/` and `PMIC/bq24157/` are the real vendor drivers, copied from the official Nokia 2.3 V3.311 tree and preserved with their GPL-2.0 headers and provenance notes.
* `Wi‑Fi/mt6631_fw.c` and `audio/mt6357_codec_stub.c` are inert bring-up helpers. They log and describe the hardware, but they are not a working port and should not be described as functional drivers.
* The repository has no CI, no test suite, and the top-level `Makefile` has never been run here. Kernel modules have not been compiled in this environment, and this repository should not be described as build-ready.
* The live device tree differs from the decompiled reference in `the devicetree.dts`; that file is kept only as historical reference material.

---

## Repository Structure

```text
├── AGENTS.md                  # Project instructions and context
├── KERNEL_SOURCE.md           # Provenance and file/symbol map for the V3.311 drop
├── Makefile                   # Top-level out-of-tree kbuild driver builder
├── the devicetree.dts         # Decompiled reference DTB (for reference only)
├── README.md                  # This overview and status document
├── audio/                     # Audio topology documentation and stub/probe notes
├── configs/                   # Target kernel configuration fragments
│   └── nokia23-mt6761.fragment
├── PMIC/                      # BQ24157 / ETA6937 charger driver + Kbuild
├── touch/                     # Himax HX83102D driver source (hxchipset) + Kbuild
├── Wi-Fi/                     # MT6631 firmware loader and blob holder
└── assets/                    # Visual assets for the repository
```

---

## Scope and Constraints

* This repository is a staging area for reverse-engineering and kernel driver investigation.
* Real vendor code is kept under `touch/` and `PMIC/` and must retain the original GPL-2.0 provenance notices.
* The Wi‑Fi and audio pieces are intentionally documented as non-working scaffolding, not working ports.
* Cross-compilation for the MT6761 arm64 kernel is not available in this environment; real builds require a Linux host with matching kernel headers under `/lib/modules/$(uname -r)/build` and a proper toolchain.

---

## Notes for Contributors

Before claiming anything works, confirm the following:

1. The relevant kernel config and DT bindings are present for the target board.
2. The vendored driver source is built against the correct MT6761 kernel tree.
3. The subsystem is tested on a real Nokia 2.3 device with the proper Android/Linux environment.
4. The `Wi‑Fi` and `audio` modules are explicitly called out as non-working unless there is concrete evidence to the contrary.

This repository is a reference and staging point for the Nokia 2.3 Linux driver effort, not a claim of a completed or stable kernel port.
