APP_NAME = myApp
BUILD_DIR = ./bin
CPP_FILES = ./src/*.cpp
C_FILES = ./src/*.c

APP_DEFINES :=
APP_INCLUDES := -I./src/libraries/* -I./src/shaders/* -I./src/*
APP_FRAMEWORKS := -framework Cocoa -framework OpenGL -framework IOKit
APP_LINKERS := -L./src/libraries/GLFW/lib -lglfw3
CFLAGS := -I./src/libraries



build:
	clang++ $(CFLAGS) $(CPP_FILES) -c
	clang $(CFLAGS) $(C_FILES) -c
	clang++ *.o -o $(BUILD_DIR)/$(APP_NAME) $(APP_DEFINES) $(APP_INCLUDES) $(APP_FRAMEWORKS) $(APP_LINKERS)