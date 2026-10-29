# audio — MT6357 codec on the generic MTK machine driver

Verified on the live device (Nokia 2.3 `TA-1206`, build `00WW_3_31J`) on 2026-10-29.

## Sound cards present

`/proc/asound/cards`:

```
0 [mtsndcard    ]: mt-snd-card - mt-snd-card
                     mt-snd-card
1 [mt63xxaccdet ]: mt63xx-accdet - mt63xx-accdet
                     mt63xx-accdet
```

- Card 0 `mt-snd-card` is the playback/capture card.
- Card 1 `mt63xx-accdet` is the headset jack detect, a separate card.

## Which machine driver

The `"mt-snd-card"` string resolves to **`common_int/mtk-soc-machine.c`**:

```c
/* sound/soc/mediatek/common_int/mtk-soc-machine.c */
.name = "mt-snd-card",
```

That closes the earlier "there is no mt6761 machine driver" question: MT6761 does
**not** get its own machine driver. It uses the shared `common_int` one, with
`mt6765` as the sibling SoC directory. Nothing is missing here.

Config that selects it:

```
CONFIG_MTK_SND_SOC_NEW_ARCH=y
CONFIG_MTK_SND_SOC_CODEC="mt6357"
CONFIG_SND_SOC_MTK_BTCVSD=y
```

and disabled: `CONFIG_SND_SOC_MTK_SMART_PHONE`, `CONFIG_SND_SOC_MTK_SRAM`,
`CONFIG_SND_SOC_MTK_AUDIO_DSP`.

## Hardware nodes

```dts
audio: audio@11220000 {
        compatible = "mediatek,audio", "syscon";
        /* AFE, 18 clocks including aud_afe_clk, aud_dac_clk, aud_apll22m_clk */
};

audio_sram@11221000 { ... };

/* inside the MT6357 sub-PMIC */
pmic_codec: pmic_codec {
        compatible = "mediatek,mt6357-sound";
        io-channel-names = "pmic_codec", "pmic_accdet";
};

pmic_accdet: pmic_accdet {
        compatible = "mediatek,mt6357-accdet";
        accdet-name = "mt63xx-accdet";
        accdet-mic-vol = <0x06>;
        accdet-mic-mode = <0x01>;
};
```

Both audio and headset-detect are **inside the MT6357 PMIC**, not separate chips —
so `audio/` and `PMIC/` overlap by design here.

## Relevant tree layout

```
sound/soc/mediatek/
  common_int/      <- mtk-soc-machine.c : "mt-snd-card" (the one in use)
  common/          <- AFE platform driver, BT CVSD, SRAM manager, USIP
  codec/           <- MT6357 codec glue
  audio_dsp/       <- disabled by config
  aw87519/         <- external speaker amp driver
  mt2701/ mt6765/ mt6779/ mt6797/ mt8173/   <- per-SoC dirs, NOT used by mt6761
```

`mt6765` is the closest sibling platform if per-SoC values are ever needed.

Also noted on the device: `/vendor/lib/modules/mt6357-accdet.ko` is loaded as a
module, so the jack-detect part is a loadable module while the codec is built in.

## Minimal module

`audio/mt6357_codec_stub.c` is a pure stub — no card, no DAI, no DAPM widget,
nothing registered. The MT6357 codec takes no firmware, so there is nothing to
load; it only reports the topology it would drive.

Registering a card here would fight the in-tree `common_int` machine driver that
already provides `mt-snd-card`.

## Open questions

1. Whether `aw87519` (external speaker amp) is fitted — the driver exists in the
   tree, but no device-side evidence has been read yet.
2. Real speaker/headset routing tables come from a product DTSI, which this BSP
   does not contain.
