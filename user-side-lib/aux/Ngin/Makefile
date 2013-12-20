TARGET := libNgin.a
RM := rm -f
OPTIONS := -Wall -g -I../shImage

$(TARGET): Ngin.o Ngin_texture.o Ngin_frustum.o Ngin_light.o Ngin_object.o Ngin_primitives.o shLoad3ds.o
	-$(RM) $(TARGET)
	ar cq $(TARGET) Ngin.o Ngin_texture.o Ngin_frustum.o Ngin_light.o Ngin_object.o Ngin_primitives.o shLoad3ds.o
%.o: %.cpp
	g++ -o $@ -c $< $(OPTIONS)
Ngin.o: Ngin.cpp Ngin.h Ngin_texture.h Ngin_frustum.h shmacros.h
Ngin_texture.o: Ngin_texture.cpp Ngin_texture.h shmacros.h
Ngin_frustum.o: Ngin_frustum.cpp Ngin_frustum.h shmacros.h
Ngin_light.o: Ngin_light.cpp Ngin_light.h Ngin_opengl.h
Ngin_object.o: Ngin_object.cpp Ngin_object.h Ngin_opengl.h
Ngin_primitives.o: Ngin_primitives.cpp Ngin_primitives.h Ngin_opengl.h
shLoad3ds.o: shLoad3ds.h shLoad3ds.cpp shmacros.h
clean:
	-$(RM) *.o $(TARGET)
