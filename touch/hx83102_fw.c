// SPDX-License-Identifier: GPL-2.0
/*
 * hx83102_fw - minimal bring-up probe for the Himax HX83102D touch controller
 *              on the Nokia 2.3 (MediaTek MT6761, "Ironman" / TA-1206).
 *
 * Scope: bind the SPI device the stock DTB exposes, pull the Himax firmware
 * blob out of /vendor/firmware, prove it is readable, and stop. There is no
 * register access, no input device, no interrupt handler and no firmware
 * upload - all of that is stubbed.
 *
 * Verified on the live device (see ../README.md):
 *   controller   spi2@11012000, compatible "mediatek,mt6765-spi"
 *   chip select  reg = <0>                -> CS0
 *   clock        spi-max-frequency = 10 MHz
 *   interrupt    EINT0 = GPIO0, edge
 *   firmware     /vendor/firmware/Himax_firmware.bin
 *                (lid/e variants use Himax_firmware_lide.bin)
 *
 * WARNING: the in-tree driver at drivers/input/touchscreen/mediatek/hxchipset/
 * claims the *same* "mediatek,himax_cap_touch" compatible. Never load both -
 * whoever probes first owns the device.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/firmware.h>
#include <linux/err.h>

#define DRV_NAME	"hx83102_fw"

static char *fw_name = "Himax_firmware.bin";
module_param(fw_name, charp, 0444);
MODULE_PARM_DESC(fw_name, "Firmware blob to load (default: Himax_firmware.bin)");

static u32 spi_speed = 10000000;
module_param(spi_speed, uint, 0444);
MODULE_PARM_DESC(spi_speed, "SPI clock in Hz (default: 10000000, measured)");

static size_t dump_len = 32;
module_param(dump_len, ulong, 0444);
MODULE_PARM_DESC(dump_len, "Bytes of firmware header to log (default: 32)");

/* ------------------------------------------------------------------ */
/* firmware                                                           */
/* ------------------------------------------------------------------ */

static int hx83102_load_fw(struct device *dev)
{
	const struct firmware *fw;
	int ret;

	ret = request_firmware(&fw, fw_name, dev);
	if (ret) {
		dev_err(dev, "request_firmware(%s) failed: %d\n", fw_name, ret);
		return ret;
	}

	if (!fw->size) {
		dev_err(dev, "%s is empty\n", fw_name);
		release_firmware(fw);
		return -EINVAL;
	}

	dev_info(dev, "%s loaded, %zu bytes\n", fw_name, fw->size);
	print_hex_dump_bytes(DRV_NAME ": ", DUMP_PREFIX_OFFSET, fw->data,
			     min_t(size_t, fw->size, dump_len));

	/*
	 * TODO: parse the Himax header instead of dumping raw bytes. Gold
	 * values from the device's /proc/android_touch/vendor are
	 * FW_VER 0x1303, TOUCH_VER 0x32, DISPLAY_VER 0x00, CID_VER 0xE00,
	 * PANEL_VER 0x1D - use them to check the blob matches the panel.
	 */

	release_firmware(fw);
	return 0;
}

/* ------------------------------------------------------------------ */
/* spi glue                                                           */
/* ------------------------------------------------------------------ */

static int hx83102_probe(struct spi_device *spi)
{
	int ret;

	dev_info(&spi->dev, "probe: CS%u, %u Hz, mode 0x%x, irq %d\n",
		 spi->chip_select, spi->max_speed_hz, spi->mode, spi->irq);

	spi->bits_per_word = 8;
	spi->max_speed_hz = spi_speed;

	ret = spi_setup(spi);
	if (ret) {
		dev_err(&spi->dev, "spi_setup failed: %d\n", ret);
		return ret;
	}

	/*
	 * The in-tree driver looks up a pinctrl state named "state_spi_all".
	 * On this phone the pinctrl nodes contain only name + phandle, so the
	 * lookup is optional here too. Uncomment when the groups exist.
	 */
	/* devm_pinctrl_get_select(&spi->dev, "state_spi_all"); */

	ret = hx83102_load_fw(&spi->dev);
	if (ret)
		dev_warn(&spi->dev, "no usable firmware (%d)\n", ret);

	/* TODO: reset pulse on the (still unknown) reset GPIO. */
	/* TODO: read the Himax chip ID over SPI and compare with HX83102D. */
	/* TODO: upload firmware, then register an input device. */
	/* TODO: request_irq(spi->irq, ..., EINT0/GPIO0, edge). */

	dev_info(&spi->dev, "bound, hardware path stubbed - driver is inert\n");
	return 0;
}

static int hx83102_remove(struct spi_device *spi)
{
	dev_info(&spi->dev, "removed\n");
	return 0;
}

static const struct of_device_id hx83102_of_match[] = {
	{ .compatible = "mediatek,himax_cap_touch" },
	{ }
};
MODULE_DEVICE_TABLE(of, hx83102_of_match);

static struct spi_driver hx83102_driver = {
	.driver = {
		.name		= DRV_NAME,
		.of_match_table	= hx83102_of_match,
	},
	.probe	= hx83102_probe,
	.remove	= hx83102_remove,
};
module_spi_driver(hx83102_driver)

MODULE_FIRMWARE("Himax_firmware.bin");
MODULE_FIRMWARE("Himax_firmware_lide.bin");
MODULE_AUTHOR("Notempty-long/nokia-drivers");
MODULE_DESCRIPTION("Minimal HX83102D SPI probe that only loads firmware");
MODULE_LICENSE("GPL");
