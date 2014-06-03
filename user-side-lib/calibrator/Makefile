include ../../Makefile.config
include ../../Makefile.common

TARGET := calibrate
KERNEL_HOME := ../../kernel-module
USER_HOME := ..
CFLAGS += -I$(KERNEL_HOME) -I$(USER_HOME) $(USER_EXTRA_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS += -L$(USER_HOME) -lskin-$(USER_VERSION) -lrt $(USER_EXTRA_LDFLAGS) $(EXTRA_LDFLAGS)

VPATH = ../tools

.PHONY: all
all:
ifneq ($(CONFIG_SKIN_ROSSKIN_LIB_ONLY), y)
	@$(MAKE) --no-print-directory -C ../tools
	@$(MAKE) --no-print-directory $(TARGET)
endif
$(TARGET): main.o $(USER_HOME)/libskin.a
	$(LD) -o $@ $(filter-out %.a,$^) $(LDFLAGS)
main.o: main.c $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h)
	$(CC) $(CFLAGS) -o $@ -c $<
.PHONY: clean
clean:
	-$(RM) *.o $(TARGET)
