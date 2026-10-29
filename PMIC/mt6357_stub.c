// SPDX-License-Identifier: GPL-2.0
/*
 * mt6357_stub - minimal placeholder for the Nokia 2.3 (MT6761) power path.
 *
 * Unlike touch and Wi-Fi, the PMIC block has **no loadable firmware**: the
 * MT6357 registers, battery profile and charger configuration are all
 * compiled-in data. So this module is a pure stub - it registers nothing,
 * touches no bus and only reports the hardware it would eventually drive.
 *
 * Verified on the live device (see ../README.md):
 *   PMIC      MT6357 on pwrap@1000d000/main_pmic
 *   charger   external ETA6937, driven by MTK's bq2415x-family code
 *             (log tag [bq2415x], functions eta6937_*)
 *   supplies  battery, bq2415x-gauge, mtk-gauge, mtk_charger_type,
 *             mtk-master-charger, mtk-slave-charger, ac, usb
 *   batteries JIADE_VK, FENGHUA_FHPV (100-point OCV tables)
 *
 * The optional fw_name parameter exists so a board that *does* gain a blob
 * (battery profile, charger table) can be tested without rewriting this file.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/firmware.h>
#include <linux/err.h>

#define DRV_NAME	"mt6357_stub"

static char *fw_name = "";
module_param(fw_name, charp, 0444);
MODULE_PARM_DESC(fw_name, "Optional blob to load; empty on this board (no PMIC firmware)");

static const struct firmware *fw_held;

static int __init mt6357_stub_init(void)
{
	pr_info(DRV_NAME ": hardware path stubbed - nothing registered\n");
	pr_info(DRV_NAME ": would drive MT6357 regulators + ETA6937 charger\n");

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
	 * TODO: read the battery profile (JIADE_VK vs FENGHUA_FHPV).
	 * TODO: register a power_supply for the gauge.
	 * TODO: hook the ETA6937 charger over its I2C bus (bus/address unknown).
	 */
	return 0;
}

static void __exit mt6357_stub_exit(void)
{
	if (fw_held) {
		release_firmware(fw_held);
		fw_held = NULL;
	}
	pr_info(DRV_NAME ": unloaded\n");
}

module_init(mt6357_stub_init);
module_exit(mt6357_stub_exit);

MODULE_AUTHOR("Notempty-long/nokia-drivers");
MODULE_DESCRIPTION("MT6357 / ETA6937 placeholder - no firmware on this board");
MODULE_LICENSE("GPL");
