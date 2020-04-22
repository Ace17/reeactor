
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

all: all_targets

-include $(HOST).mk

engine.srcs:=\
	src/platform/imgui_impl_opengl3.cpp\
	src/platform/imgui_impl_sdl.cpp\
	src/platform/main.cpp\
	extra/glad/glad.cpp\
	extra/imgui/imgui.cpp\
	extra/imgui/imgui_demo.cpp\
	extra/imgui/imgui_draw.cpp\
	extra/imgui/imgui_widgets.cpp\

game.srcs:=\
	src/app.cpp\
	src/game.cpp\
	src/simuflow.cpp\
	$(engine.srcs)\

$(BIN)/game.exe: $(game.srcs:%=$(BIN)/%.o)
TARGETS+=$(BIN)/game.exe

#------------------------------------------------------------------------------

testapp.srcs:=\
	src/apptest.cpp\
	src/simuflow.cpp\
	$(engine.srcs)\

$(BIN)/testapp.exe: $(testapp.srcs:%=$(BIN)/%.o)
TARGETS+=$(BIN)/testapp.exe

#------------------------------------------------------------------------------

all_targets: $(TARGETS)

$(BIN)/%.exe:
	@mkdir -p $(dir $@)
	$(CXX) -o "$@" $^ $(LDFLAGS)

$(BIN)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) -MM -MT "$@" -c -o "$(BIN)/$*.cpp.o.dep" $< $(CXXFLAGS)
	$(CXX) -c -o "$@" $< $(CXXFLAGS)

-include $(shell test -d $(BIN) && find $(BIN) -name "*.dep")

clean:
	rm -rf $(BIN)
