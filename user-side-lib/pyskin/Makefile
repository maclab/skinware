include ../../Makefile.config
include ../../Makefile.common

# Use helper to update when dependencies change
HELPER := setup.py

.PHONY: all clean
all:
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
	@$(MAKE) --no-print-directory $(HELPER)
	@$(MAKE) --no-print-directory service
endif
clean:
	-$(RM) -R build

$(HELPER): pyskin.cpp $(wildcard ../*.h) $(wildcard ../../kernel-module/*.h) ../libskin-$(USER_VERSION).so
	@$(MAKE) --no-print-directory clean
	@python setup.py "$(RTAI_HOME)/include" "$(USER_VERSION)" build
	@touch $@
