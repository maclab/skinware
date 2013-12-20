include ../../Makefile.config
include ../../Makefile.common

BUILDING_DRIVERS := n
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
  ifeq ($(CONFIG_SKIN_DRIVER_VIRTUAL), y)
    BUILDING_DRIVERS := y
  endif
endif

.PHONY: all
all:
ifeq ($(BUILDING_DRIVERS), y)
	@$(PRINTF) "$(STATUS_COLOR)Building skin drivers...$(DEFAULT_COLOR)\n"
  ifeq ($(CONFIG_SKIN_DRIVER_VIRTUAL), y)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Virtual$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C virtual $(HIGHLIGHT)
  endif
endif
.PHONY: clean
clean:
	-@$(MAKE) --no-print-directory -C virtual clean
