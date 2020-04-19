BIN?=bin
CXXFLAGS+=-Iimgui -Iglad -Isrc -I.

CXXFLAGS+=$(shell pkg-config sdl2 --cflags)
LDFLAGS+=$(shell pkg-config sdl2 --libs)

CXXFLAGS+=-DIMGUI_IMPL_OPENGL_LOADER_GLAD

HOST:=$(shell $(CXX) -dumpmachine | sed 's/.*-//')

-include $(HOST).mk

SRCS:=\
	glad/glad.cpp\
	imgui/imgui.cpp\
	imgui/imgui_demo.cpp\
	imgui/imgui_draw.cpp\
	imgui/imgui_widgets.cpp\
	platform/imgui_impl_opengl3.cpp\
	platform/imgui_impl_sdl.cpp\
	platform/main.cpp\
	src/simuflow.cpp\
	src/game.cpp\
	src/app.cpp\

all: $(BIN)/game.exe

$(BIN)/game.exe: $(SRCS:%=$(BIN)/%.o)
	@mkdir -p $(dir $@)
	$(CXX) -o "$@" $^ $(LDFLAGS)

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o "$@" $< $(CXXFLAGS)

clean:
	rm -rf $(BIN)
