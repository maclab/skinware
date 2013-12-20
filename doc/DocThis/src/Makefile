include ../Makefile.config

TARGET := ../docthis
SH_COMPILER_HOME := ../../shCompiler
OPTIONS := -Wall -I$(SH_COMPILER_HOME)/include -O2
LINK := -L$(SH_COMPILER_HOME) -lshCompiler
RM := rm -f

.PHONY: all
all: $(TARGET)
%.o: %.cpp
	g++ -o $@ -c $< $(OPTIONS)
semantics.o: semantics.cpp semantics.h specials.h error_print.h
main.o: main.cpp semantics.h specials.h error_print.h originals.h
specials.o: specials.cpp specials.h semantics.h error_print.h
error_print.o: error_print.cpp error_print.h
originals.o: originals.cpp originals.h
$(TARGET): semantics.o main.o specials.o error_print.o originals.o
	g++ -o $@ $^ $(LINK)
clean:
	$(RM) *.o
