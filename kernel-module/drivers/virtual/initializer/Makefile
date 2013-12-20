include ../../../../Makefile.config
include ../../../../Makefile.common

TARGET := initialize
CFLAGS += -Wall -g
RM := rm -f

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
.PHONY: clean
clean:
	-$(RM) $(TARGET)
