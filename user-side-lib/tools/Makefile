include ../../Makefile.config
include ../../Makefile.common

TARGET := test
KERNEL_HOME := ../../kernel-module
USER_HOME := ..
CXXFLAGS += -I$(KERNEL_HOME) -I$(USER_HOME) $(USER_EXTRA_CXXFLAGS) $(EXTRA_CXXFLAGS)
SKIN_LINK := -L$(USER_HOME) -lskin -lrt
LDFLAGS += $(SKIN_LINK) $(USER_EXTRA_LDFLAGS) $(EXTRA_LDFLAGS)

OBJECTS := triangulate.o filter.o scaler.o amplifier.o test.o $(TARGET)
TARGET_OBJECTS := filter.o scaler.o amplifier.o

.PHONY: all clean
all: $(OBJECTS)
clean:
	-$(RM) $(OBJECTS)

%.o: %.cpp %.h $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h)
	$(CXX) $(CXXFLAGS) -o $@ -c $<
test.o: test.cpp $(TARGET_OBJECTS:.o=.h) $(wildcard $(USER_HOME)/*.h) $(wildcard $(KERNEL_HOME)/*.h)
	$(CXX) $(CXXFLAGS) -o $@ -c $<
$(TARGET): test.o filter.o scaler.o amplifier.o
	$(LD) -o $@ $^ $(LDFLAGS)
