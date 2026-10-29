# ![Nokia 2.3](assets/nokia23.png) Linux Drivers (`nokia-drivers`)

Staging and documentation repository for Linux kernel driver work on the **Nokia 2.3** retail model `TA-1206` (codename `Ironman_00WW` / `IRM_sprout`).

---

## Target Hardware Profile

* **SoC:** MediaTek MT6761 (Helio A22), model string `MT6761V/WBB`.
* **Kernel Base:** Linux 4.19.127 (from Nokia 2.3 official source drop `V3.311`).
* **Subsystem Breakdown:**

| Subsystem | Component / IC | Interface / Architecture | Driver Source Status |
|---|---|---|---|
| **Touch** | Himax **HX83102D** TDDI | SPI2 (CS0, 10 MHz), INT on GPIO 0 | Vendored (`touch/hxchipset/`) |
| **PMIC / Charger** | MT6357 PMIC + **ETA6937** Charger | I²C charger (`bq24157_charger.c`), pwrap PMIC | Vendored (`PMIC/bq24157/`) |
| **Audio** | MT6357 codec & jack detect | Shared `common_int` machine driver (`mt-snd-card`) | In-tree / Documented |
| **Wi-Fi / BT** | MediaTek **MT6631** (Gen4M combo) | CONSYS (`consys@18002000`) | Firmware probe (`Wi-Fi/`) — vendor source separate |

---

## Repository Structure

```text
├── AGENTS.md                  # Project instructions and context
├── KERNEL_SOURCE.md           # Provenance and file/symbol map for the V3.311 drop
├── Makefile                   # Top-level out-of-tree kbuild driver builder
├── the devicetree.dts         # Decompiled reference DTB
├── audio/                     # Audio topology documentation & stubs
├── configs/                   # Target kernel configuration fragments
│   └── nokia23-mt6761.fragment
├── PMIC/                      # BQ24157 / ETA6937 charger driver + Kbuild
├── touch/                     # Himax HX83102D driver source (hxchipset) + Kbuild
└── Wi-Fi/                     # MT6631 firmware loader and blob holder
