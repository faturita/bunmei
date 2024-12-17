ODEF:=$(shell ode-config --cflags)
ODEFL:=$(shell ode-config --libs)
CC = g++
CFLAGS = -std=c++17 -w -g -Wall $(ODEF) -I/usr/include -I/usr/include/GL -I/System/Library/Frameworks/OpenGL.framework/Headers -I../stk/include  -fpermissive
PROG = bunmei

SCS = src/City.cpp src/lodepng.cpp src/openglutils.cpp src/imageloader.cpp src/commandline.cpp src/profiling.cpp src/camera.cpp src/math/yamathutil.cpp src/math/vec3f.cpp src/font/DrawFonts.cpp src/hud.cpp src/map.cpp src/usercontrols.cpp src/map.cpp $(shell ls src/units/*.cpp) src/bunmei.cpp

SSRC = $(SCS)
OBJS = $(SSRC:.cpp=.o)

ifeq ($(shell uname),Darwin)
	LIBS = -lstk -lpthread -framework CoreAudio -framework CoreMIDI -framework CoreFoundation -framework OpenGL -framework GLUT $(ODEFL)
else
	LIBS = -L/usr/local/lib -I/usr/local/include -L/usr/lib/x86_64-linux-gnu/ -lGL -lGLU -lglut -pthread -lbsd
endif

all: $(PROG)

$(PROG):	$(OBJS)
	@echo "Object files are $(OBJS)"
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.cpp.o:		$(SSRC)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(PROG)
	rm -f $(OBJS)


