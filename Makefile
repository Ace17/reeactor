
CROSS_COMPILE?=
ifneq (,$(CROSS_COMPILE))
CXX:=$(CROSS_COMPILE)g++
endif

BIN?=bin
CXXFLAGS+=-Iextra/imgui -Iextra -Isrc -I.

CXXFLAGS+=$(shell pkg-config sdl2 --cflags)
LDFLAGS+=$(shell pkg-config sdl2 --libs)

CXXFLAGS+=-DIMGUI_IMPL_OPENGL_LOADER_GLAD

HOST:=$(shell $(CXX) -dumpmachine | sed 's/.*-//')

-include $(HOST).mk

SRCS:=\
	src/app.cpp\
	src/game.cpp\
	src/simuflow.cpp\
	src/platform/imgui_impl_opengl3.cpp\
	src/platform/imgui_impl_sdl.cpp\
	src/platform/main.cpp\

SRCS+=\
	extra/glad/glad.cpp\
	extra/imgui/imgui.cpp\
	extra/imgui/imgui_demo.cpp\
	extra/imgui/imgui_draw.cpp\
	extra/imgui/imgui_widgets.cpp\

all: $(BIN)/game.exe

$(BIN)/game.exe: $(SRCS:%=$(BIN)/%.o)
	@mkdir -p $(dir $@)
	$(CXX) -o "$@" $^ $(LDFLAGS)

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o "$@" $< $(CXXFLAGS)

clean:
	rm -rf $(BIN)
