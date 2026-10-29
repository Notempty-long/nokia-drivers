# touch — Himax HX83102D over SPI2

Every value below was read from the live device (Nokia 2.3, `TA-1206`, build
`00WW_3_31J`, Android 11, `00WW_3_310_SP09`) on 2026-10-29, not inferred from the
reference BSP. Source of each fact is noted so it can be re-checked.

## Device identity

| Item | Value | Source |
|---|---|---|
| Model / codename | Nokia 2.3 / `Ironman_00WW` / `IRM_sprout` | `adb shell getprop` |
| Retail model | `TA-1206` | `ro.product.model.display` |
| Board | `T99621AA1_4` | `tp_color` line in `/proc/cmdline` |
| SoC model string | `MT6761V/WBB` | `/proc/device-tree/model` |
| Panel colour / SKU | `tp_color=0x83`, `lockdown_info = 0000830000000000` | `/proc/cmdline`, `/proc/touchscreen/lockdown_info` |
| Display software | `SW_T99621AA3_V049_M11_NF_NOKIA_Ironman_USR_PR31` | `ro.vendor.build.display.wtid` |

## Bus and device tree node

Touch is **SPI, not I²C** — confirmed three ways.

| Item | Value |
|---|---|
| Controller | `spi2@11012000`, `compatible = "mediatek,mt6765-spi"` |
| Chip select | `reg = <0>` → CS0 |
| Clock | `spi-max-frequency = <10000000>` → **10 MHz** |
| Node path | `/proc/device-tree/spi2@11012000/fts_touch@0` (note: name is FocalTech-style but it is a Himax part) |
| `status` | `okay` |
| Bound driver | `hxmax_generic` (`/sys/bus/spi/devices/spi2.0/driver`) |
| Interrupt | `mt-eint 0`, **Edge**, consumer `hxmax_generic` → INT on **GPIO 0** |

`/proc/interrupts` shows the binding directly:

```
 32:  205  193  216  198   mt-eint   0 Edge   hxmax_generic
```

### compatible list (as shipped)

```
focaltech_ft8006p_boe,fts
focaltech_ft8615,fts
focaltech_ft8006p,fts
mediatek,himax_cap_touch
mediatek,ilitek_cap_touch
```

Nokia ships **one DTB covering several panel suppliers**; probe order decides which
driver claims the node. On this unit the Himax one wins.

The driver in the reference tree matches on `mediatek,himax_cap_touch`
(`himax_match_table` in `hxchipset/himax_platform.c`), so that entry is the one that
matters for a Himax-only build.

## Fitted IC (from `/proc/android_touch/vendor`)

```
FW_VER      = 0x1303
TOUCH_VER   = 0x32
DISPLAY_VER = 0x00
CID_VER     = 0xE00
PANEL_VER   = 0x1D

Customer = WingTech
Project  = CTC

Himax Touch Driver Version:
2.0.0.22_Wingtech_nokia_01
```

- **HX83102D** — TDDI / in-cell, touch and display share the IC.
- ODM is **WingTech**; panel project is **CTC**.
- `PANEL_VER`/`TOUCH_VER` are the values the panel reports, useful for matching
  firmware (`Himax_firmware.bin` vs `Himax_firmware_lide.bin` in `/vendor/firmware`).
- The self-test node passes: `/proc/android_touch/self_test` reports
  `HIMAX_OPEN / MICRO_OPEN / SHORT / BPN_RAWDATA / WEIGHT_NOISE : OK`.
- Channel geometry, from `/proc/android_touch/diag/bank_s`: **32 RX × 18 TX**.

## Panel / display side

| Item | Value | Source |
|---|---|---|
| Resolution | **720 × 1520** | `/touch/lcm-resolution`, `/touch/tpd-resolution`, `fb0/modes = U:720x1520p-0` |
| Framebuffer | `/sys/class/graphics/fb0`, `name = mtkfb` | |
| Matching LCM drivers in BSP | `hx83102d_hdp_vdo_incell_{lc,lide,txd}` | `drivers/misc/mediatek/lcm/` |

`hx83102d_hdp_vdo_incell_lc` and `..._lide` both export
`hx83102d_hdp_vdo_cmd_ctc_lcm_drv` named `hx83102d_hd_plus_dsi_cmd_ctc_lcm_drv` —
"ctc" matches `Project = CTC` from the touch IC, so **CTC is the panel vendor**.
Which of `lc` / `lide` is loaded is still unresolved (see open questions).

## `/touch` node properties

`interrupt-parent` is phandle `0x1e`, which is `pio: pinctrl@10005000` in the BSP DTS
(`gpio-controller`, `#interrupt-cells = <2>`) — so `<&pio>` below is the same node.

```dts
touch: touch {
        compatible = "mediatek,touch";
        status = "okay";

        interrupt-parent = <&pio>;
        interrupts = <0 2 0 0>;      /* EINT0 = GPIO0, edge */
        vtouch-supply = <&mt_pmic_vldo28_ldo_reg>;

        lcm-resolution      = <720 1520>;
        tpd-resolution      = <720 1520>;
        tpd-max-touch-num   = <5>;
        tpd-cfg-version     = "config17";
        tpd-filter-enable   = <0>;
        tpd-filter-pixel-density = <192>;
        tpd-key-num         = <3>;
        use-tpd-button      = <0>;   /* 3 virtual keys defined, but disabled */

        pinctrl-names = "default", "state_eint_as_int",
                        "state_eint_output0", "state_eint_output1",
                        "state_rst_output0", "state_rst_output1";
        pinctrl-0 = <&ctp_eint0_default>;
        pinctrl-1 = <&ctp_eint_as_int>;
        pinctrl-2 = <&ctp_eint_output0>;
        pinctrl-3 = <&ctp_eint_output1>;
        pinctrl-4 = <&ctp_rst_output0>;
        pinctrl-5 = <&ctp_rst_output1>;
};
```

