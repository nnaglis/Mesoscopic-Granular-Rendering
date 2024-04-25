APP_NAME = myApp
BUILD_DIR = ./bin
CPP_FILES = ./src/*.cpp
C_FILES = ./src/*.c

APP_DEFINES :=
APP_INCLUDES := -I./src/libraries/* -I./src/shaders/* -I./src/* $(shell find ./src/libraries -type d -exec echo -n "-I"{}" " \;)
APP_FRAMEWORKS := -framework Cocoa -framework OpenGL -framework IOKit
APP_LINKERS := -L./src/libraries/GLFW/lib -lglfw3 -L./src/libraries/assimp/lib -lassimp -Wl -rpath $(shell pwd)/src/libraries/assimp/lib
CXXFLAGS := -I./src/libraries -ferror-limit=1000 -std=c++11
CFLAGS := -I./src/libraries -ferror-limit=1000




build:
	clang++ $(CXXFLAGS) $(APP_INCLUDES) $(CPP_FILES) -c
	clang $(CFLAGS) $(APP_INCLUDES) $(C_FILES) -c
	clang++ *.o -o $(BUILD_DIR)/$(APP_NAME) $(APP_DEFINES) $(APP_INCLUDES) $(APP_FRAMEWORKS) $(APP_LINKERS)