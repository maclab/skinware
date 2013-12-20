TARGET := libNgin3d.a
RM := rm -f
OPTIONS := -Wall -g

$(TARGET): Ngin3d.o Ngin3d_input.o Ngin3d_actions.o Ngin3d_physics.o
	-$(RM) $(TARGET)
	ar cq $(TARGET) Ngin3d.o Ngin3d_input.o Ngin3d_actions.o Ngin3d_physics.o
%.o: %.cpp
	g++ -o $@ -c $< $(OPTIONS)
Ngin3d.o: Ngin3d.cpp Ngin3d.h Ngin3d_input.h
Ngin3d_input.o: Ngin3d_input.cpp Ngin3d_input.h
Ngin3d_actions.o: Ngin3d_actions.cpp Ngin3d_actions.h Ngin3d_base.h Ngin3d_physics.h Ngin3d_internal.h
Ngin3d_physics.o: Ngin3d_physics.cpp Ngin3d_physics.h Ngin3d_internal.h
clean:
	-$(RM) *.o $(TARGET)
