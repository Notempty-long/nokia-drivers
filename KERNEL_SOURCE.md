# Vendor kernel source

This is the source of truth for every subsystem in this repo. Nothing here is
guessed: each row below was checked against the extracted tree, and the case where
the drivers **already exist** is called out separately from the cases where they do
not.

## Provenance

| Item | Value |
|---|---|
| URL | `https://nokiaphones-opensource.azureedge.net/download/phones/Nokia2.3_V3.311.tar.bz2` |
| HTTP check | `200`, `application/x-bzip2`, `176 489 901` bytes (168 MiB) — verified 2026-10-29 |
| Local extraction | `C:\n23\src\LINUX\android\kernel` |
| Kernel version | **4.19.127** (`Makefile`: 4.19.127) |
| Tree size | 74 069 files, 951 MB |
| Arch | **arm64** — `arch/arm64/configs/k61v1_64_bsp_defconfig` |
| Platform string | `mt6761` (`CONFIG_MTK_PLATFORM="mt6761"`) |

Re-download (PowerShell, ~168 MB):

```powershell
$u = 'https://nokiaphones-opensource.azureedge.net/download/phones/Nokia2.3_V3.311.tar.bz2'
Invoke-WebRequest $u -OutFile Nokia2.3_V3.311.tar.bz2 -UseBasicParsing
mkdir C:\n23 -Force
tar -xjf Nokia2.3_V3.311.tar.bz2 -C C:\n23     # Windows tar.exe handles bzip2
```

## What this drop is, and what it is not

It is the **generic Wingtech/MediaTek MT6761 tree** (`k61v1_64_bsp`), not a
board-specific product tree.

Project defconfigs that ship in it:

```
T99652AA1_defconfig  T99652AA1_debug_defconfig    <- not this phone
T99653AA1_defconfig  T99653AA1_debug_defconfig    <- not this phone
k61v1_64_bsp_defconfig  k61v1_64_bsp_debug_defconfig
```

This phone is board **`T99621AA1_4`** (`TA-1206`, `Ironman_00WW`). **There is no
`T99621AA1` defconfig in the drop.** The product config, the product DTS and the
LCM/touch wiring therefore still have to come from the phone or be reconstructed —
the generic tree alone will not produce a bootable image for this model.

A useful side effect: the two `T99*` configs are the same ODM's (WingTech) boards,
so they show *how* Nokia enables these blocks, even though they are not our board.

## Where the real drivers are

| Subsystem | Path in the tree | Config symbol | Status |
|---|---|---|---|
| touch | `drivers/input/touchscreen/mediatek/hxchipset/` | `TOUCHSCREEN_HIMAX_COMMON`, `..._IC_HX83102` | **complete** |
| charger | `drivers/power/supply/bq24157_charger.c` | `MTK_BQ24157_SUPPORT` | **complete (ETA6937)** |
| PMIC / gauge | `drivers/power/supply/mt6357-*.c`, `mtk_charger.c`, `mtk_battery.c` | `MTK_CHARGER` | **complete** |
| audio | `sound/soc/mediatek/common_int/` | `MTK_SND_SOC_NEW_ARCH` | **complete** |
| Wi-Fi | *not in the kernel tree* — `vendor/mediatek/kernel_modules/connectivity/` | `MTK_COMBO`, `..._CONSYS_6761` | **source absent** |

### touch — `hxchipset`

16 files, including `himax_ic_HX83102.c`. This is the exact production driver:

```c
/* himax_common.h */
#define HIMAX_common_NAME "hxmax_generic"
#define HIMAX_DRIVER_VER  "2.0.0.22_Wingtech_nokia_01"
```

and the live device reports both strings (`/sys/bus/spi/devices/spi2.0/driver` →
`hxmax_generic`, `/proc/android_touch/vendor` → `2.0.0.22_Wingtech_nokia_01`).
The build target is the `himax_mmi` module. Firmware name is
`Himax_firmware.bin`; `Himax_firmware_lide.bin` is the alternate panel variant.

The Kconfig text says "i2c touchscreen", but this tree's driver is the SPI one used
by this phone — the help string is stale upstream wording, not evidence about the bus.

### charger — `bq24157_charger.c` is the ETA6937 driver

