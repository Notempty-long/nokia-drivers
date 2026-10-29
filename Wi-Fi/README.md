# Wi-Fi — MT6631 combo, source **missing** from the reference BSP

Verified on the live device (Nokia 2.3 `TA-1206`, build `00WW_3_31J`) on 2026-10-29.

## What the phone actually runs

`/vendor/lib/modules/`:

```
wlan_drv_gen4m.ko
wmt_chrdev_wifi.ko
wmt_drv.ko
bt_drv.ko
gps_drv.ko
fmradio_drv.ko
```

That is the MediaTek **Gen4M** connectivity stack:

| Module | Role |
|---|---|
| `wmt_drv.ko` | WMT core — shared transport for Wi-Fi / BT / GPS / FM |
| `wmt_chrdev_wifi.ko` | Wi-Fi character device glue for WMT |
| `wlan_drv_gen4m.ko` | the actual Wi-Fi driver (gen4m) |

Live interfaces (`/sys/class/net`): `wlan0` and `p2p0` present, running.

## Firmware in use

`/vendor/firmware/`, with `firmware_class.path=/vendor/firmware` set in bootargs:

```
WIFI_RAM_CODE_soc1_0_1_1.bin     634040 B   <- Wi-Fi RAM code
soc1_0_ram_wifi_1_1_hdr.bin      196216 B
soc1_0_ram_mcu_1_1_hdr.bin       105400 B
soc1_0_ram_bt_1_1_hdr.bin        571025 B
soc1_0_patch_mcu_1_1_hdr.bin      26446 B
WMT_SOC.cfg                         120 B   <- selects soc1_0 vs soc3_0
BT_FW.cfg
```

Companion silicon is **MT6631** (`/vendor/firmware/mt6631_fm_v1_patch.bin`,
`mt6631_fm_v1_coeff.bin`, `fm_cust.cfg`). `soc3_0_*` images are also shipped for the
other die variant — `WMT_SOC.cfg` decides which set is loaded.

## DTS anchors

```dts
consys: consys@18002000 {
        compatible = "mediatek,mt6761-consys";
};

wifi@18000000 {
        compatible = "mediatek,wifi";
};
```

Reserved memory: `consys-reserve-memory` and `wifi-reserve-memory`.
Supply rail: `ldo_vcn33_wifi` on the MT6357.

Config in the reference tree:

```
CONFIG_MTK_COMBO=y
CONFIG_MTK_COMBO_CHIP="CONSYS_6761"
CONFIG_MTK_COMBO_CHIP_CONSYS_6761=y
CONFIG_MTK_COMBO_WIFI=y
CONFIG_MTK_COMBO_BT=y
CONFIG_MTK_COMBO_GPS=y
CONFIG_CFG80211=y
```

## The blocker: driver source is not in this tree

`drivers/misc/mediatek/connectivity/` contains only:

```
Kconfig
Makefile
common/connectivity_build_in_adapter.c
common/connectivity_build_in_adapter.h
common/wmt_build_in_adapter.c
common/wmt_build_in_adapter.h
```

There is **no** `gen4m/`, no `wmt_drv`, no `wlan_drv_gen4m` anywhere in the tree
(searched by filename — zero hits). `drivers/net/wireless/mediatek/` holds only
mac80211 `mt76` and `mt7601u`, which are unrelated to this phone.

So `CONFIG_MTK_COMBO_WIFI=y` is set against an adapter shim with no driver behind
it. The real driver lives in the separate MTK module tree
(`vendor/mediatek/kernel_modules/connectivity/…`) or in Nokia's product kernel
source. **Nothing in this repo can be built or tested for Wi-Fi until that tree is
obtained.**

## Open questions

1. Which `soc*` firmware set is live — read `WMT_SOC.cfg` contents.
2. Whether the product kernel keeps connectivity as modules (it does, per `lsmod`)
   or builds it in.
3. Whether Nokia's tree is the only copy, i.e. whether an MT6761 gen4m tree can be
   sourced from another MT6761 device's BSP.