Notes on what is *not* in the DTB:

- `tpd-key-dim-local` / `tpd-key-local` describe 3 virtual keys at y=883, but
  `use-tpd-button = 0`, so they are inert.
- The six pinctrl states on the device contain **only `name` and `phandle`** — no
  `pins`, no `function`, no drive strength. Pin muxing is done in driver code, so a
  mainline-style node needs explicit `pins`/`function` entries.
- The touch slave node carries **no** `himax,irq-gpio` / `himax,reset-gpio`
  properties. This BSP variant reads GPIOs elsewhere; a `mediatek,himax_cap_touch`
  node for this driver does not need them.

## Minimum SPI node to reproduce this

```dts
&spi2 {
        status = "okay";
        #address-cells = <1>;
        #size-cells = <0>;

        touch@0 {
                compatible = "mediatek,himax_cap_touch";
                reg = <0>;
                spi-max-frequency = <10000000>;
                status = "okay";
        };
};
```

`reg = <0>` and `10000000` are measured, not guessed. INT is GPIO 0; reset pin is
still unknown (see below).

## The shipped driver *is* in this tree

An earlier suspicion that Nokia ran a private driver is wrong — the strings match
exactly:

```c
/* drivers/input/touchscreen/mediatek/hxchipset/himax_common.h */
#define HIMAX_common_NAME   "hxmax_generic"
#define HIMAX_DRIVER_VER    "2.0.0.22_Wingtech_nokia_01"
```

`hxmax_generic` is the driver name in `/sys/bus/spi/devices/spi2.0/driver`, and
`2.0.0.22_Wingtech_nokia_01` is what `/proc/android_touch/vendor` reports. So the
`hxchipset/` directory in
`C:\n23\src\LINUX\android\kernel\drivers\input\touchscreen\mediatek\hxchipset\`
**is** the production source for this phone's touch stack, IC-specific file
`himax_ic_HX83102.c` included.

The source comes from the public **Nokia 2.3 V3.311** tarball; provenance, tree path
and the exact config symbols to enable (`TOUCHSCREEN_HIMAX_CHIPSET`,
`..._COMMON`, `..._INCELL`, `..._IC_HX83102`) are in `../KERNEL_SOURCE.md`. The
`himax_mmi` module loads `Himax_firmware.bin` from `/vendor/firmware` at runtime, so
the embedded-firmware option stays off unless you want the blob linked in.

That means the stub below is **superseded by working code**: for a functional touch
stack, enable the vendor driver instead of loading `hx83102_fw.ko`.

What the BSP still lacks is only the product DTS wiring: no Himax node exists in any
`arch/arm64/boot/dts/mediatek/*.dts{,i}`, and both
`cust_mt6761_touch_{720x1280,1080x1920}.dtsi` point at `goodix,gt1151`, whose driver
is disabled in `.config`. Those two files are stale and must not be used as a
template.

## Minimal module

`touch/hx83102_fw.c` is a deliberately inert probe: it binds
`mediatek,himax_cap_touch` on SPI2, calls `request_firmware()` for
`Himax_firmware.bin`, logs the size and a header hexdump, then stops. No register
access, no input device, no interrupt handler, no firmware upload.

```
sudo insmod hx83102_fw.ko                      # defaults are the measured values
sudo insmod hx83102_fw.ko fw_name=Himax_firmware_lide.bin spi_speed=10000000
dmesg | grep hx83102_fw
```

**Do not load it alongside the in-tree `hxchipset` driver** — both claim
`mediatek,himax_cap_touch`, so whichever probes first owns the device.

## Open questions

1. **Reset GPIO number.** The DTB only names the states `state_rst_output0/1`, and
   the pinctrl nodes are empty. Needs a boot-time kernel log (`dmesg` immediately
   after reboot, before the WLAN driver floods the ring buffer) or a `pinmux-pins`
   dump — `debugfs` is not mounted and `/sys/kernel/debug` cannot be created on this
   build.
2. **LCM variant: `lc` vs `lide`.** Both ship the CTC driver. `PANEL_VER = 0x1D` and
   `tp_color = 0x83` are the discriminators a product DTS would use.

## Reproducing the dump

Root is available on this unit via Magisk (`su -c` works; `adb root` does not —
production build). Read the tree as root, then filter locally:

```powershell
.\platform-tools\adb.exe shell "su -c 'cp -r /proc/device-tree/. /data/local/tmp/dt/; tar -C /data/local/tmp -cf /data/local/tmp/dt.tar dt'"
.\platform-tools\adb.exe pull /data/local/tmp/dt.tar .
```

```powershell
.\platform-tools\adb.exe shell "su -c 'cat /proc/interrupts'" | Select-String mt-eint
.\platform-tools\adb.exe shell "su -c 'cat /proc/android_touch/vendor'"
.\platform-tools\adb.exe shell "su -c 'cat /proc/android_touch/self_test'"
```

`/proc/device-tree` and `/sys/bus/spi/devices/spi2.0/modalias` are **unreadable as
the `shell` user** (SELinux); `su -c` is required.
