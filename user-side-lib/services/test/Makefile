include ../../../Makefile.config
include ../../../Makefile.common

TARGET := test_service
TEST := test_connect
KERNEL_HOME := ../../../kernel-module
USER_HOME := ../..
CFLAGS += -I$(KERNEL_HOME) -I$(USER_HOME) $(USER_EXTRA_CFLAGS) $(EXTRA_CFLAGS)
SKIN_LINK := -L$(USER_HOME) -lskin-$(USER_VERSION) -lrt
LDFLAGS += $(SKIN_LINK) $(USER_EXTRA_LDFLAGS) $(EXTRA_LDFLAGS)

.PHONY: all
all: $(TARGET) $(TEST)
$(TARGET): main.o $(USER_HOME)/libskin.a
	$(LD) -o $@ $(filter-out %.a,$^) $(LDFLAGS)
$(TEST): test.o $(USER_HOME)/libskin.a
	$(LD) -o $@ $(filter-out %.a,$^) $(LDFLAGS)
%.o: %.c $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h) skin_test_service.h
	$(CC) $(CFLAGS) -c $< -o $@
main.o:
test.o:
.PHONY: clean
clean:
	-$(RM) *.o $(TARGET)
