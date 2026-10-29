# SPDX-License-Identifier: GPL-2.0
#
# Out-of-tree build for the minimal bring-up modules.
#
# Target kernel: Nokia 2.3 V3.311 vendor tree, Linux 4.19.127, arm64
# (arch/arm64/configs/k61v1_64_bsp_defconfig). See KERNEL_SOURCE.md.
#
# If the vendor tree is extracted locally it is used automatically; otherwise
# this falls back to the host's installed headers (wrong arch for this phone,
# but useful for a syntax check).
#
# The development host has no arm64 compiler, so this Makefile has never run.
# See AGENTS.md.
#
#   make                     build every module
#   make KDIR=...            build against a specific kernel tree
#   make VENDOR_TREE=...     override the auto-detected vendor tree
#   make clean               remove build output

VENDOR_TREE ?= C:/n23/src/LINUX/android/kernel

KDIR ?= $(if $(wildcard $(VENDOR_TREE)/Makefile),$(VENDOR_TREE),/lib/modules/$(shell uname -r)/build)

# touch/hxchipset and PMIC/bq24157 are the real vendor drivers vendored into this
# repo; Wi-Fi and audio are intentionally excluded from the default build.
MODULES := touch/hxchipset PMIC/bq24157

.PHONY: all clean $(MODULES)

all: $(MODULES)

$(MODULES):
	$(MAKE) -C $(KDIR) M=$(CURDIR)/$@ modules

clean:
	@for d in $(MODULES); do \
		$(MAKE) -C $(KDIR) M=$(CURDIR)/$$d clean; \
	done
