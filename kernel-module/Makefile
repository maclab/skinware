ifneq ($(KERNELRELEASE),)
# For older kbuild systems
include Kbuild
else

include ../Makefile.config
include ../Makefile.common

EXTRA_CFLAGS += $(KERNEL_EXTRA_CFLAGS)
KBUILD_EXTRA_SYMBOLS := $(RTAI_MODULE_SYMBOLS)
INC_BUILD := ./increase_build
BUILD_FILE := build.txt

.PHONY: all
all: module drivers services
.PHONY: module
module:
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
	@$(PRINTF) "$(STATUS_COLOR)Building skin kernel...$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C /lib/modules/$(shell uname -r)/build M=$(CURDIR)\
		EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS="$(KBUILD_EXTRA_SYMBOLS)" modules $(HIGHLIGHT)
  ifeq ($(CONFIG_SKIN_DEVELOPER_MODE), y)
	@$(MAKE) --no-print-directory $(INC_BUILD)
	@$(MAKE) --no-print-directory $(BUILD_FILE)
  endif
endif
.PHONY: drivers services
drivers:
	@$(MAKE) --no-print-directory -C drivers
services:
	@$(MAKE) --no-print-directory -C services

$(INC_BUILD): increase_build.c
	$(CC) -o $@ $< -lrt
$(BUILD_FILE): skin_kernel.ko
	$(INC_BUILD) $@

.PHONY: clean
clean:
	-@$(MAKE) --no-print-directory -C /lib/modules/$(shell uname -r)/build M=$(CURDIR) clean
	-@$(MAKE) --no-print-directory -C drivers clean
	-@$(MAKE) --no-print-directory -C services clean
	-$(RM) $(INC_BUILD)

endif
