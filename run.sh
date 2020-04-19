g++ glad/glad.c imgui/*.cpp main.cpp imgui_impl_opengl3.cpp imgui_impl_sdl.cpp -I. -Iimgui -Iglad `sdl2-config --cflags --libs` -o test -DIMGUI_IMPL_OPENGL_LOADER_GLAD -ldl
