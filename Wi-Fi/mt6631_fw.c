// SPDX-License-Identifier: GPL-2.0
/*
 * mt6631_fw - minimal loader for the MediaTek MT6631 combo firmware used by
 *             the Nokia 2.3 (MT6761, "Ironman" / TA-1206).
 *
 * Scope: request the real blobs from /vendor/firmware, hold them for the
 * lifetime of the module and report what was found. The WMT transport, the
 * gen4m Wi-Fi driver, consys power-up, BT and GPS are all stubbed - the real
 * driver (wlan_drv_gen4m.ko / wmt_drv.ko) is *not* in the reference BSP, see
 * ../README.md.
 *
 * Deliberately a plain module: registering a platform_driver against
 * "mediatek,wifi" or "mediatek,mt6761-consys" would fight the in-tree WMT
 * driver for the same device node.
 *
 * Verified on the live device:
 *   modules   wlan_drv_gen4m.ko, wmt_chrdev_wifi.ko, wmt_drv.ko
 *   interfaces wlan0, p2p0
 *   firmware  firmware_class.path=/vendor/firmware
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/firmware.h>
#include <linux/err.h>

#define DRV_NAME	"mt6631_fw"

/*
 * soc1_0_* is the set this unit loads; soc3_0_* ships for the other die
 * variant and is picked at runtime by WMT_SOC.cfg. Only soc1_0 is requested
 * here - add the soc3_0 names via fw_extra if you need them.
 */
static const char * const fw_list[] = {
	"WMT_SOC.cfg",
	"WIFI_RAM_CODE_soc1_0_1_1.bin",
	"soc1_0_ram_wifi_1_1_hdr.bin",
	"soc1_0_ram_mcu_1_1_hdr.bin",
	"soc1_0_ram_bt_1_1_hdr.bin",
};

static const struct firmware *fw_held[ARRAY_SIZE(fw_list)];
static int fw_count;

static int __init mt6631_fw_init(void)
{
	int i;

	pr_info(DRV_NAME ": hardware path stubbed - loading firmware only\n");

	for (i = 0; i < ARRAY_SIZE(fw_list); i++) {
		const struct firmware *fw;
		int ret;

		ret = request_firmware(&fw, fw_list[i], NULL);
		if (ret) {
			pr_warn(DRV_NAME ": %s unavailable (%d)\n",
				fw_list[i], ret);
			continue;
		}

		pr_info(DRV_NAME ": %s loaded, %zu bytes\n",
			fw_list[i], fw->size);
		print_hex_dump_bytes(DRV_NAME ": ", DUMP_PREFIX_OFFSET,
				     fw->data, min_t(size_t, fw->size, 32));

		fw_held[i] = fw;
		fw_count++;
	}

	pr_info(DRV_NAME ": %d/%zu blobs held\n", fw_count,
		ARRAY_SIZE(fw_list));

	/*
	 * TODO: read WMT_SOC.cfg to pick soc1_0 vs soc3_0.
	 * TODO: power up consys@18002000 and release it from reset.
	 * TODO: hand the RAM code to WMT instead of just holding it.
	 */
	return 0;
}

static void __exit mt6631_fw_exit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fw_list); i++) {
		if (fw_held[i]) {
			release_firmware(fw_held[i]);
			fw_held[i] = NULL;
		}
	}

	pr_info(DRV_NAME ": firmware released\n");
}

module_init(mt6631_fw_init);
module_exit(mt6631_fw_exit);

MODULE_FIRMWARE("WMT_SOC.cfg");
MODULE_FIRMWARE("WIFI_RAM_CODE_soc1_0_1_1.bin");
MODULE_AUTHOR("Notempty-long/nokia-drivers");
MODULE_DESCRIPTION("Minimal MT6631 combo firmware loader (no transport)");
MODULE_LICENSE("GPL");
