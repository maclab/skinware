include ../../../Makefile.config
include ../../../Makefile.common

# Use helper to update when dependencies change
HELPER := setup.py
TEST := test

KERNEL_HOME := ../../../kernel-module
USER_HOME := ../..
CFLAGS += -I$(KERNEL_HOME) -I$(USER_HOME) $(USER_EXTRA_CFLAGS) $(EXTRA_CFLAGS)
SKIN_LINK := -L$(USER_HOME) -lskin-$(USER_VERSION) -lrt
LDFLAGS += $(SKIN_LINK) $(USER_EXTRA_LDFLAGS) $(EXTRA_LDFLAGS)

.PHONY: all clean
all: $(HELPER) $(TEST)
clean:
	-$(RM) -R build
	-$(RM) *.o $(TEST)

$(HELPER): example_pyservice.c example_pyservice.h $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h) $(USER_HOME)/libskin-$(USER_VERSION).so
	@$(MAKE) --no-print-directory clean
	@python setup.py "$(RTAI_HOME)/include" "$(USER_VERSION)" build
	@touch $@
$(TEST): test.o $(USER_HOME)/libskin.a
	$(LD) -o $@ $(filter-out %.a,$^) $(LDFLAGS)
test.o: test.c example_pyservice.h $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h)
	$(CC) $(CFLAGS) -c $< -o $@
