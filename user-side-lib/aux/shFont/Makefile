TARGET := libshFont.a
RM := rm -f
OPTIONS := -Wall -g -I../shImage -I.

.PHONY: all
$(TARGET): shfont.o
	-$(RM) $(TARGET)
	ar cq $(TARGET) shfont.o
shfont.o: shfont.cpp shfont.h
	g++ -o $@ -c $< $(OPTIONS)
.PHONY: clean
clean:
	-$(RM) *.o $(TARGET)
