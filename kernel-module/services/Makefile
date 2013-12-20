include ../../Makefile.config
include ../../Makefile.common

BUILDING_SERVICES := n
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
  ifeq ($(CONFIG_SKIN_SERVICE_TEST), y)
    BUILDING_SERVICES := y
  endif
  ifeq ($(CONFIG_SKIN_SERVICE_MOTION), y)
    BUILDING_SERVICES := y
  endif
endif

.PHONY: all
all:
ifeq ($(BUILDING_SERVICES), y)
	@$(PRINTF) "$(STATUS_COLOR)Building skin services...$(DEFAULT_COLOR)\n"
  ifeq ($(CONFIG_SKIN_SERVICE_TEST), y)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Test$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C test $(HIGHLIGHT)
  endif
  ifeq ($(CONFIG_SKIN_SERVICE_MOTION), y)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Motion Tracker$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C motion $(HIGHLIGHT)
  endif
endif
.PHONY: clean
clean:
	-@$(MAKE) --no-print-directory -C test clean
	-@$(MAKE) --no-print-directory -C motion clean
