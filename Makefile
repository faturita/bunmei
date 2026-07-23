CC = g++
CFLAGS = -O3 -std=c++17 -w -g -Wall -I/usr/include -I/usr/include/GL -I/System/Library/Frameworks/OpenGL.framework/Headers -I../stk/include -I../boost  -fpermissive
PROG = bunmei

SCS = src/savegame.cpp src/resources.cpp src/tiles.cpp src/improvements.cpp src/diplomacy.cpp src/dialog.cpp src/messages.cpp src/City.cpp src/lodepng.cpp src/openglutils.cpp src/sounds/soundtexture.cpp src/sounds/sounds.cpp src/imageloader.cpp src/commandline.cpp src/profiling.cpp src/camera.cpp src/math/yamathutil.cpp src/math/vec3f.cpp src/font/FontsBitmap.cpp src/font/DrawFonts.cpp src/cityscreenui.cpp src/hud.cpp src/map.cpp src/usercontrols.cpp src/engine.cpp src/ai.cpp $(shell ls src/units/*.cpp) $(shell ls src/buildings/*.cpp) 

SSRC = $(SCS) src/gamekernel.cpp src/bunmei.cpp
OBJS = $(SSRC:.cpp=.o)

TCSRCS = $(SCS) src/bunmei.cpp src/tests/tester.cpp src/tests/testcase.cpp src/tests/testcase_$(TC).cpp
TCOBJS = $(TCSRCS:.cpp=.o)

SSM = src/math/vec3f.cpp src/math/yamathutil.cpp src/tiles.cpp src/resources.cpp src/engine.cpp src/City.cpp $(shell ls src/units/*.cpp) $(shell ls src/buildings/*.cpp)  src/simulate.cpp
OSSM = $(SSM:.cpp=.o)

ifeq ($(shell uname),Darwin)
	LIBS = -lstk -lpthread -framework CoreAudio -framework CoreMIDI -framework CoreFoundation -framework OpenGL -framework GLUT
else
	LIBS = -L/usr/local/lib -I/usr/local/include -L/usr/lib/x86_64-linux-gnu/ -lGL -lGLU -lglut -pthread -lbsd -lstk -lasound 
endif

all: $(PROG)

$(PROG):	$(OBJS)
	@echo "Object files are $(OBJS)"
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile with automatic header dependency tracking: -MMD writes a .d file next to
# each .o listing every project header it includes, so touching a .h recompiles exactly
# the .cpp files that use it (no more make clean after a header change).  -MP adds
# phony targets so deleting a header does not break the build.
%.o: %.cpp
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

DEPS = $(OBJS:.o=.d) $(TCOBJS:.o=.d) $(OSSM:.o=.d)
-include $(DEPS)

clean:
	rm -f $(PROG)
	rm -f $(OBJS)
	rm -f $(TOBJS)
	rm -f $(TCOBJS)
	rm -f testcase
	rm -f simulate
	rm -f $(DEPS)
	rm -f src/tests/testcase_*.d src/tests/testcase_*.o

testcase:	$(TCOBJS)
	@echo "Building test cases"
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

simulate:	$(OSSM)
	@echo "Building simulation"
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
