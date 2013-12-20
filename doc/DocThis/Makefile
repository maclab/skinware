include Makefile.config
include Makefile.common

TARGET := docthis
RM := rm -f
ifeq ($(OS_TYPE),Windows)
  TARGET := $(TARGET).exe
endif

.PHONY: all
all:
	@$(MAKE) --no-print-directory -C src
.PHONY: install uninstall
install uninstall:
	@$(MAKE) --no-print-directory -f Makefile.install $@
.PHONY: clean
clean:
	@$(MAKE) --no-print-directory -C src clean
	-$(RM) $(TARGET)
