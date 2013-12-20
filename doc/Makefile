include ../Makefile.config
include ../Makefile.common

.PHONY: all
all:
ifeq ($(CONFIG_SKIN_DOC), y)
	@$(PRINTF) "$(STATUS_COLOR)Building documentation generator program...$(DEFAULT_COLOR)\n"
	@$(PRINTF) "$(SUB_STATUS_COLOR)- shCompiler$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C shCompiler $(HIGHLIGHT)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- DocThis!$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C DocThis $(HIGHLIGHT)
	@$(PRINTF) "$(STATUS_COLOR)Building documentation...$(DEFAULT_COLOR)\n"
	@$(PRINTF) "$(SUB_STATUS_COLOR)- General documentation$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C general $(HIGHLIGHT)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Application developer documentation$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C api $(HIGHLIGHT)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Service developer documentation$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C service-api $(HIGHLIGHT)
	@$(PRINTF) "$(SUB_STATUS_COLOR)- Driver developer documentation$(DEFAULT_COLOR)\n"
	@$(MAKE) --no-print-directory -C driver-api $(HIGHLIGHT)
endif
.PHONY: clean
clean:
ifeq ($(CONFIG_SKIN_DOC), y)
	-@$(MAKE) --no-print-directory -C api clean
	-@$(MAKE) --no-print-directory -C general clean
	-@$(MAKE) --no-print-directory -C service-api clean
	-@$(MAKE) --no-print-directory -C driver-api clean
endif
