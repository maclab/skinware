TARGET := libshImage.a
RM := rm -f
OPTIONS := -Wall -g

.PHONY: all
$(TARGET): shBMP.o
	-$(RM) $(TARGET)
	ar cq $(TARGET) shBMP.o
shBMP.o: shBMP.cpp shBMP.h
	g++ -o $@ -c $< $(OPTIONS)
.PHONY: clean
clean:
	-$(RM) *.o $(TARGET)