Earlier notes suspected the tree's charger config did not match the phone. It does —
the ETA6937 support is inside `bq24157_charger.c`:

```c
/* bq24157_charger.c */
#define ETA6937_RE  4                                   /* :52  */
#define ETA6937     1                                   /* :55  */
static int eta6937_reinit(struct bq2415x *bq);           /* :156 */
/* distinguish which charger ic we use,vendor:ETA6937,BQ24157,HL7057  (:164) */
```

Kconfig symbol: `MTK_BQ24157_SUPPORT` (`drivers/power/supply/Kconfig:811`), described
as "Charger Driver for BQ BQ24157 Charger IC". The `[bq2415x]` printk tag comes from
this file, which is why the log looks like the TI driver while the code is ETA6937.
`CONFIG_CHARGER_BQ2415X` (the mainline TI driver) stays **off** — it is a different
driver and would fight this one.

### audio — `common_int`

`sound/soc/mediatek/common_int/mtk-soc-machine.c` defines `.name = "mt-snd-card"`,
matching card 0 on the device. MT6761 has no per-SoC machine driver; `mt6765` is the
sibling directory. `mt6357-accdet.ko` ships as a module.

### Wi-Fi — the source is genuinely not in this drop, and here is the proof

`drivers/misc/mediatek/connectivity/` contains **only** the adapter shims:

```
Kconfig
Makefile
common/connectivity_build_in_adapter.{c,h}
common/wmt_build_in_adapter.{c,h}
```

The tree's own `Makefile` explains why:

```make
# Do Nothing, move to standalone repo
MODULE_NAME := connadp
...
ifeq ($(CONFIG_WLAN_DRV_BUILD_IN),y)
    PATH_TO_WMT_DRV      = vendor/mediatek/kernel_modules/connectivity/common
    PATH_TO_WLAN_CHR_DRV = vendor/mediatek/kernel_modules/connectivity/wlan/adaptor
    PATH_TO_WLAN_DRV     = vendor/mediatek/kernel_modules/connectivity/wlan/core/gen4m
```

So the gen4m Wi-Fi driver was moved out of `kernel/` into the vendor module tree
(`vendor/mediatek/kernel_modules/connectivity/wlan/core/gen4m`), which is part of the
Android **vendor** source, not of this kernel tarball. `k61v1_64_bsp_defconfig` still
sets `CONFIG_MTK_COMBO_CHIP_CONSYS_6761=y` (Cortex "CONNAC" combo), so the build
expects that separate tree to be present.

Consequence: **no rebuild of this kernel can produce `wlan_drv_gen4m.ko`.** Wi-Fi is
the one block this tarball cannot fix. The phone's working binary is
`/vendor/lib/modules/wlan_drv_gen4m.ko`; recovering source means obtaining the vendor
`kernel_modules` tree, not the kernel one.

## Building against this tree

Les modules build out-of-tree; the root `Makefile` picks this tree up automatically
when it exists:

```powershell
make                    # uses C:/n23/src/LINUX/android/kernel if present
make KDIR=D:/other/tree # or point it anywhere
```

**It has not been built, and cannot be on this host.** There is no arm64 compiler:
`C:\n23\bin` holds only `arm-linux-gnueabi-*` (32-bit) stubs and `make.exe`, and
`aarch64-linux-gnu-gcc`, `clang`, `gcc`, `perl`, `bc`, `flex`, `bison` are all
missing from `PATH`. A Linux host with an `aarch64` cross toolchain (or an Android
kernel build environment) is required before any claim of "working" can be made.

Filesystem note: the tree is on NTFS, which is case-insensitive. Building a kernel on
a case-insensitive filesystem is unreliable — the kernel source contains filenames
that differ only by case. Build on a Linux filesystem in the end.

## Config fragment

`configs/nokia23-mt6761.fragment` lists the symbols the real drivers need. Merge it
into the defconfig before building:

```sh
scripts/kconfig/merge_config.sh -m arch/arm64/configs/k61v1_64_bsp_defconfig \
    ../configs/nokia23-mt6761.fragment
make ARCH=arm64 O=../out olddefconfig
make ARCH=arm64 O=../out -j"$(nproc)"
```

The fragment is **unverified** — no symbol in it has been compiled on this host. It
is derived from the sources above, not copied from a working `.config`.
