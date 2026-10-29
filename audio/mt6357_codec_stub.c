// SPDX-License-Identifier: GPL-2.0
/*
 * mt6357_codec_stub - minimal placeholder for the Nokia 2.3 (MT6761) audio path.
 *
 * The codec is inside the MT6357 PMIC and takes no firmware, so this is a
 * pure stub: it registers no card, no DAI and no DAPM widget, and only
 * reports the topology it would eventually drive.
 *
 * Verified on the live device (see ../README.md):
 *   card 0  "mt-snd-card"     <- sound/soc/mediatek/common_int/mtk-soc-machine.c
 *   card 1  "mt63xx-accdet"   <- mt6357-accdet.ko, headset jack detect
 *   codec   "mt6357-sound"    (MT6357 sub-PMIC)
 *   config  CONFIG_MTK_SND_SOC_NEW_ARCH=y, CONFIG_MTK_SND_SOC_CODEC="mt6357"
 *
 * There is no mt6761 machine driver to write: MT6761 uses the shared
 * common_int card. Adding a second card here would only fight it.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/firmware.h>
#include <linux/err.h>

#define DRV_NAME	"mt6357_codec_stub"

static char *fw_name = "";
module_param(fw_name, charp, 0444);
MODULE_PARM_DESC(fw_name, "Optional blob to load; empty on this board (codec has no firmware)");

static const struct firmware *fw_held;

static int __init mt6357_codec_stub_init(void)
{
	pr_info(DRV_NAME ": hardware path stubbed - no card registered\n");
	pr_info(DRV_NAME ": playback path would be AFE audio@11220000 -> MT6357 codec\n");

	if (!fw_name || !*fw_name) {
		pr_info(DRV_NAME ": no firmware for this block, loader idle\n");
		return 0;
	}

	if (request_firmware(&fw_held, fw_name, NULL)) {
		pr_warn(DRV_NAME ": %s unavailable\n", fw_name);
		return 0;
	}

	pr_info(DRV_NAME ": %s loaded, %zu bytes\n", fw_name, fw_held->size);
	print_hex_dump_bytes(DRV_NAME ": ", DUMP_PREFIX_OFFSET, fw_held->data,
			     min_t(size_t, fw_held->size, 32));

	/*
	 * TODO: check whether aw87519 (external speaker amp) is fitted.
	 * TODO: real routing tables need the product DTSI, absent from the BSP.
	 */
	return 0;
}

static void __exit mt6357_codec_stub_exit(void)
{
	if (fw_held) {
		release_firmware(fw_held);
		fw_held = NULL;
	}
	pr_info(DRV_NAME ": unloaded\n");
}

module_init(mt6357_codec_stub_init);
module_exit(mt6357_codec_stub_exit);

MODULE_AUTHOR("Notempty-long/nokia-drivers");
MODULE_DESCRIPTION("MT6357 codec placeholder - no firmware on this board");
MODULE_LICENSE("GPL");
