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

## Mismatch to be aware of

The reference `.config` does **not** build `bq2415x_charger.c`, yet the live device
runs a bq2415x-family driver. Nokia's build therefore differs from this tree's
config in the charger area, most likely by enabling `CONFIG_CHARGER_BQ2415X` (or a
patched ETA6937 variant of it) in the product defconfig.

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

## Minimal module

`PMIC/mt6357_stub.c` is a pure stub — no bus, no `power_supply`, nothing
registered. It only logs what it would drive, because **the PMIC block has no
loadable firmware**: registers, battery profile and charger tables are all
compiled-in data.

An optional `fw_name` parameter exists so a board that later gains a blob
(battery profile / charger table) can be exercised without editing the file.

## Open questions

1. Which I²C bus and address the ETA6937 sits on — not present in the reference DTS.
2. Whether MT6370 is physically fitted or only its drivers are loaded.
3. Whether both battery profiles are live, or only `JIADE_VK`.
