include ../../../Makefile.config
include ../../../Makefile.common

TARGET := motion_detect
KERNEL_HOME := ../../../kernel-module
USER_HOME := ../..
TOOLS_HOME := $(USER_HOME)/tools
CXXFLAGS += -I$(KERNEL_HOME) -I$(USER_HOME) -I$(TOOLS_HOME) $(USER_EXTRA_CXXFLAGS) $(EXTRA_CXXFLAGS)
SKIN_LINK := -L$(USER_HOME) -lskin-$(USER_VERSION) -lrt
LDFLAGS += $(SKIN_LINK) $(USER_EXTRA_LDFLAGS) $(EXTRA_LDFLAGS)

VPATH = $(TOOLS_HOME)

.PHONY: all
all:
	@$(MAKE) --no-print-directory -C $(TOOLS_HOME)
	@$(MAKE) --no-print-directory $(TARGET)
$(TARGET): main.o scaler.o filter.o amplifier.o $(USER_HOME)/libskin.a
	$(LD) -o $@ $(filter-out %.a,$^) $(LDFLAGS)
main.o: main.cpp $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h) skin_user_motion.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
.PHONY: clean
clean:
	-$(RM) *.o $(TARGET)
