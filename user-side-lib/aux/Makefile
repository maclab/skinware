include ../../Makefile.config
include ../../Makefile.common

.PHONY: all clean
all:
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- shImage$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C shImage
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Ngin$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C Ngin
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Ngin3d$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C Ngin3d
	@$(PRINTF) "$(SUB_STATUS_COLOR)- shFont$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C shFont
endif
clean:
	@$(MAKE) --no-print-directory -C shImage clean
	@$(MAKE) --no-print-directory -C Ngin clean
	@$(MAKE) --no-print-directory -C Ngin3d clean
	@$(MAKE) --no-print-directory -C shFont clean
