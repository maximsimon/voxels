CC = gcc
SCRIPT_NAME = voxel
TARGET = $(SCRIPT_NAME).o
SRC = $(SCRIPT_NAME).c

INCLUDE_DIRS = -I$(HOME)/raylib/src
LIB_DIRS = -L$(HOME)/raylib/build

LIBS = -lraylib -lm -ldl -lpthread -lGL -lX11 -g 

all: rebuild

rebuild:
	@if [ -f $(TARGET) ]; then rm $(TARGET); fi
	$(CC) $(SRC) -o $(TARGET) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS)


