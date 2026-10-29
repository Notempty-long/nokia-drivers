# PMIC — MT6357 plus an external charger IC

The phone does **not** run on MT6357 alone. The PMIC handles regulators, the ADC /
battery temperature, the fuel gauge path and the audio accessory detect; a separate
I²C charger chip handles charging. All values below were read from the live device
(Nokia 2.3 `TA-1206`, build `00WW_3_31J`) on 2026-10-29.

## What the running kernel exposes

`/sys/class/power_supply/`:

| Supply | Type | Meaning |
|---|---|---|
| `battery` | `Battery`, `status = Charging`, `technology = Li-ion` | main battery, MT6357 gauge |
| `bq2415x-gauge` | `USB`, `status = Charging` | **external charger IC** |
| `mtk_charger_type` | `USB` | MTK charger-type detection |
| `mtk-master-charger` / `mtk-slave-charger` | `Unknown` | MTK charger framework |
| `mtk-gauge` | `Unknown` | MT6357 gauge |
| `ac` / `usb` | `Mains` / `USB` | legacy inputs |

The charger IC is an **ETA6937**, driven by MTK's bq2415x-family driver. The kernel
log tag is still `bq2415x` while the functions are `eta6937_*`:

```
[bq2415x]:eta6937_dump_regs: eta6937_dump_regs Reg[02] = 0xBA
[bq2415x]:eta6937_check_status: Charge State:Charging
[bq2415x]:eta6937_check_status: otg pin:High
[bq2415x]:eta6937_check_status: Fault Status:Normal
```

Source: `console-ramoops-0` in `/sys/fs/pstore/`.

## PMIC identity

- `mt6357` sits on `pwrap@1000d000/main_pmic`, `compatible = "mediatek,mt6357"`.
- Charger/gauge/charger-type nodes hang off it: `mt6357-charger-type`,
  `mt6357-gauge`, `mtk_charger`, `mtk_battery`.
- Battery profiles in the BSP DTS are `JIADE_VK` and `FENGHUA_FHPV`, each with a
  100-point OCV table. These are **device-specific tuning** — a generic BSP build
  without the product DTS loses them.
- Both are worth keeping: the two profiles imply Nokia shipped two battery vendors
  under the same model.

## Driver sources in the BSP

`drivers/power/supply/`:

```
mt6357-charger-type.c   mt6357-gauge.c        mt6357-pulse-charger.c
mtk_charger.c           mtk_battery.c         mtk_battery_algo.c
mtk_battery_daemon.c    mtk_pd.c              mtk_pe2.c / mtk_pe4.c
bq24157_charger.c       bq2415x_charger.c     bq24157_reg.h
```

Config state in the reference tree:

```
CONFIG_CHARGER_BQ2415X is not set
CONFIG_MTK_BQ24157_SUPPORT=y
CONFIG_MFD_MT6370_PMU=y  (+ CHARGER/FLED/BLED/LDO/DSV)
```

## The ETA6937 driver is in this tree

Earlier notes suspected the tree's charger config did not match the phone. It does.
The ETA6937 support lives **inside `bq24157_charger.c`**, alongside the BQ24157 and
HL7057 variants:

```c
/* drivers/power/supply/bq24157_charger.c */
#define ETA6937_RE  4                                   /* :52  */
#define ETA6937     1                                   /* :55  */
static int eta6937_reinit(struct bq2415x *bq);           /* :156 */
/* distinguish which charger ic we use,vendor:ETA6937,BQ24157,HL7057  (:164) */
```

That is why the kernel log shows the `[bq2415x]` tag on `eta6937_*` functions — one
driver, several IC variants, selected at runtime.

Config:

```
CONFIG_MTK_BQ24157_SUPPORT=y     <- the one to enable (Kconfig:811)
CONFIG_CHARGER_BQ2415X is not set <- mainline TI driver, different part, leave off
CONFIG_MFD_MT6370_PMU=y  (+ CHARGER/FLED/BLED/LDO/DSV)
```

`CONFIG_MTK_BQ24157_SUPPORT` is already set in the tree's `T99652AA1_defconfig`
(the other WingTech board), which is a useful template even though this phone's own
defconfig is not in the tarball. See `../KERNEL_SOURCE.md`.

Also loaded on the device: the whole `mt6370_pmu_*` module set
(`mt6370_pmu_charger`, `_fled`, `_bled`, `_ldo`, `_dsv`, `_i2c`, `_core`). MT6370 is
a *different* PMIC — companion/charger silicon. Its presence in `lsmod` does not
prove the IC is fitted; treat it as unresolved and check the I²C bus before porting
anything around it.

## DTS anchors

```dts
pwrap@1000d000 {
        main_pmic {
                compatible = "mediatek,mt6357";
                mt6357regulator { ... ldo_vcn33_wifi, ldo_vldo28 ... };
                pmic_accdet: pmic_accdet { compatible = "mediatek,mt6357-accdet"; };
                pmic_codec:  pmic_codec  { compatible = "mediatek,mt6357-sound";  };
        };
};
```

`ldo_vldo28` is the touch supply (`/touch/vtouch-supply = <&ldo_vldo28>`), and
`ldo_vcn33_wifi` feeds the connectivity block.

## The driver lives here now

`PMIC/bq24157/` holds the **production charger driver**, copied unmodified from the
vendor tree (`bq24157_charger.c` + `bq24157_reg.h`), plus a `Kbuild` of ours.

- One driver, three IC variants: `ETA6937`, `BQ24157`, `HL7057`, picked at runtime.
- I²C client, `power_supply` + MTK `charger_class` device, kthread polling.
- `pr_fmt` is `"[bq2415x]"`, so its messages look like the TI driver's.

```sh
make KDIR=C:/n23/src/LINUX/android/kernel PMIC/bq24157
sudo insmod bq24157_charger.ko
```

It needs the MTK charger framework built into the target kernel
(`CONFIG_MTK_CHARGER`, for `charger_class`/`charger_device`) and `mtk_charger.h`
from `drivers/power/supply/`, so `KDIR` must be the vendor tree.

**It has not been compiled.** The earlier `mt6357_stub.c` was removed: it
registered nothing and this is the real driver.

The PMIC itself (MT6357 regulators, gauge, accdet) still has no loadable firmware -
those are in-tree drivers driven by the product DTS, not by a module here.

## Open questions

1. Which I²C bus and address the ETA6937 sits on — not present in the reference DTS.
2. Whether MT6370 is physically fitted or only its drivers are loaded.
3. Whether both battery profiles are live, or only `JIADE_VK`.
