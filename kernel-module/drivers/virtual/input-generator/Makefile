include ../../../../Makefile.config
include ../../../../Makefile.common

TARGET := generate
CXXFLAGS += -Wall
RM := rm -f

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)
.PHONY: clean
clean:
	-$(RM) $(TARGET)
