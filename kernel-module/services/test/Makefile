ifneq ($(KERNELRELEASE),)
# For older kbuild systems
include Kbuild
else

include ../../../Makefile.config
include ../../../Makefile.common

SKINK_HOME := $(CURDIR)/../..
EXTRA_CFLAGS += $(KERNEL_EXTRA_CFLAGS) -I$(SKINK_HOME)
KBUILD_EXTRA_SYMBOLS := $(RTAI_MODULE_SYMBOLS) $(SKINK_HOME)/Module.symvers

.PHONY: all
all:
	@$(MAKE) --no-print-directory -C /lib/modules/$(shell uname -r)/build M=$(CURDIR)\
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS="$(KBUILD_EXTRA_SYMBOLS)" modules
.PHONY: clean
clean:
	-@$(MAKE) --no-print-directory -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean

endif
