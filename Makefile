# SPDX-License-Identifier: GPL-2.0
#
# Out-of-tree build for the minimal bring-up modules.
#
# This needs a Linux host with kernel headers for the *target* kernel:
#   Ubuntu/Debian:  sudo apt install build-essential linux-headers-$(uname -r)
# The Windows host this repo is developed on has no compiler at all, so this
# Makefile cannot run there. See AGENTS.md.
#
#   make              build every module
#   make KDIR=...     build against a specific kernel tree
#   make clean        remove build output

KDIR ?= /lib/modules/$(shell uname -r)/build
SUBDIRS := touch Wi-Fi PMIC audio

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $(KDIR) M=$(CURDIR)/$@ modules

clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C $(KDIR) M=$(CURDIR)/$$d clean; \
	done
