clang++ -O3 -march=native -Wall `sdl2-config --cflags` `sdl2-config --libs` -lSDL2_image -framework OpenGL src/main.cpp -o snow
